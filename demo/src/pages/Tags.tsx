import init, { DBEvent, OperationFilter, Tag, crdblModule } from "@panzoid/crdbl";
import { useCallback, useEffect, useMemo, useState } from "react";
import EventEmitter from "../utility/EventEmitter";
import { TextEditor } from "../components/TextEditor";
import styles from "./Tags.module.css";

const createCtx = (module: crdblModule, siteId: number) => {
  const eventEmitter = new EventEmitter<[DBEvent]>();
  const db = new module.ProjectDB(() => {}, (event) =>
    {
      eventEmitter.emit(event);
    });
  const log = new module.OperationLog();
  const logApplyStream = log.createApplyStream();

  const builder = db.createOperationBuilder();
  builder.setSiteId(siteId);
  builder.getReadableStream().pipeTo(logApplyStream);

  const dbApplyStream = db.createApplyStream();
  log.createReadStream().pipeTo(dbApplyStream);

  const projectData: Uint8Array[] = [];
  const serializer = module.LogOperationSerialization.CreateSerializer(
    module.LogOperationSerialization.DefaultFormat());
  const dataStream = new module.DataCallbackStream(
    (data) => {
      projectData.push(data.slice());
    },
    () => {});

  const onlyMyOpsFilter = new module.FilterOperationStream();
  onlyMyOpsFilter.getFilter().setSiteFilter(siteId);

  log.createReadStream().pipeTo(onlyMyOpsFilter.asWritable());
  onlyMyOpsFilter.asReadable().pipeTo(serializer.asWritable());
  serializer.asReadable().pipeTo(dataStream);

  const deserializer = module.LogOperationSerialization.CreateDeserializer(
    module.LogOperationSerialization.DefaultFormat(),
    module.DeserializeDirection.Forward);
  deserializer.asReadable().pipeTo(logApplyStream);

  const valueId = (siteId === 1)
    ? builder.createNode("StringValue")
    : module.ProjectDB.INITID();


  const currentFilter = new module.OperationFilter();
  const applyFilter = (newFilter: OperationFilter) => {
    //reverse pass
    //unapply operations from the db that are not included in the new filter
    //for now, operations always must be unapplied in reverse order
    {
      const deserializer = module.LogOperationSerialization.CreateDeserializer(
        module.LogOperationSerialization.DefaultFormat(),
        module.DeserializeDirection.Reverse);

      const newFilterStream = new module.FilterOperationStream();
      newFilterStream.getFilter().setFromOther(newFilter);
      newFilterStream.getFilter().invert();
      deserializer.asReadable().pipeTo(newFilterStream.asWritable());

      const currentFilterStream = new module.FilterOperationStream();
      currentFilterStream.getFilter().setFromOther(currentFilter);
      newFilterStream.asReadable().pipeTo(currentFilterStream.asWritable());

      const unapplyStream = db.createUnapplyStream();
      currentFilterStream.asReadable().pipeTo(unapplyStream);

      for (let i = projectData.length - 1; i >= 0; i--) {
        deserializer.asWritable().write(projectData[i]);
      }

      deserializer.asWritable().close();
      newFilterStream.asWritable().close();
      currentFilterStream.asWritable().close();
      unapplyStream.close();

      deserializer.delete();
      newFilterStream.delete();
      currentFilterStream.delete();
      unapplyStream.delete();
    }

    //forward pass
    //apply operations from the project that are
    //  - included in the new filter
    //  - weren't included in the previous filter (i.e. aren't in the db)
    {
      const deserializer = module.LogOperationSerialization.CreateDeserializer(
        module.LogOperationSerialization.DefaultFormat(),
        module.DeserializeDirection.Forward);

      const newFilterStream = new module.FilterOperationStream();
      newFilterStream.getFilter().setFromOther(newFilter);
      deserializer.asReadable().pipeTo(newFilterStream.asWritable());

      const currentFilterStream = new module.FilterOperationStream();
      currentFilterStream.getFilter().setFromOther(currentFilter);
      currentFilterStream.getFilter().invert();

      const applyStream = db.createApplyStream();
      currentFilterStream.asReadable().pipeTo(applyStream);

      const log = new module.OperationLog();
      const logApplyStream = log.createApplyStream();

      const teeStream = new module.LogOperationTeeStream(logApplyStream,
        currentFilterStream.asWritable());
      newFilterStream.asReadable().pipeTo(teeStream);

      for (let i = 0; i < projectData.length; i++) {
        deserializer.asWritable().write(projectData[i]);
      }

      deserializer.asWritable().close();
      newFilterStream.asWritable().close();
      teeStream.close();
      currentFilterStream.asWritable().close();
      applyStream.close();
      logApplyStream.close();

      //update the database's vector clock from the applied ops
      db.getVectorClock().setFromOther(log.getVectorClock());

      deserializer.delete();
      newFilterStream.delete();
      teeStream.delete();
      currentFilterStream.delete();
      applyStream.delete();
      logApplyStream.delete();
      log.delete();
    }

    currentFilter.setFromOther(newFilter);
  };

  return {
    db,
    builder,
    eventEmitter,
    valueId,
    applyFilter
  };
};

