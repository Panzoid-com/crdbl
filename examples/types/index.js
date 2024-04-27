import init from "@panzoid/crdbl";
import { objectToNode, nodeToObject } from "../json/json.js";

/**
 * @param {import("@panzoid/crdbl")} Module
 * @param {any} data
 */
function createType(Module, data) {
  const {
    ProjectDB,
    LogOperationSerialization,
    DataCallbackStream
  } = Module;

  const db = new ProjectDB(() => {}, () => {});
  const builder = db.createOperationBuilder();
  const dbApplyStream = db.createApplyStream();
  builder.getReadableStream().pipeTo(dbApplyStream);

  const rootId = objectToNode(builder, data);

  const logGenerator = db.createTypeLogGenerator();
  logGenerator.addAllNodes(rootId);

  const serializer = LogOperationSerialization.CreateSerializer(
    LogOperationSerialization.DefaultTypeFormat());
  const typeDataChunks = [];
  const callbackStream = new DataCallbackStream(
    (data) => {
      typeDataChunks.push(new Uint8Array(data, data.byteOffset, data.byteLength));
    },
    () => {});
  serializer.asReadable().pipeTo(callbackStream);

  logGenerator.generate(serializer.asWritable());

  callbackStream.close();
  callbackStream.delete();
  serializer.delete();
  logGenerator.delete();
  dbApplyStream.delete();
  builder.delete();
  db.delete();

  return new Uint8Array(Buffer.concat(typeDataChunks));
}

async function main() {
  const Module = await init();
  const { ProjectDB } = Module;

  const userType = createType(Module, {
    name: "John Doe",
    age: 42,
    email: "",
    products: []
  });

  const productType = createType(Module, {
    name: "Product",
    description: "",
    price: 0,
    quantity: 0
  });

  const nodeWaitingCallbacks = new Map();
  const waitForNode = async (nodeId) => {
    if (db.getNode(nodeId)?.ready)
    {
      return;
    }

    return new Promise((resolve) => {
      const callbacks = nodeWaitingCallbacks.get(nodeId) || [];
      callbacks.push(resolve);
      nodeWaitingCallbacks.set(nodeId, callbacks);
    });
  };

  const db = new ProjectDB(
    (type) => {
      console.log("Type data requested:", type);

      const format = Module.LogOperationSerialization.DefaultTypeFormat();
      if (type === "User")
      {
        ProjectDB.addType("User", format, userType);
      }
      else if (type === "Product")
      {
        ProjectDB.addType("Product", format, productType);
      }

      //NOTE: resolveTypeSpec must be called and it must be called after this
      //  function returns
      setImmediate(() => {
        db.resolveTypeSpec(type);
      });
    },
    (event) => {
      console.log("Event:", event);

      if (event.eventType === "NodeAdded" && !event.speculative)
      {
        const { childId } = event;
        const callbacks = nodeWaitingCallbacks.get(childId) || [];
        for (const callback of callbacks)
        {
          callback();
        }
        nodeWaitingCallbacks.delete(childId);
      }
    });
  const dbApplyStream = db.createApplyStream();

  const builder = db.createOperationBuilder();
  builder.setSiteId(1);
  builder.getReadableStream().pipeTo(dbApplyStream);

  const rootId = builder.createNode("User");

  await waitForNode(rootId);

  const children = db.getNodeChildren(rootId);
  const productsListId = children.find(({ key }) => key === "products").childId;

  let lastEdgeId = null;
  let readyPromises = [];
  for (let i = 0; i < 5; i++)
  {
    const productNodeId = builder.createNode("Product");
    lastEdgeId = builder.addChild(productsListId, productNodeId,
      builder.createPositionBetweenEdges(lastEdgeId, null));
    readyPromises.push(waitForNode(productNodeId));
  }

  await Promise.all(readyPromises);

  console.log("Root node:", nodeToObject(db, rootId));

  builder.delete();
  dbApplyStream.delete();
  db.delete();
}

main();