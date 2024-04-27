type LogOperation = {};
type NodeId = string;
type EdgeId = string;

type PrimitiveType =
  "Abstract" |
  "Null" |
  "Set" |
  "List" |
  "Map" |
  "OrderedFloat64Map" |
  "Reference" |
  "Int8Value" |
  "Int32Value" |
  "Int64Value" |
  "FloatValue" |
  "DoubleValue" |
  "BoolValue" |
  "StringValue";

type NodeType = PrimitiveType | string;

type NodeData =
{
  ready?: true;
  type: NodeType[];
  childType?: NodeType;
  nullable?: boolean;
};

type NodeChildrenData =
{
  edgeId: EdgeId;
  childId: NodeId;
  speculative?: true;
  key?: string | number; //can be a float (for now)
}[];

type DBEvent = any;
// {
//   eventType: "NodeDeleted";
//   nodeId: NodeId;
//   speculative: never;
// } | {
//   eventType: "NodeAdded";
//   parentId: NodeId;
//   childId: NodeId;
//   edgeId: EdgeId;
//   speculative: boolean;
//   index?: number;
//   actualIndex?: number;
//   key?: string | number; //can be a float (for now)
// } | {
//   eventType: "NodeRemoved";
//   parentId: NodeId;
//   childId: NodeId;
//   edgeId: EdgeId;
//   speculative: boolean;
//   index?: number;
//   actualIndex?: number;
//   key?: string | number; //can be a float (for now)
// } | {
//   eventType: "NodeValueChanged";
//   id: NodeId;
//   value: number;
//   oldValue: number;
//   speculative: never;
// } | {
//   eventType: "NodeBlockValueInserted";
//   id: NodeId;
//   offset: number;
//   value: string;
//   speculative: never;
// } | {
//   eventType: "NodeBlockValueDeleted";
//   id: NodeId;
//   offset: number;
//   length: number;
//   speculative: never;
// };

export declare enum DeserializeDirection
{
  Forward = "Forward",
  Reverse = "Reverse"
}

export type Timestamp = [number, number];

interface IEmbindClassHandle
{
  delete(): void;
}

declare class EmbindClassHandle implements IEmbindClassHandle
{
  delete(): void;
}

export type ManagedRef<T> = Omit<T, 'delete'>
export type Ref<T> = T | ManagedRef<T>;

interface IWritableStream<T>
{
  write(data: T): boolean;
  close(): void;
}

interface IReadableStream<T>
{
  pipeTo(stream: IWritableStream<T>): void;
}

interface ITransformStream<W, R>
{
  asWritable(): IWritableStream<W>;
  asReadable(): IReadableStream<R>;
}

export declare class DBWorker extends EmbindClassHandle
{
  constructor(db: Ref<ProjectDB>, url: string);
  getWorker(): Worker;
}

export declare class VectorTimestamp extends EmbindClassHandle
{
  isEmpty(): boolean;
  toString(): string;
  asArray(): Uint32Array;
  clone(): VectorTimestamp;
  ltOther(other: Ref<VectorTimestamp>): boolean;
  eqOther(other: Ref<VectorTimestamp>): boolean;
  merge(other: Ref<VectorTimestamp>): void;
  reset(): void;
  setFromOther(other: Ref<VectorTimestamp>): void;
  update(timestamp: Timestamp): void;
  getClockAtSite(siteId: number): number;
  static fromArray(array: ArrayBufferView): VectorTimestamp;
}

export declare class Tag extends EmbindClassHandle
{
  isEmpty(): boolean;
  toString(): string;
  asArray(): Uint32Array;
  clone(): Tag;
  eqOther(other: Ref<Tag>): boolean;
  reset(): void;
  setFromOther(other: Ref<Tag>): void;
  setFromArray(array: ArrayBufferView): void;
  static fromArray(array: ArrayBufferView): Tag;
}

export declare class ProjectDB extends EmbindClassHandle
{
  constructor(getTypeSpecCallback: (type: string, needsData: boolean) => void,
    eventRaisedCallback: (event: DBEvent) => void);

  resolveTypeSpec(type: string): void;

  getNode(nodeId: NodeId): NodeData | null;
  getNodeChildren(nodeId: NodeId, includePending?: boolean): NodeChildrenData | null;
  getNodeValue(nodeId: NodeId): number;
  getNodeBlockValue(nodeId: NodeId): string;

  getRootNodeId(nodeId: NodeId): NodeId;

  createOperationBuilder(): OperationBuilder;
  createTypeLogGenerator(): TypeLogGenerator;
  createWorker(url: string): DBWorker;

  getVectorClock(): ManagedRef<VectorTimestamp>;

  createApplyStream(): IWritableStream<LogOperation> & IEmbindClassHandle;
  createUnapplyStream(): IWritableStream<LogOperation> & IEmbindClassHandle;

  processingEvent(): void;
  acknowledgeEvent(): void;

  static addType(type: string, format: string, data: Uint8Array): void;
  static deleteType(type: string): void;
  static clearTypes(): void;

