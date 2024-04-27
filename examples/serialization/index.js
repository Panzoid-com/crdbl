import init from "@panzoid/crdbl";
import { objectToNode, nodeToObject } from "../json/json.js";
import { data } from "../json/example_data.js";

async function main() {
  const {
    ProjectDB,
    OperationLog,
    WriteFilterOperationStream,
    LogOperationSerialization,
    DeserializeDirection,
    DataCallbackStream
  } = await init();

  const db = new ProjectDB(() => {}, () => {});
  const dbApplyStream = db.createApplyStream();

  const opLog = new OperationLog();

  // log -> db
  const logApplyStream = opLog.createApplyStream();
  const logDbReadStream = opLog.createReadStream();
  logDbReadStream.pipeTo(dbApplyStream);

  // builder -> log
  const builder = db.createOperationBuilder();
  builder.setSiteId(1);
  builder.getReadableStream().pipeTo(logApplyStream);

  // log -> filter -> serializer
  const logReadStream = opLog.createReadStream();
  // NOTE: a write filter does nothing in this example, but it is good practice
  //  to use when e.g. writing to a file, since it filters operations that
  //  should not be written to a file (e.g. realtime preview operations)
  const writeFilterStream = new WriteFilterOperationStream();
  logReadStream.pipeTo(writeFilterStream.asWritable());

  const serializer = LogOperationSerialization.CreateSerializer(
    LogOperationSerialization.DefaultFormat());
  writeFilterStream.asReadable().pipeTo(serializer.asWritable());

  // deserializer -> second db
  const secondDb = new ProjectDB(() => {}, () => {});
  const secondDbApplyStream = secondDb.createApplyStream();

  const deserializer = LogOperationSerialization.CreateDeserializer(
    LogOperationSerialization.DefaultFormat(), DeserializeDirection.Forward);
  deserializer.asReadable().pipeTo(secondDbApplyStream);

  // serializer -> callback -> deserializer
  const writeCallbackStream = new DataCallbackStream(
    (data) => {
      console.log("Serialized data:", Buffer.from(data).toString("base64"));
      deserializer.asWritable().write(data);
    },
    () => {});
  serializer.asReadable().pipeTo(writeCallbackStream);

  // apply some changes to the first db
  const rootId = objectToNode(builder, data);

  deserializer.asWritable().close();
  secondDbApplyStream.close();

  // show that second db has received the changes
  console.log("Root node:", nodeToObject(secondDb, rootId));

  builder.delete();
  dbApplyStream.delete();
  db.delete();
  opLog.delete();
  logApplyStream.delete();
  writeFilterStream.delete();
  serializer.delete();
  deserializer.delete();
  secondDb.delete();
  secondDbApplyStream.delete();
  writeCallbackStream.delete();
}

main();