type TagItem = {
  name: string;
  active: boolean;
  enabled: boolean;
  tag: Tag;
}

export const Tags = () => {
  const [module, setModule] = useState<crdblModule>();
  const [tags, setTags] = useState<TagItem[]>([]);

  const ctx = useMemo(() => {
    if (!module)
      return null;
    setTags([]);
    return createCtx(module, 1);
  }, [module]);

  const createRandomTag = (module: crdblModule) => {
    const tag = module.Tag.fromArray(new Uint32Array([
      Math.random() * 0xFFFFFFFF,
      Math.random() * 0xFFFFFFFF,
      Math.random() * 0xFFFFFFFF,
      Math.random() * 0xFFFFFFFF
    ]));
    return tag;
  };

  const addNewTag = useCallback((name: string) => {
    if (!module)
      return;
    setTags(tags => [...tags,
      { name, active: false, enabled: true, tag: createRandomTag(module) }]);
  }, [module]);

  useEffect(() => {
    init().then((Module) => {
      setModule(module => module ?? Module);
    });
  }, []);

  //initial tags
  useEffect(() => {
    if (!module || !ctx)
      return;

    const initialTags = [
      { name: "Tag 1", active: true, enabled: true, tag: createRandomTag(module) },
      { name: "Tag 2", active: false, enabled: true, tag: createRandomTag(module) },
      { name: "Tag 3", active: false, enabled: true, tag: createRandomTag(module) }
    ];
    setTags(initialTags);

    ctx.builder.setTag(initialTags[0].tag);
    ctx.builder.insertText(ctx.valueId, 0, "This is some text created with tag 1.\n\n");
    ctx.builder.setTag(initialTags[1].tag);
    ctx.builder.insertText(ctx.valueId, ctx.db.getNodeBlockValue(ctx.valueId).length,
      "And this was written with tag 2.\n\n");
    ctx.builder.setTag(initialTags[2].tag);
    ctx.builder.insertText(ctx.valueId, ctx.db.getNodeBlockValue(ctx.valueId).length,
      "This text was created on tag 3.\n\n");
  }, [ctx, module]);

  const setActiveTag = (tag: TagItem) => {
    setTags(tags => tags.map((tagItem) => {
      if (tagItem === tag) {
        tagItem.active = true;
      } else {
        tagItem.active = false;
      }
      return tagItem;
    }));
  };

  const setTagEnabled = (tag: TagItem, enabled: boolean) => {
    if (!ctx)
      return;
    setTags(tags => tags.map((tagItem) => {
      if (tagItem === tag) {
        tagItem.enabled = enabled;
      }
      return tagItem;
    }));
  };

  useEffect(() => {
    if (!ctx || !module)
      return;
    const filter = new module.OperationFilter();

    //if no tags are set, then there will be no tag filtering
    //this just ensures that with no tags selected, all operations are filtered
    const emptyTag = module.Tag.fromArray(new Uint32Array([0, 0, 0, 0]));
    filter.setTagClockRange(emptyTag, null, null);
    emptyTag.delete();

    const builder = ctx.builder;
    tags.forEach((tagItem) => {
      if (tagItem.enabled) {
        filter.setTagClockRange(tagItem.tag, null, null);
      }
      if (tagItem.active) {
        builder.setTag(tagItem.tag);
      }
    });
    //NOTE: we don't need to update the filter in all cases that tags change
    //  but for this simple demo it should be fine
    ctx.applyFilter(filter);
    filter.delete();
  }, [module, ctx, tags]);

  return (
    <div className={styles.container}>
      <div className={styles.editorSplit}>
        <div>
          { ctx &&
            <TextEditor
              builder={ctx.builder}
              db={ctx.db}
              onEvent={ctx.eventEmitter}
              valueId={ctx.valueId}
              siteId={1} /> }
        </div>
        <div className={styles.tagsContainer}>
          <div className={styles.toolbar}>
            <button onClick={() => {
              addNewTag(`Tag ${tags.length + 1}`);
            }}>➕ New</button>
            <button onClick={() => {
              setTags(tags => tags.map((tagItem) => ({ ...tagItem, enabled: true })));
            }}>⬜ Select All</button>
            <button onClick={() => {
              setTags(tags => tags.map((tagItem) => ({ ...tagItem, enabled: false })));
            }}>⬛ Deselect All</button>
          </div>
          <div className={styles.tags}>
            { tags.map((tagItem, idx) => (<div key={idx} className={tagItem.active ? `${styles.active} ${styles.tag}` : styles.tag}>
              <button onClick={() => setActiveTag(tagItem)}>
                { tagItem.active ? <span>🖊</span> : null }
                {tagItem.name}
              </button>
              <input type="checkbox" checked={tagItem.enabled} onChange={(e) => {
                setTagEnabled(tagItem, e.target.checked);
              }} />
            </div>))}
          </div>
        </div>
    </div>
  </div>
  );
};