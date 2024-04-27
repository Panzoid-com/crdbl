import init, { DBEvent, crdblModule } from "@panzoid/crdbl";
import { useCallback, useEffect, useMemo, useState } from "react";
import EventEmitter from "../utility/EventEmitter";
import { TextEditor } from "../components/TextEditor";
import styles from "./Sync.module.css";

const createCtx = (module: crdblModule, siteId: number) => {
  const onEvent = new EventEmitter<[DBEvent]>();
  const db = new module.ProjectDB(() => {}, (event) =>
    {
      onEvent.emit(event);
    });
  const log = new module.OperationLog();
  const logApplyStream = log.createApplyStream();

  const builder = db.createOperationBuilder();
  builder.setSiteId(siteId);
  builder.getReadableStream().pipeTo(logApplyStream);

  const dbApplyStream = db.createApplyStream();
  log.createReadStream().pipeTo(dbApplyStream);

  const onHasPending = new EventEmitter<[]>();

  let unsyncedQueue: Uint8Array[] = [];
  const serializer = module.LogOperationSerialization.CreateSerializer(
    module.LogOperationSerialization.DefaultFormat());
  const dataStream = new module.DataCallbackStream(
    (data) => {
      unsyncedQueue.push(data.slice());
      onHasPending.emit();
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

  const getUnsynced = () => {
    const unsynced = unsyncedQueue;
    unsyncedQueue = [];
    return unsynced;
  };

  const receiveOps = (ops: Uint8Array[]) => {
    for (const op of ops) {
      deserializer.asWritable().write(op);
    }
  };

  const valueId = (siteId === 1)
    ? builder.createNode("StringValue")
    : module.ProjectDB.INITID();

  return {
    db,
    builder,
    onEvent,
    onHasPending,
    valueId,
    getUnsynced,
    receiveOps
  };
};

export const Sync = () => {
  const [module, setModule] = useState<crdblModule>();

  const ctx1 = useMemo(() => {
    if (!module)
      return null;
    return createCtx(module, 1);
  }, [module]);

  const ctx2 = useMemo(() => {
    if (!module)
      return null;
    return createCtx(module, 2);
  }, [module]);

  const [pendingOpsCount1, setPendingOpsCount1] = useState(0);
  const [pendingOpsCount2, setPendingOpsCount2] = useState(0);

  const syncRight = useCallback(() => {
    if (!ctx1 || !ctx2)
      return;

    const unsynced = ctx1.getUnsynced();
    ctx2.receiveOps(unsynced);
    setPendingOpsCount1(0);
  }, [ctx1, ctx2]);

  const syncLeft = useCallback(() => {
    if (!ctx1 || !ctx2)
      return;

    const unsynced = ctx2.getUnsynced();
    ctx1.receiveOps(unsynced);
    setPendingOpsCount2(0);
  }, [ctx1, ctx2]);

  useEffect(() => {
    init().then((Module) => {
      setModule(module => module ?? Module);
    });
  }, []);

  const [autoSync1, setAutoSync1] = useState(false);
  const [autoSync2, setAutoSync2] = useState(false);

  useEffect(() => {
    if (!ctx1 || !ctx2)
      return;

    const handleEvent1 = () => {
      setPendingOpsCount1(count => count + 1);

      if (autoSync1) {
        syncRight();
      }
    };
    ctx1.onHasPending.watch(handleEvent1);

    const handleEvent2 = () => {
      setPendingOpsCount2(count => count + 1);

      if (autoSync2) {
        syncLeft();
      }
    };
    ctx2.onHasPending.watch(handleEvent2);

    return () => {
      ctx1.onHasPending.unwatch(handleEvent1);
      ctx2.onHasPending.unwatch(handleEvent2);
    };
  }, [autoSync1, autoSync2, ctx1, ctx2, syncLeft, syncRight]);

  //initial sync for the common string node and initial text
  useEffect(() => {
    setPendingOpsCount1(0);
    setPendingOpsCount2(0);

    if (ctx1 && ctx2) {
      syncRight();
      ctx1.builder.insertText(ctx1.valueId, 0,
        "This is some text at replica 1.\n\n");
      ctx2.builder.insertText(ctx2.valueId, 0,
        "And here is some text at replica 2.\n");
    }

  }, [ctx1, ctx2, syncRight]);

  return (
    <div className={styles.container}>
      <div className={styles.toolbar}>
        <span className={styles.pendingOps} style={{ fontWeight: pendingOpsCount1 > 0 ? "bold" : "normal" }}>
          <span>{pendingOpsCount1}</span>
        </span>
        <button onClick={() => {
          setAutoSync1(!autoSync1);
          syncRight();
        }}>
          {autoSync1 ? <span>🔵</span> : <span>⚪</span>}
          Auto
        </button>

        <button onClick={() => syncRight()}>Sync ➡</button>
        <button onClick={() => { syncLeft(); syncRight(); }}>🔃 Sync Both</button>
        <button onClick={() => syncLeft()}>⬅ Sync</button>

        <button onClick={() => {
          setAutoSync2(!autoSync2);
          syncLeft();
        }}>
          {autoSync2 ? <span>🔵</span> : <span>⚪</span>}
          Auto
        </button>

        <span className={styles.pendingOps} style={{ fontWeight: pendingOpsCount2 > 0 ? "bold" : "normal" }}>
          <span>{pendingOpsCount2}</span>
        </span>
      </div>
      <div className={styles.editorSplit}>
        <div>
          { ctx1 &&
            <TextEditor
              builder={ctx1.builder}
              db={ctx1.db}
              onEvent={ctx1.onEvent}
              valueId={ctx1.valueId}
              siteId={1} /> }
        </div>
        <div>
          { ctx2 &&
            <TextEditor
              builder={ctx2.builder}
              db={ctx2.db}
              onEvent={ctx2.onEvent}
              valueId={ctx2.valueId}
              siteId={2} /> }
      </div>
    </div>
  </div>
  );
};