  static getChildId(nodeId: NodeId, childOffset: number): NodeId;
  static getChildOffset(nodeId: NodeId): number;
  static NULLID(): NodeId;
  static ROOTID(): NodeId;
  static INITID(): NodeId;
  static PENDINGID(): NodeId;
}

export declare class TypeLogGenerator extends EmbindClassHandle
{
  addNode(nodeId: NodeId): void;
  addAllNodes(rootId: NodeId): void;
  addAllNodesWithFilter(rootId: NodeId, filterFn: (nodeId: NodeId) => boolean): void;
  generate(outputStream: IWritableStream<LogOperation>): void;

  /**
   * @deprecated Prefer streams instead. Will likely be removed in a future version.
   */
  generateToBuffer(format: string): Uint8Array;
}

export declare class OperationBuilder extends EmbindClassHandle
{
  setSiteId(siteId: number): void;
  getSiteId(): number;
  setTag(tag: Ref<Tag>): void;
  getTag(): Tag;

  setEnabled(enabled: boolean): void;
  setOperationLog(log: Ref<OperationLog>): void;

  createApplyStream(): IWritableStream<LogOperation> & IEmbindClassHandle;
  createUndoStream(): IWritableStream<LogOperation> & IEmbindClassHandle;
  getReadableStream(): ManagedRef<IReadableStream<LogOperation> & IEmbindClassHandle>;

  cloneAllNodes(rootId: NodeId): NodeId;
  cloneAllNodesFrom(db: ProjectDB, rootId: NodeId): NodeId;

  /**
   * @deprecated Prefer streams instead. Will likely be removed in a future version.
   */
  applyOperations(format: string, data: Uint8Array): Timestamp;

  startGroup(): void;
  commitGroup(): Timestamp;
  discardGroup(): void;

  createNode(type: NodeType): NodeId;
  createContainerNode(type: NodeType, childType: NodeType): NodeId;

  addChild(parentId: NodeId, childId: NodeId, edgeData: Uint8Array | null): EdgeId;
  removeChild(parentId: NodeId, edgeId: EdgeId): Timestamp;

  createKey(key: string): Uint8Array;
  createFloat64Key(key: number): Uint8Array;

  createPositionBetweenEdges(firstEdgeId: EdgeId | null, secondEdgeId: EdgeId | null): Uint8Array;
  createPositionFromIndex(parentId: NodeId, index: number): Uint8Array;
  createPositionFromEdge(parentId: NodeId, sourceEdgeId: EdgeId): Uint8Array;
  createPositionAbsolute(position: number): Uint8Array;

  setValue(nodeId: NodeId, value: number): Timestamp;
  setValueInt(nodeId: NodeId, value: number): Timestamp;
  setValueDouble(nodeId: NodeId, value: number): Timestamp;
  setValueBool(nodeId: NodeId, value: boolean): Timestamp;
  setValueRaw(nodeId: NodeId, value: Uint8Array): Timestamp;
  setValuePreview(nodeId: NodeId, value: number): Timestamp;
  setValuePreviewInt(nodeId: NodeId, value: number): Timestamp;
  setValuePreviewDouble(nodeId: NodeId, value: number): Timestamp;
  setValuePreviewRaw(nodeId: NodeId, value: Uint8Array): Timestamp;
  clearValuePreview(nodeId: NodeId): Timestamp;

  insertText(nodeId: NodeId, offset: number, text: string): void;
  insertTextAtFront(nodeId: NodeId, text: string): void;
  deleteText(nodeId: NodeId, offset: number, length: number): void;
  deleteInheritedText(nodeId: NodeId): void;

  getNextTimestamp(): Timestamp;
  getNextNodeId(): NodeId;
}

declare class OperationLog extends EmbindClassHandle
{
  applyOperation(op: any): void;
  createApplyStream(): IWritableStream<LogOperation> & IEmbindClassHandle;
  createReadStream(): ManagedRef<(IEmbindClassHandle & IReadableStream<LogOperation>)>;
  cancelReadStream(stream: IReadableStream<LogOperation>): void;
  getVectorClock(): ManagedRef<VectorTimestamp>;
}

declare class OperationFilter extends EmbindClassHandle
{
  setTagClockRange(tag: Ref<Tag>, startTime: Ref<VectorTimestamp> | null, endTime: Ref<VectorTimestamp> | null): void;
  setClockRange(startTime: Ref<VectorTimestamp> | null, endTime: Ref<VectorTimestamp> | null): void;
  setSiteFilter(siteId: number): void;
  setSiteFilterInvert(invert: boolean): void;
  invert(): void;
  reset(): void;

  boundClock(end: Ref<VectorTimestamp>): void;
  boundTag(tag: Ref<Tag>, end: Ref<VectorTimestamp>): void;
  boundTags(end: Ref<VectorTimestamp>): void;
  removeTag(tag: Ref<Tag>): void;

  merge(tag: Ref<OperationFilter>): void;

  /**
   * @deprecated Use streams instead.
   */
  filter(op: ArrayBufferView): void;
  clone(): OperationFilter;
  setFromOther(other: Ref<OperationFilter>): void;
  isBounded(): boolean;
  hasTags(): boolean;
  toString(): string;
  clockRangeToString(): string;

