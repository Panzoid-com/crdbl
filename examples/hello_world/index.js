import init from "@panzoid/crdbl";

async function main() {
  const { ProjectDB } = await init();

  const db = new ProjectDB(() => {},
    (event) => {
      console.log("Event:", event);
    });
  const dbApplyStream = db.createApplyStream();

  const builder = db.createOperationBuilder();
  builder.setSiteId(1);
  builder.getReadableStream().pipeTo(dbApplyStream);

  const rootId = builder.createNode("List");
  console.log("Created root node with id:", rootId);

  let lastEdgeId = null;

  const createValueNode = (value) =>
  {
    const childId = builder.createNode("DoubleValue");
    console.log("Created child node with id:", childId);

    const ts = builder.setValueDouble(childId, value);
    console.log("Set value of child node at timestamp:", ts, "to:", value);

    lastEdgeId = builder.addChild(rootId, childId, builder.createPositionBetweenEdges(lastEdgeId, null));
    console.log("Added child to root node with edge id:", lastEdgeId);
  };

  createValueNode(3.14);

  const createStringNode = (value) =>
  {
    const childId = builder.createNode("StringValue");
    console.log("Created child node with id:", childId);

    builder.insertText(childId, 0, value);
    console.log("Insert text into child node:", value);

    lastEdgeId = builder.addChild(rootId, childId, builder.createPositionBetweenEdges(lastEdgeId, null));
    console.log("Added child to root node with edge id:", lastEdgeId);
  };

  createStringNode("Hello, World!");

  console.log("Root node:", db.getNode(rootId));

  const children = db.getNodeChildren(rootId);
  console.log("Root node children:", children);

  for (const { childId } of children)
  {
    const childNode = db.getNode(childId);
    console.log("  Child node:", childNode);

    if (childNode.type.includes("DoubleValue"))
    {
      console.log("    Value:", db.getNodeValue(childId));
    }
    else if (childNode.type.includes("StringValue"))
    {
      console.log("    Value:", db.getNodeBlockValue(childId));
    }
  }

  builder.delete();
  dbApplyStream.delete();
  db.delete();
}

main();