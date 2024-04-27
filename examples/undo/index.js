import init from "@panzoid/crdbl";

async function main() {
  const {
    ProjectDB,
    OperationLog,
    UndoFilterOperationStream,
    LogOperationSerialization,
    DeserializeDirection,
    DataCallbackStream
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

  const undo = () => {
    const deserializer = LogOperationSerialization.CreateDeserializer(
      LogOperationSerialization.DefaultFormat(), DeserializeDirection.Reverse);
    const undoFilterStream = new UndoFilterOperationStream(1);
    deserializer.asReadable().pipeTo(undoFilterStream.asWritable());
    const undoStream = builder.createUndoStream();
    undoFilterStream.asReadable().pipeTo(undoStream);

    for (let i = projectData.length - 1; i >= 0; i--) {
      deserializer.asWritable().write(projectData[i]);
    }

    deserializer.asWritable().close();
    undoFilterStream.asWritable().close();
    undoStream.close();

    deserializer.delete();
    undoFilterStream.delete();
    undoStream.delete();
  }

  const iterations = 3;
  for (let i = 0; i < iterations; i++) {
    builder.startGroup();
    insertRandomItems(3, i);
    builder.commitGroup();
    console.log(`List after edit ${i + 1}:`, getList());
  }

  for (let i = 0; i < iterations; i++) {
    undo();
    console.log(`List after undo ${i + 1}:`, getList());
  }

  builder.delete();
  dbApplyStream.delete();
  db.delete();

  log.delete();
  logApplyStream.delete();
  serializer.delete();
}

main();