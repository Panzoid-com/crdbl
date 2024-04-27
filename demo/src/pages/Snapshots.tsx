import init, { DBEvent, OperationFilter, VectorTimestamp, crdblModule } from "@panzoid/crdbl";
import { useCallback, useEffect, useMemo, useState } from "react";
import EventEmitter from "../utility/EventEmitter";
import { TextEditor } from "../components/TextEditor";
import styles from "./Snapshots.module.css";

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
    log,
    builder,
    eventEmitter,
    valueId,
    applyFilter
  };
};

type SnapshotItem = {
  name: string;
  clock: VectorTimestamp;
}

export const Snapshots = () => {
  const [module, setModule] = useState<crdblModule>();
  const [snapshots, setSnapshots] = useState<SnapshotItem[]>([]);

  const ctx = useMemo(() => {
    if (!module)
      return null;
    return createCtx(module, 1);
  }, [module]);

  const createSnapshot = useCallback((name: string) => {
    if (!ctx)
      return;
    const newClock = ctx.log.getVectorClock().clone();
    setSnapshots(snapshots => [...snapshots,
      { name, active: false, enabled: true, clock: newClock }]);
  }, [ctx]);

  useEffect(() => {
    init().then((Module) => {
      setModule(module => module ?? Module);
    });
  }, []);

  //initial tags
  useEffect(() => {
    if (!module || !ctx)
      return;

    setSnapshots([]);

    ctx.builder.insertText(ctx.valueId, 0, "This is some initial text.\n\n");
    createSnapshot("Snapshot 1");
    ctx.builder.insertText(ctx.valueId, ctx.db.getNodeBlockValue(ctx.valueId).length,
      "And this was written before snapshot 2.\n\n");
    createSnapshot("Snapshot 2");
    ctx.builder.insertText(ctx.valueId, ctx.db.getNodeBlockValue(ctx.valueId).length,
      "This text was created before snapshot 3.\n\n");
    createSnapshot("Snapshot 3");
  }, [createSnapshot, ctx, module]);

  const [currentSnapshot, setCurrentSnapshot] = useState<SnapshotItem | null>(null);

  useEffect(() => {
    if (!ctx || !module)
      return;
    const filter = new module.OperationFilter();
    if (currentSnapshot)
    {
      filter.setClockRange(null, currentSnapshot.clock);
    }
    ctx.applyFilter(filter);
    filter.delete();
  }, [module, ctx, currentSnapshot]);

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
              createSnapshot(`Snapshot ${snapshots.length + 1}`);
            }}>➕ New</button>
          </div>
          <div className={styles.tags}>
            { snapshots.map((snapshotItem, idx) =>
              (<div key={idx} className={snapshotItem === currentSnapshot ? `${styles.active} ${styles.tag}` : styles.tag}>
                <button onClick={() => setCurrentSnapshot(snapshotItem)}>
                  { snapshotItem === currentSnapshot ? <span>👁</span> : null }
                  {snapshotItem.name}
                </button>
              </div>))}
            <div className={!currentSnapshot ? `${styles.active} ${styles.tag}` : styles.tag}>
              <button onClick={() => setCurrentSnapshot(null)}>
                { !currentSnapshot ? <span>🖊</span> : null }
                Current version
              </button>
            </div>
          </div>
        </div>
    </div>
  </div>
  );
};