const MAX_DEPTH = 1000;
const NULL_ID = "0,0,0"; //NOTE: use ProjectDB.NULLID() instead

export function nodeToObject(db, nodeId, depth = 0)
{
  if (depth > MAX_DEPTH)
    return;

  const node = db.getNode(nodeId);

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
    return db.getNodeBlockValue(nodeId);
  }
  else if (baseType === "BoolValue")
  {
    return !!db.getNodeValue(nodeId);
  }
  else
  {
    return db.getNodeValue(nodeId);
  }

  const children = db.getNodeChildren(nodeId);
  for (let i = 0; i < children.length; i++)
  {
    const key = children[i].key || i;
    obj[key] = nodeToObject(db, children[i].childId, depth + 1);
  }

  return obj;
}

export function objectToNode(opBuilder, data, depth = 0)
{
  if (depth > MAX_DEPTH)
    return;

  let nodeId;

  if (Array.isArray(data))
  {
    nodeId = opBuilder.createNode("List");

    let prevEdgeId = null;
    for (let i = 0; i < data.length; i++)
    {
      const childId = objectToNode(opBuilder, data[i], depth + 1);
      prevEdgeId = opBuilder.addChild(nodeId, childId,
        opBuilder.createPositionBetweenEdges(prevEdgeId, null));
    }
  }
  else if (typeof data === "object")
  {
    if (data === null)
    {
      return NULL_ID;
    }

    nodeId = opBuilder.createNode("Map");

    const keys = Object.keys(data);
    for (let i = 0; i < keys.length; i++)
    {
      const childId = objectToNode(opBuilder, data[keys[i]], depth + 1);
      if (childId)
      {
        opBuilder.addChild(nodeId, childId, opBuilder.createKey(keys[i]));
      }
    }
  }
  else if (typeof data === "string")
  {
    nodeId = opBuilder.createNode("StringValue");

    if (data.length)
    {
      opBuilder.insertText(nodeId, 0, data);
    }
  }
  else if (typeof data === "number")
  {
    nodeId = opBuilder.createNode("DoubleValue");

    opBuilder.setValueDouble(nodeId, data);
  }
  else if (typeof data === "boolean")
  {
    nodeId = opBuilder.createNode("BoolValue");

    opBuilder.setValueBool(nodeId, data);
  }

  return nodeId;
}

export function nodeToJson(db, nodeId)
{
  return JSON.stringify(nodeToObject(db, nodeId));
}

export function jsonToNode(opBuilder, json)
{
  return objectToNode(opBuilder, JSON.parse(json));
}