import init from "@panzoid/crdbl";

class Replica
{
  /**
   * @param {import("@panzoid/crdbl")} Module
   * @param {number} siteId
   */
  constructor(Module, siteId)
  {
    const {
      ProjectDB,
      OperationLog,
      FilterOperationStream,
      LogOperationSerialization,
      DeserializeDirection,
      DataCallbackStream
    } = Module;

    this.LogOperationSerialization = LogOperationSerialization;
    this.DeserializeDirection = DeserializeDirection;

    this.siteId = siteId;

    this.db = new ProjectDB(() => {}, () => {});
    this.dbApplyStream = this.db.createApplyStream();

    this.opLog = new OperationLog();

    // log -> db
    this.logApplyStream = this.opLog.createApplyStream();
    const logDbReadStream = this.opLog.createReadStream();
    logDbReadStream.pipeTo(this.dbApplyStream);

    // builder -> log
    this.builder = this.db.createOperationBuilder();
    this.builder.setSiteId(siteId);
    this.builder.getReadableStream().pipeTo(this.logApplyStream);

    // log -> filter -> serializer
    const logReadStream = this.opLog.createReadStream();

    // we insert a filter so that we only serialize and "send" operations from
    //  this replica
    const onlyMyOpsFilter = new FilterOperationStream();
    onlyMyOpsFilter.getFilter().setSiteFilter(siteId);
    logReadStream.pipeTo(onlyMyOpsFilter.asWritable());

    this.serializer = LogOperationSerialization.CreateSerializer(
      LogOperationSerialization.DefaultFormat());
    onlyMyOpsFilter.asReadable().pipeTo(this.serializer.asWritable());

    // serializer -> buffer
    this.buffer = [];

    const writeCallbackStream = new DataCallbackStream(
      (data) => {
        //we slice the data since it is a view into the heap, not a copy,
        //  and after this call returns, the data is no longer guaranteed to be valid
        this.buffer.push(data.slice());
      },
      () => {});
    this.serializer.asReadable().pipeTo(writeCallbackStream);

    if (siteId === 1) {
      this.rootId = this.builder.createNode("List");
    } else {
      this.rootId = ProjectDB.INITID();
    }
  }

  receiveData(data)
  {
    const deserializer = this.LogOperationSerialization.CreateDeserializer(
      this.LogOperationSerialization.DefaultFormat(), this.DeserializeDirection.Forward);
    deserializer.asReadable().pipeTo(this.logApplyStream);

    for (const chunk of data) {
      deserializer.asWritable().write(chunk);
    }

    deserializer.asWritable().close();
    deserializer.delete();
  }

  getOutgoingData()
  {
    const buffer = this.buffer;
    this.buffer = [];
    return buffer;
  }

  insertRandomItems(count, depth, site) {
    const numIndices = Math.max(this.db.getNodeChildren(this.rootId).length, count);
    const indices = new Array(numIndices).fill(0)
      .map((_, i) => i)
      .sort(() => Math.random() - 0.5)
      .slice(0, count)
      .sort((a, b) => a - b);

    for (let i = 0; i < count; i++) {
      const newItemStr = String.fromCharCode(97 + (i % 26));

      const children = this.db.getNodeChildren(this.rootId);
      const idx = Math.min(indices[i] + i, children.length);
      const prevEdgeId = idx === 0 ? null : children[idx - 1].edgeId;
      const nextEdgeId = idx === children.length ? null : children[idx].edgeId;

      const newItem = this.builder.createNode("StringValue");
      this.builder.insertText(newItem, 0, "  ".repeat(depth) + site + newItemStr);
      this.builder.addChild(this.rootId, newItem, this.builder.createPositionBetweenEdges(prevEdgeId, nextEdgeId));
    }
  }

  getList() {
    const children = this.db.getNodeChildren(this.rootId);
    return children.map(({ childId }) => this.db.getNodeBlockValue(childId));
  }

  dispose() {
    this.dbApplyStream.delete();
    this.db.delete();
    this.opLog.delete();
    this.logApplyStream.delete();
    this.serializer.delete();
    this.builder.delete();
  }
}

function printSideBySide(list1, list2) {
  const maxLength = Math.max(list1.length, list2.length);
  const columnWidth = 20;

  console.log('Replica 1'.padEnd(columnWidth) + ' | Replica 2');
  console.log("-".repeat(columnWidth) + '-|-' + "-".repeat(columnWidth));
  for (let i = 0; i < maxLength; i++) {
    const item1 = list1[i] || '';
    const item2 = list2[i] || '';
    console.log(`${item1.toString().padEnd(columnWidth)} | ${item2}`);
  }
}

async function main() {
  const Module = await init();

  const replica1 = new Replica(Module, 1);
  const replica2 = new Replica(Module, 2);

  // merge the root node (created on replica1)
  replica2.receiveData(replica1.getOutgoingData());

  const numIterations = 3;
  for (let i = 0; i < numIterations; i++) {
    replica1.insertRandomItems(3, i, 1);
    replica2.insertRandomItems(3, i, 2);

    console.log(`Pre-merge states:`);
    printSideBySide(replica1.getList(), replica2.getList());

    replica1.receiveData(replica2.getOutgoingData());
    replica2.receiveData(replica1.getOutgoingData());

    console.log("Merge result:");
    printSideBySide(replica1.getList(), replica2.getList());
  }

  replica1.dispose();
  replica2.dispose();
}

main();