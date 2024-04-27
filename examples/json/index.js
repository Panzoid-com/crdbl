import init from "@panzoid/crdbl";
import { objectToNode, nodeToObject } from "./json.js";
import { data } from "./example_data.js";

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

  const rootId = objectToNode(builder, data);

  console.log("Root node:", nodeToObject(db, rootId));

  builder.delete();
  dbApplyStream.delete();
  db.delete();
}

main();