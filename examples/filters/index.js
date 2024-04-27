import init from "@panzoid/crdbl";

async function main() {
  const {
    ProjectDB,
    OperationLog,
    Tag,
    LogOperationSerialization,
    LogOperationTeeStream,
    DeserializeDirection,
    DataCallbackStream,
    OperationFilter,
    FilterOperationStream
  } = await init();

  const db = new ProjectDB(() => {}, () => {});
  const dbApplyStream = db.createApplyStream();

  const log = new OperationLog();
  const logApplyStream = log.createApplyStream();
  log.createReadStream().pipeTo(dbApplyStream);

  const projectData = [];

  // serialize and "store" applied ops to project data array
  const serializer = LogOperationSerialization.CreateSerializer(
    LogOperationSerialization.DefaultFormat());
  const logDbReadStream = log.createReadStream();
  logDbReadStream.pipeTo(serializer.asWritable());
  const dataCallbackStream = new DataCallbackStream(
    (data) => {
      projectData.push(data.slice());
    },
    () => {});
  serializer.asReadable().pipeTo(dataCallbackStream);

  const builder = db.createOperationBuilder();
  builder.setSiteId(1);
  builder.getReadableStream().pipeTo(logApplyStream);

  const rootId = builder.createNode("List");

  const insertRandomItems = (count, depth) => {
    const childrenEdges = db.getNodeChildren(rootId).map(({ edgeId }) => edgeId);
    const numIndices = Math.max(childrenEdges.length, count);
    const indices = new Array(numIndices).fill(0)
      .map((_, i) => i)
      .sort(() => Math.random() - 0.5)
      .slice(0, count)
      .sort((a, b) => a - b);

    for (let i = 0; i < count; i++) {
      const newItemStr = String.fromCharCode(97 + (i % 26));

      const idx = Math.min(indices[i] + i, childrenEdges.length);
      const prevEdgeId = idx === 0 ? null : childrenEdges[idx - 1];
      const nextEdgeId = idx === childrenEdges.length ? null : childrenEdges[idx];

      const newItem = builder.createNode("StringValue");
      builder.insertTextAtFront(newItem, String(depth + 1) + newItemStr);
      const edgeId = builder.addChild(rootId, newItem,
        builder.createPositionBetweenEdges(prevEdgeId, nextEdgeId));

      childrenEdges.splice(idx, 0, edgeId);
    }
  };

  const getList = () => {
    const children = db.getNodeChildren(rootId);
    return children.map(({ childId }) => db.getNodeBlockValue(childId));
  };

  const currentFilter = new OperationFilter();

  const applyFilter = (newFilter) => {

    //reverse pass
    //unapply operations from the db that are not included in the new filter
    //for now, operations always must be unapplied in reverse order
    {
      const deserializer = LogOperationSerialization.CreateDeserializer(
        LogOperationSerialization.DefaultFormat(),
        DeserializeDirection.Reverse);

      const newFilterStream = new FilterOperationStream();
      newFilterStream.getFilter().setFromOther(newFilter);
      newFilterStream.getFilter().invert();
      deserializer.asReadable().pipeTo(newFilterStream.asWritable());

      const currentFilterStream = new FilterOperationStream();
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
      const deserializer = LogOperationSerialization.CreateDeserializer(
        LogOperationSerialization.DefaultFormat(), DeserializeDirection.Forward);

      const newFilterStream = new FilterOperationStream();
      newFilterStream.getFilter().setFromOther(newFilter);
      deserializer.asReadable().pipeTo(newFilterStream.asWritable());

      const currentFilterStream = new FilterOperationStream();
      currentFilterStream.getFilter().setFromOther(currentFilter);
      currentFilterStream.getFilter().invert();

      const applyStream = db.createApplyStream();
      currentFilterStream.asReadable().pipeTo(applyStream);

      const log = new OperationLog();
      const logApplyStream = log.createApplyStream();

      const teeStream = new LogOperationTeeStream(logApplyStream,
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

  const defaultTag = new Tag();
  const tags = [
    Tag.fromArray(new Uint32Array([1, 2, 3, 4])),
    Tag.fromArray(new Uint32Array([5, 6, 7, 8])),
    Tag.fromArray(new Uint32Array([9, 10, 11, 12]))
  ];

  const iterations = 3;
  for (let i = 0; i < iterations; i++) {
    builder.setTag(tags[i % tags.length]);
    insertRandomItems(3, i);
    console.log(`List after iteration ${i + 1}:`, getList());
  }

  for (let i = 0; i < iterations; i++) {
    const filter = new OperationFilter();
    filter.setTagClockRange(defaultTag, null, null); //include untagged ops
    filter.setTagClockRange(tags[i % tags.length], null, null);
    applyFilter(filter);
    filter.delete();

    console.log(`List filtered by tag ${i + 1}:`, getList());
  }

  builder.delete();
  dbApplyStream.delete();
  db.delete();

  log.delete();
  logApplyStream.delete();
  serializer.delete();
}

main();