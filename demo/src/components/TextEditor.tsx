import Editor, { OnChange, useMonaco } from '@monaco-editor/react';
import type { DBEvent, OperationBuilder, ProjectDB, Timestamp } from "@panzoid/crdbl";
import EventEmitter from '../utility/EventEmitter';
import { useCallback, useEffect, useRef } from 'react';
import type * as Monaco from 'monaco-editor';

export type TextEditorProps = {
  db: ProjectDB;
  builder: OperationBuilder;
  onEvent: EventEmitter<[DBEvent]>;
  valueId: string;
  siteId: number;
};

const OPERATION_TIMEOUT = 400;
const MAX_OPERATION_LATENCY = 5000;

enum EditOperationType
{
  delete,
  insert
}

type EditOperationCommon =
{
  type: EditOperationType;
  offset: number;
};

type InsertEditOperation = EditOperationCommon &
{
  type: EditOperationType.insert;
  text: string;
};

type DeleteEditOperation = EditOperationCommon &
{
  type: EditOperationType.delete;
  length: number;
};

type EditOperation = InsertEditOperation | DeleteEditOperation;

export const TextEditor = (props: TextEditorProps) => {
  const monaco = useMonaco();

  const currentOperation = useRef<EditOperation | null>(null);
  const selfOps = useRef<Set<number>>(new Set());
  const latestVersionId = useRef<number>(0);
  const operationTimeoutId = useRef<number>(0);
  const operationTimeoutId2 = useRef<number>(0);

  const applyTextEdit = useCallback((model: Monaco.editor.ITextModel,
    offset: number, data: number | string) =>
  {
    if (!monaco || !model)
      return;

    const startPos = model.getPositionAt(offset);
    const endPos = (typeof data === "number")
      ? model.getPositionAt(offset + data)
      : undefined;

    const range = monaco.Range.fromPositions(startPos, endPos);

    const editOperations =
    [{
      range: range,
      text: (typeof data === "string") ? data : null
    }];

    latestVersionId.current = model.getVersionId() + 1;
    model.applyEdits(editOperations);
  }, [monaco]);

  const textEvent = useCallback((ts: Timestamp, offset: number, data: number | string) =>
  {
    if (ts[1] === props.builder.getSiteId() &&
      selfOps.current.has(ts[0]))
    {
      selfOps.current.delete(ts[0]);
      return;
    }

    let performEdit = true;

    const currentOp = currentOperation.current;

    if (currentOp)
    {
      if (offset >= currentOp.offset)
      {
        if (currentOp.type === EditOperationType.insert)
        {
          //just move the new operation past the buffered op
          offset += currentOp.text.length;
        }
        else
        {
          if (typeof data === "string")
          {
            if (offset < currentOp.offset + currentOp.length)
            {
              currentOp.length += data.length;
              performEdit = false;
            }
            else
            {
              offset -= currentOp.length;
            }
          }
          else
          {
            if (offset < currentOp.offset + currentOp.length)
            {
              currentOp.length -= data;
              performEdit = false;
            }
            else
            {
              offset -= currentOp.length;
            }
          }
        }
      }
      else
      {
        if (typeof data === "string")
        {
          //just move the buffered operation forward
          currentOp.offset += data.length;
        }
        else
        {
          if (offset + data > currentOp.offset)
          {
            if (currentOp.type === EditOperationType.insert)
            {
              data += currentOp.text.length;
              currentOp.text = "";
            }
            else
            {
              const overlap = Math.min(offset + data - currentOp.offset,
                currentOp.length);
              data -= overlap;
              currentOp.length -= overlap;
            }

            currentOp.offset = offset;
          }
          else
          {
            //just move the buffered operation back
            currentOp.offset -= data;
          }
        }
      }
    }

    if (performEdit)
    {
      const model = monaco?.editor.getModel(monaco.Uri.parse(String(props.siteId)));
      applyTextEdit(model!, offset, data);
    }
  }, [applyTextEdit, monaco?.Uri, monaco?.editor, props.builder, props.siteId]);

  useEffect(() => {
    const handleEvent = (event: DBEvent) =>
    {
      if (event.speculative)
        return;

      if (event.id === props.valueId)
      {
        if (event.eventType === "NodeBlockValueInserted")
        {
          textEvent(event.ts, event.offset, event.value);
        }
        else if (event.eventType === "NodeBlockValueDeleted")
        {
          textEvent(event.ts, event.offset, event.length);
        }
      }
    };

    //resync anything that was missed
    currentOperation.current = null;
    selfOps.current.clear();

    const model = monaco?.editor.getModel(monaco.Uri.parse(String(props.siteId)));
    if (model)
    {
      //reset the model to the latest value
      //NOTE: due to the timing of things, this probably doesn't do anything
      //  because the model is usually not ready by this point
      model.setValue(props.db.getNodeBlockValue(props.valueId));
      latestVersionId.current = model.getVersionId();
    }

    props.onEvent.watch(handleEvent);

    return () => {
      props.onEvent.unwatch(handleEvent);
    };
  }, [monaco?.Uri, monaco?.editor, props.db, props.onEvent, props.siteId, props.valueId, textEvent]);

  const commitCurrentOperation = useCallback(() =>
  {
    if (!currentOperation.current)
      return;

    props.builder.startGroup();
    const oldTs = props.builder.getNextTimestamp()[0];

    if (currentOperation.current.type === EditOperationType.insert)
    {
      if (currentOperation.current.text.length > 0)
      {
        props.builder.insertText(props.valueId, currentOperation.current.offset,
          currentOperation.current.text);
      }
    }
    else
    {
      if (currentOperation.current.length > 0)
      {
        props.builder.deleteText(props.valueId, currentOperation.current.offset,
          currentOperation.current.length);
      }
    }

    const tsDiff = props.builder.getNextTimestamp()[0] - oldTs;
    for (let i = 0; i < tsDiff; i++)
    {
      selfOps.current.add(oldTs + i);
    }

    props.builder.commitGroup();

    currentOperation.current = null;
    clearTimeout(operationTimeoutId.current);
    clearTimeout(operationTimeoutId2.current);
  }, [props.builder, props.valueId]);

  const onChangeCallback = useCallback<OnChange>((_, e) => {
    if (e.versionId <= latestVersionId.current)
    {
      return;
    }

    if (e.changes.length > 1)
    {
      if (currentOperation.current)
      {
        commitCurrentOperation();
      }

      props.builder.startGroup();
      const oldTs = props.builder.getNextTimestamp()[0];

      for (let i = 0; i < e.changes.length; i++)
      {
        const change = e.changes[i];
        if (change.rangeLength)
        {
          props.builder.deleteText(props.valueId, change.rangeOffset, change.rangeLength);
        }
        if (change.text)
        {
          props.builder.insertText(props.valueId, change.rangeOffset, change.text);
        }
      }

      const tsDiff = props.builder.getNextTimestamp()[0] - oldTs;
      for (let i = 0; i < tsDiff; i++)
      {
        selfOps.current.add(oldTs + i);
      }

      props.builder.commitGroup();
    }
    else
    {
      const change = e.changes[0];

      if (change.rangeLength)
      {
        if (currentOperation.current)
        {
          if (currentOperation.current.type === EditOperationType.delete &&
            currentOperation.current.offset === change.rangeOffset + change.rangeLength)
          {
            //delete backward
            currentOperation.current.offset = change.rangeOffset;
            currentOperation.current.length += change.rangeLength;
          }
          else if (currentOperation.current.type === EditOperationType.delete &&
            currentOperation.current.offset === change.rangeOffset)
          {
            //delete forward
            currentOperation.current.length += change.rangeLength;
          }
          else if (currentOperation.current.type === EditOperationType.insert &&
            currentOperation.current.offset + currentOperation.current.text.length === change.rangeOffset + change.rangeLength)
          {
            //deleted from the end of the current operation
            if (change.rangeLength > currentOperation.current.text.length)
            {
              const newLength = change.rangeLength - currentOperation.current.text.length;
              currentOperation.current =
              {
                type: EditOperationType.delete,
                offset: change.rangeOffset,
                length: newLength
              } as DeleteEditOperation;
            }
            else
            {
              currentOperation.current.text = currentOperation.current.text.substring(
                0, change.rangeOffset - currentOperation.current.offset);
            }
          }
          else
          {
            commitCurrentOperation();
          }
        }

        if (!currentOperation.current)
        {
          currentOperation.current =
          {
            type: EditOperationType.delete,
            offset: change.rangeOffset,
            length: change.rangeLength
          } as DeleteEditOperation;
          operationTimeoutId.current = window.setTimeout(
            commitCurrentOperation, MAX_OPERATION_LATENCY);
        }
      }

      if (change.text)
      {
        if (currentOperation.current)
        {
          if (currentOperation.current.type === EditOperationType.insert &&
            currentOperation.current.offset + currentOperation.current.text.length === change.rangeOffset)
          {
            currentOperation.current.text += change.text;
          }
          else
          {
            commitCurrentOperation();
          }
        }

        if (!currentOperation.current)
        {
          currentOperation.current =
          {
            type: EditOperationType.insert,
            offset: change.rangeOffset,
            text: change.text
          } as InsertEditOperation;
          operationTimeoutId.current = window.setTimeout(
            commitCurrentOperation, MAX_OPERATION_LATENCY);
        }
      }

      clearTimeout(operationTimeoutId2.current);
      operationTimeoutId2.current = window.setTimeout(
        commitCurrentOperation, OPERATION_TIMEOUT);
    }
  }, [commitCurrentOperation, props.builder, props.valueId]);

  return (
    <div>
      <Editor
        path={String(props.siteId)}
        theme="vs-dark"
        width="100%"
        height="100%"
        onChange={onChangeCallback}
        onMount={(editor) => {
          editor.getModel()?.setValue(props.db.getNodeBlockValue(props.valueId));
        }}
        defaultLanguage="plaintext" />
    </div>
  );
};