  static DefaultFormat(): string;
  static Serialize(format: string, filter: Ref<OperationFilter>): Uint8Array;
  static Deserialize(format: string, data: ArrayBufferView): OperationFilter;
}

export declare class LogOperationSerializer extends EmbindClassHandle implements ITransformStream<LogOperation, ArrayBufferView>
{
 asReadable(): IReadableStream<ArrayBufferView>;
 asWritable(): IWritableStream<LogOperation>;
 setOutputSize(size: number): void;
}

export declare class LogOperationDeserializer extends EmbindClassHandle implements ITransformStream<ArrayBufferView, LogOperation>
{
 asReadable(): IReadableStream<LogOperation>;
 asWritable(): IWritableStream<ArrayBufferView>;
}

export declare class LogOperationSerialization extends EmbindClassHandle
{
  static DefaultFormat(): string;
  static DefaultTypeFormat(): string;
  static CreateSerializer(format: string): LogOperationSerializer;
  static CreateDeserializer(format: string, direction: DeserializeDirection): LogOperationDeserializer;
}

export declare class DataCallbackStream extends EmbindClassHandle implements IWritableStream<ArrayBufferView>
{
  constructor(dataCallback: (data: Uint8Array) => void, endCallback: () => void);
  write(data: ArrayBufferView): boolean;
  close(): void;
}

export declare class LogOperationCallbackStream extends EmbindClassHandle implements IWritableStream<LogOperation>
{
  constructor(dataCallback: (data: LogOperation) => void, endCallback: () => void);
  write(data: ArrayBufferView): boolean;
  close(): void;
}

export declare class LogOperationTeeStream extends EmbindClassHandle implements IWritableStream<LogOperation>
{
  constructor(destA: IWritableStream<LogOperation>, destB: IWritableStream<LogOperation>);
  write(data: LogOperation): boolean;
  close(): void;
}

export declare class FilterOperationStream extends EmbindClassHandle implements ITransformStream<LogOperation, LogOperation>
{
  getFilter(): ManagedRef<OperationFilter>;
  asReadable(): IReadableStream<LogOperation>;
  asWritable(): IWritableStream<LogOperation>;
}

export declare class TransformOperationStream extends EmbindClassHandle implements ITransformStream<LogOperation, LogOperation>
{
  constructor(local: boolean, db: Ref<ProjectDB>);
  mapType(fromTypeId: string, toTypeId: string): void;
  mapTypeNodeId(typeId: string, fromOffset: number, toOffset: number): void;
  mapTypeEdgeId(typeId: string, fromOffset: number, toOffset: number): void;
  asReadable(): IReadableStream<LogOperation>;
  asWritable(): IWritableStream<LogOperation>;
}

export declare class UndoFilterOperationStream extends EmbindClassHandle implements ITransformStream<LogOperation, LogOperation>
{
  constructor(siteId: number);
  asReadable(): IReadableStream<LogOperation>;
  asWritable(): IWritableStream<LogOperation>;
}

export declare class RedoFilterOperationStream extends EmbindClassHandle implements ITransformStream<LogOperation, LogOperation>
{
  constructor(siteId: number);
  asReadable(): IReadableStream<LogOperation>;
  asWritable(): IWritableStream<LogOperation>;
}

export declare class WriteFilterOperationStream extends EmbindClassHandle implements ITransformStream<LogOperation, LogOperation>
{
  asReadable(): IReadableStream<LogOperation>;
  asWritable(): IWritableStream<LogOperation>;
}

export declare class QoSFilterOperationStream extends EmbindClassHandle implements ITransformStream<LogOperation, LogOperation>
{
  setFilterRate(period: number): void;
  asReadable(): IReadableStream<LogOperation>;
  asWritable(): IWritableStream<LogOperation>;
}

export interface crdblModule
{
  DBWorker: typeof DBWorker;
  ProjectDB: typeof ProjectDB;

  OperationLog: typeof OperationLog;
  Tag: typeof Tag;
  VectorTimestamp: typeof VectorTimestamp;
  OperationFilter: typeof OperationFilter;
  OperationBuilder: typeof OperationBuilder;

  TypeLogGenerator: typeof TypeLogGenerator;
  LogOperationSerializer: typeof LogOperationSerializer;
  LogOperationDeserializer: typeof LogOperationDeserializer;
  LogOperationSerialization: typeof LogOperationSerialization;
  LogOperationCallbackStream: typeof LogOperationCallbackStream;
  DataCallbackStream: typeof DataCallbackStream;
  TransformOperationStream: typeof TransformOperationStream;
  FilterOperationStream: typeof FilterOperationStream;
  UndoFilterOperationStream: typeof UndoFilterOperationStream;
  RedoFilterOperationStream: typeof RedoFilterOperationStream;
  WriteFilterOperationStream: typeof WriteFilterOperationStream;
  QoSFilterOperationStream: typeof QoSFilterOperationStream;
  LogOperationTeeStream: typeof LogOperationTeeStream;
  DeserializeDirection: typeof DeserializeDirection;
}

export default function init(module?: any): Promise<crdblModule>;