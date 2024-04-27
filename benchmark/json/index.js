import init from "@panzoid/crdbl";
import { benchmark, profile, printResults } from "../benchmark.js";
import { generateRandomData } from "./generateRandomData.js";

const MAX_DEPTH = 20;

function nodeToObject(db, nodeId, depth = 0)
{
  if (depth > MAX_DEPTH)
    return;

  const node = profile("getNode", () => db.getNode(nodeId));

  let obj;
  const baseType = node.type[node.type.length - 1];

  if (baseType === "Map")
  {
    obj = {};
  }
  else if (baseType === "List" || baseType === "Set")
  {
    obj = [];
  }
  else if (baseType === "StringValue")
  {
    return profile("getNodeBlockValue", () => db.getNodeBlockValue(nodeId));
  }
  else if (baseType === "BoolValue")
  {
    return profile("getNodeValue", () => !!db.getNodeValue(nodeId));
  }
  else
  {
    return profile("getNodeValue", () => db.getNodeValue(nodeId));
  }

  const children = profile("getNodeChildren", () => db.getNodeChildren(nodeId));
  db.getNodeChildren(nodeId);
  for (let i = 0; i < children.length; i++)
  {
    const key = children[i].key || i;
    obj[key] = nodeToObject(db, children[i].childId, depth + 1);
  }

  return obj;
}

function objectToNode(opBuilder, data, depth = 0)
{
  if (depth > MAX_DEPTH)
    return;

  let nodeId;

  if (Array.isArray(data))
  {
    profile("createNode", () => {
      nodeId = opBuilder.createNode("List");
    });

    let prevEdgeId = null;
    for (let i = 0; i < data.length; i++)
    {
      const childId = objectToNode(opBuilder, data[i], depth + 1);
      profile("addListChild", () => {
        prevEdgeId = opBuilder.addChild(nodeId, childId,
          opBuilder.createPositionBetweenEdges(prevEdgeId, null));
      });
    }
  }
  else if (typeof data === "object")
  {
    if (data === null)
    {
      return NULL_ID;
    }

    profile("createNode", () => {
      nodeId = opBuilder.createNode("Map");
    });

    const keys = Object.keys(data);
    for (let i = 0; i < keys.length; i++)
    {
      const childId = objectToNode(opBuilder, data[keys[i]], depth + 1);
      if (childId)
      {
        profile("addMapChild", () => {
          opBuilder.addChild(nodeId, childId, opBuilder.createKey(keys[i]));
        });
      }
    }
  }
  else if (typeof data === "string")
  {
    profile("createNode", () => {
      nodeId = opBuilder.createNode("StringValue");
    });

    if (data.length)
    {
      profile("insertText", () => {
        opBuilder.insertText(nodeId, 0, data);
      });
    }
  }
  else if (typeof data === "number")
  {
    profile("createNode", () => {
      nodeId = opBuilder.createNode("DoubleValue");
    });

    profile("setValueDouble", () => {
      opBuilder.setValueDouble(nodeId, data);
    });
  }
  else if (typeof data === "boolean")
  {
    profile("createNode", () => {
      nodeId = opBuilder.createNode("BoolValue");
    });

    profile("setValueBool", () => {
      opBuilder.setValueBool(nodeId, data);
    });
  }

  return nodeId;
}

async function main() {
  const Module = {};
  const { ProjectDB } = await init(Module);

  const db = new ProjectDB(() => {}, () => {});
  const dbApplyStream = db.createApplyStream();

  const builder = db.createOperationBuilder();
  builder.setSiteId(1);
  builder.getReadableStream().pipeTo(dbApplyStream);

  // Optional: fetch some real data instead
  // const jsonUrl = ``;
  // const response = await fetch(jsonUrl);
  // const data = await response.json();

  const data = generateRandomData(50000);

  const results = benchmark(() => {
    const nodeId = profile("objectToNode", () => objectToNode(builder, data));
    profile("nodeToObject", () => nodeToObject(db, nodeId));
  }, 10);
  printResults(results);

  console.log("Final heap size:", Module.HEAP8.length);
}

main();