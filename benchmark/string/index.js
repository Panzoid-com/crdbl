import init from "@panzoid/crdbl";
import { profile, printResults, benchmark } from "../benchmark.js";
import { random } from "../random.js";

/**
 * @param {import("@panzoid/crdbl").ProjectDB} db
 * @param {import("@panzoid/crdbl").OperationBuilder} opBuilder
 * @param {string} stringNodeId
 * @param {number} numEdits
 */
function applyEdits(db, opBuilder, stringNodeId, numEdits = 1000)
{
  const editProbabilities = {
    insertAtEnd: 0.9,
    deleteAtEnd: 0.1,
    randomInsert: 0.1,
    randomDelete: 0.1,
    getValue: 0.2
  };
  const probSum = Object.values(editProbabilities).reduce((a, b) => a + b);
  const probList = Object.values(editProbabilities).map((prob) => prob / probSum);
  const probBoundaries = probList.map((prob, i) => prob + probList.slice(0, i).reduce((a, b) => a + b, 0));
  probBoundaries[probBoundaries.length - 1] = 1;
  const editTypes = Object.keys(editProbabilities);

  let currentStr = "";
  for (let i = 0; i < numEdits; i++)
  {
    const rand = random();
    let editType;
    for (let j = 0; j < probBoundaries.length; j++)
    {
      if (rand < probBoundaries[j])
      {
        editType = editTypes[j];
        break;
      }
    }

    switch (editType)
    {
      case "insertAtEnd":
      {
        const length = Math.floor(random() * 10) + 1;
        const text = "a".repeat(length);
        profile("insertAtEnd", () => {
          opBuilder.insertText(stringNodeId, currentStr.length, text);
        });
        currentStr += text;
        break;
      }
      case "deleteAtEnd":
      {
        const deleteLength = Math.floor(random() * Math.min(10, currentStr.length)) + 1;
        profile("deleteAtEnd", () => {
          opBuilder.deleteText(stringNodeId, currentStr.length - deleteLength, deleteLength);
        });
        currentStr = currentStr.slice(0, -deleteLength);
        break;
      }
      case "randomInsert":
      {
        const insertIdx = Math.floor(random() * currentStr.length);
        const insertLength = Math.floor(random() * 10) + 1;
        const insertText = "b".repeat(insertLength);
        profile("randomInsert", () => {
          opBuilder.insertText(stringNodeId, insertIdx, insertText);
        });
        currentStr = currentStr.slice(0, insertIdx) + insertText + currentStr.slice(insertIdx);
        break;
      }
      case "randomDelete":
      {
        const deleteIdx = Math.floor(random() * currentStr.length);
        const deleteLength = Math.floor(random() * Math.min(10, currentStr.length - deleteIdx)) + 1;
        profile("randomDelete", () => {
          opBuilder.deleteText(stringNodeId, deleteIdx, deleteLength);
        });
        currentStr = currentStr.slice(0, deleteIdx) + currentStr.slice(deleteIdx + deleteLength);
        break;
      }
      case "getValue":
      {
        let dbStr;
        profile("getValue", () => {
          dbStr = db.getNodeBlockValue(stringNodeId);
        });
        if (currentStr !== dbStr)
        {
          throw new Error("String value mismatch");
        }
        break;
      }
    }
  }
}

async function main() {
  const Module = {};
  const { ProjectDB } = await init(Module);

  const db = new ProjectDB(() => {}, () => {});
  const dbApplyStream = db.createApplyStream();

  const builder = db.createOperationBuilder();
  builder.setSiteId(1);
  builder.getReadableStream().pipeTo(dbApplyStream);

  const results = benchmark(() => {
    const stringNodeId = builder.createNode("StringValue");
    applyEdits(db, builder, stringNodeId, 10000);
  }, 10);
  printResults(results);

  console.log("Final heap size:", Module.HEAP8.length);
}

main();