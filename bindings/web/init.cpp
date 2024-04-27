#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#include "ProjectDB.h"
#include "VectorTimestampExtensions.h"
#include "TagExtensions.h"
#include "OperationFilterWrapper.h"
#include "TypeLogGeneratorWrapper.h"
#include "OperationBuilderWrapper.h"
#include "TransformOperationStreamWrapper.h"
#include "Wrappers.h"
#include "Worker.h"
#include <OperationFilter.h>
#include <OperationLog.h>
#include <Streams/FilterOperationStream.h>
#include <Streams/CallbackWritableStream.h>
#include <Streams/TeeStream.h>
#include <Streams/TransformOperationStream.h>
#include <Streams/UndoFilterOperationStream.h>
#include <Streams/RedoFilterOperationStream.h>
#include <Streams/QoSFilterOperationStream.h>
#include <Streams/WriteFilterOperationStream.h>
#include <Serialization/LogOperationSerialization.h>
#include <Serialization/ILogOperationDeserializer.h>
#include <Serialization/ILogOperationSerializer.h>
#include <RefCounted.h>
#include <string>

EMSCRIPTEN_BINDINGS(Worker)
{
  class_<Worker>("DBWorker")
    .constructor<ProjectDB &, std::string>()

    .function("getWorker", &Worker::getWorker)
    ;
}

EMSCRIPTEN_BINDINGS(Timestamp)
{
  value_array<Timestamp>("Timestamp")
    .element(&Timestamp::clock)
    .element(&Timestamp::site)
    ;
}

EMSCRIPTEN_BINDINGS(DeserializeDirection)
{
  enum_<DeserializeDirection>("DeserializeDirection")
    .value("Forward", DeserializeDirection::Forward)
    .value("Reverse", DeserializeDirection::Reverse)
    ;
}

EMSCRIPTEN_BINDINGS(VectorTimestamp)
{
  class_<VectorTimestamp>("VectorTimestamp")
    .constructor<>()

    .function("isEmpty", &VectorTimestamp::isEmpty)
    .function("toString", &VectorTimestamp::toString)
    .function("asArray", &VectorTimestamp_asArray)
    .function("clone", &VectorTimestamp_clone)
    .function("ltOther", &VectorTimestamp_ltOther)
    .function("eqOther", &VectorTimestamp_eqOther)
    .function("merge", &VectorTimestamp::merge)
    .function("reset", &VectorTimestamp::reset)
    .function("setFromOther", &VectorTimestamp_setFromOther)
    .function("update", &VectorTimestamp::update)
    .function("getClockAtSite", &VectorTimestamp::getClockAtSite)

    .class_function("fromArray", &VectorTimestamp_fromArray)
    ;
}

EMSCRIPTEN_BINDINGS(Tag)
{
  class_<Tag>("Tag")
    .constructor<>()
    .function("isNull", &Tag::isNull)
    .function("toString", &Tag::toString)
    .function("reset", &Tag::reset)
    .function("asArray", &Tag_asArray)
    .function("clone", &Tag_clone)
    .function("eqOther", &Tag_eqOther)
    .function("setFromOther", &Tag_setFromOther)
    .function("setFromArray", &Tag_setFromArray)

    .class_function("fromArray", &Tag_fromArray)
    ;
}

EMSCRIPTEN_BINDINGS(ProjectDB)
{
  class_<ProjectDB>("ProjectDB")
    .constructor<val, val>()

    .function("resolveTypeSpec", &ProjectDB::resolveTypeSpec)

    .function("getNode_raw", &ProjectDB::getNode)
    .function("getNodeChildren_raw", &ProjectDB::getNodeChildren)
    .function("getNodeValue_raw", &ProjectDB::getNodeValue)
    .function("getNodeBlockValue_raw", &ProjectDB::getNodeBlockValue)

    .function("getVectorClock", &ProjectDB::getVectorClock, allow_raw_pointers())

    .function("createApplyStream", &ProjectDB::createApplyStream, allow_raw_pointers())
    .function("createUnapplyStream", &ProjectDB::createUnapplyStream, allow_raw_pointers())

    .function("createOperationBuilder", &ProjectDB::createOperationBuilder, allow_raw_pointers())
    .function("createTypeLogGenerator", &ProjectDB::createTypeLogGenerator, allow_raw_pointers())
    .function("createWorker", &ProjectDB::createWorker, allow_raw_pointers())

    .class_function("addType", &ProjectDB::addType)
    .class_function("deleteType", &ProjectDB::deleteType)
    .class_function("clearTypes", &ProjectDB::clearTypes)

    .class_function("NULLID", &ProjectDB::NULLID)
    .class_function("ROOTID", &ProjectDB::ROOTID)
    .class_function("INITID", &ProjectDB::INITID)
    .class_function("PENDINGID", &ProjectDB::PENDINGID)
    ;
}

EMSCRIPTEN_BINDINGS(TypeLogGenerator)
{
  class_<TypeLogGeneratorWrapper>("TypeLogGenerator")

    .function("addNode", &TypeLogGeneratorWrapper::addNode)
    .function("addAllNodes", &TypeLogGeneratorWrapper::addAllNodes)
    .function("addAllNodesWithFilter", &TypeLogGeneratorWrapper::addAllNodesWithFilter)
    .function("generate", &TypeLogGeneratorWrapper::generate, allow_raw_pointers())

    .function("generateToBuffer", &TypeLogGeneratorWrapper::generateToBuffer, allow_raw_pointers())
    ;
}

EMSCRIPTEN_BINDINGS(OperationBuilder)
{
  class_<OperationBuilder>("OperationBuilder")

    .function("setSiteId", &OperationBuilder::setSiteId)
    .function("getSiteId", &OperationBuilder::getSiteId)
    .function("setTag", &OperationBuilder::setTag)
    .function("getTag", &OperationBuilder::getTag)

    .function("setEnabled", &OperationBuilder::setEnabled)
    .function("setOperationLog", &OperationBuilder::setOperationLog)

    .function("createApplyStream", &OperationBuilder::createApplyStream, allow_raw_pointers())
    .function("createUndoStream", &OperationBuilder::createUndoStream, allow_raw_pointers())
    .function("getReadableStream", &OperationBuilderWrapper::getReadableStream, allow_raw_pointers())

    .function("cloneAllNodes", &OperationBuilderWrapper::cloneAllNodes)
    .function("cloneAllNodesFrom", &OperationBuilderWrapper::cloneAllNodesFrom)

    .function("startGroup", &OperationBuilderWrapper::startGroup)
    .function("commitGroup", &OperationBuilder::commitGroup)
    .function("discardGroup", &OperationBuilder::discardGroup)

    .function("applyOperations", &OperationBuilderWrapper::applyOperations)

    .function("createNode", &OperationBuilderWrapper::createNode)
    .function("createContainerNode", &OperationBuilderWrapper::createContainerNode)

    .function("addChild", &OperationBuilderWrapper::addChild)
    .function("removeChild", &OperationBuilderWrapper::removeChild)

    .function("createKey", &OperationBuilderWrapper::createKey)
    .function("createFloat64Key", &OperationBuilderWrapper::createFloat64Key)

    .function("createPositionBetweenEdges", &OperationBuilderWrapper::createPositionBetweenEdges)
    .function("createPositionFromIndex", &OperationBuilderWrapper::createPositionFromIndex)
    .function("createPositionFromEdge", &OperationBuilderWrapper::createPositionFromEdge)
    .function("createPositionAbsolute", &OperationBuilderWrapper::createPositionAbsolute)

    .function("setValue", &OperationBuilderWrapper::setValueAuto)
    .function("setValueInt", select_overload<Timestamp(OperationBuilder &, const StringNodeId &, int32_t)>(&OperationBuilderWrapper::setValue<int32_t>))
    .function("setValueDouble", select_overload<Timestamp(OperationBuilder &, const StringNodeId &, double)>(&OperationBuilderWrapper::setValue<double>))
    .function("setValueBool", select_overload<Timestamp(OperationBuilder &, const StringNodeId &, bool)>(&OperationBuilderWrapper::setValue<bool>))
    .function("setValueRaw", &OperationBuilderWrapper::setValueRaw)
    .function("setValuePreview", &OperationBuilderWrapper::setValuePreviewAuto)
    .function("setValuePreviewInt", select_overload<Timestamp(OperationBuilder &, const StringNodeId &, int32_t)>(&OperationBuilderWrapper::setValuePreview<int32_t>))
    .function("setValuePreviewDouble", select_overload<Timestamp(OperationBuilder &, const StringNodeId &, double)>(&OperationBuilderWrapper::setValuePreview<double>))
    .function("setValuePreviewBool", select_overload<Timestamp(OperationBuilder &, const StringNodeId &, bool)>(&OperationBuilderWrapper::setValuePreview<bool>))
    .function("setValuePreviewRaw", &OperationBuilderWrapper::setValuePreviewRaw)
    .function("clearValuePreview", &OperationBuilderWrapper::clearValuePreview)

    .function("insertText", select_overload<void(OperationBuilder & ref, const StringNodeId &, size_t, const std::string &)>(&OperationBuilderWrapper::insertText))
    .function("insertTextAtFront", select_overload<void(OperationBuilder & ref, const StringNodeId &, const std::string &)>(&OperationBuilderWrapper::insertTextAtFront))
    .function("deleteText", &OperationBuilderWrapper::deleteText)
    .function("deleteInheritedText", &OperationBuilderWrapper::deleteInheritedText)

    .function("getNextTimestamp", &OperationBuilder::getNextTimestamp)
    .function("getNextNodeId", &OperationBuilderWrapper::getNextNodeId)
    ;
}

EMSCRIPTEN_BINDINGS(OperationLog)
{
  class_<OperationLog>("OperationLog")
    .constructor<>()

    .function("applyOperation", &OperationLog::applyOperation, allow_raw_pointers())
    .function("createApplyStream", &OperationLog::createApplyStream, allow_raw_pointers())
    .function("createReadStream", &OperationLog::createReadStream, allow_raw_pointers())
    .function("cancelReadStream", &OperationLog::cancelReadStream, allow_raw_pointers())
    .function("getVectorClock", &OperationLog_getVectorClock, allow_raw_pointers())
    ;
}

EMSCRIPTEN_BINDINGS(OperationFilter)
{
  class_<OperationFilter>("OperationFilter")
    .constructor<>()

    .function("setTagClockRange", &OperationFilter_setTagClockRange, allow_raw_pointers())
    .function("setClockRange", &OperationFilter_setClockRange, allow_raw_pointers())
    .function("setSiteFilter", &OperationFilter_setSiteFilter, allow_raw_pointers())
    .function("setSiteFilterInvert", &OperationFilter_setSiteFilterInvert, allow_raw_pointers())

    .function("invert", &OperationFilter_invert)
    .function("setInvert", &OperationFilter::setInvert)
    .function("reset", &OperationFilter_reset)

    .function("boundClock", &OperationFilter_boundClock)
    .function("boundTag", &OperationFilter_boundTag)
    .function("boundTags", &OperationFilter_boundTags)
    .function("removeTag", &OperationFilter_removeTag)

    .function("merge", &OperationFilter_merge)

    .function("filter", &OperationFilter_filter)
    .function("clone", &OperationFilter_clone)
    .function("setFromOther", &OperationFilter_setFromOther)
    .function("isBounded", &OperationFilter::isBounded)
    .function("hasTags", &OperationFilter::hasTags)
    .function("clockRangeToString", &OperationFilter::clockRangeToString)
    .function("toString", &OperationFilter::toString)

    .class_function("DefaultFormat", &OperationFilter::DefaultFormat)
    .class_function("Serialize", &OperationFilter_Serialize)
    .class_function("Deserialize", &OperationFilter_Deserialize)
    ;
}

EMSCRIPTEN_BINDINGS(LogOperationSerialization)
{
  class_<LogOperationSerialization>("LogOperationSerialization")
    .class_function("DefaultFormat", &LogOperationSerialization_DefaultFormat)
    .class_function("DefaultTypeFormat", &LogOperationSerialization_DefaultTypeFormat)
    .class_function("CreateSerializer", &LogOperationSerialization::CreateSerializer, allow_raw_pointers())
    .class_function("CreateDeserializer", &LogOperationSerialization::CreateDeserializer, allow_raw_pointers())
    ;
}

EMSCRIPTEN_BINDINGS(LogOperation)
{
  value_object<RefCounted<const LogOperation>>("LogOperation")
    ;
}

EMSCRIPTEN_BINDINGS(ReadableDataStream)
{
  class_<IReadableStream<std::string_view>>("ReadableDataStream")
    .function("pipeTo", &IReadableStream<std::string_view>::pipeTo)
    ;
}

EMSCRIPTEN_BINDINGS(WritableDataStream)
{
  class_<IWritableStream<std::string_view>>("WritableDataStream")
    .function("write", &WritableDataStream_write)
    .function("close", &IWritableStream<std::string_view>::close)
    ;
}

EMSCRIPTEN_BINDINGS(ReadableLogOperationStream)
{
  class_<IReadableStream<RefCounted<const LogOperation>>>("ReadableLogOperationStream")
    .function("pipeTo", &IReadableStream<RefCounted<const LogOperation>>::pipeTo)
    ;
}

EMSCRIPTEN_BINDINGS(WritableLogOperationStream)
{
  class_<IWritableStream<RefCounted<const LogOperation>>>("WritableLogOperationStream")
    .function("close", &IWritableStream<RefCounted<const LogOperation>>::close)
    ;
}

EMSCRIPTEN_BINDINGS(DataCallbackStream)
{
  class_<CallbackWritableStream<std::string_view>,
    base<IWritableStream<std::string_view>>>("DataCallbackStream")
    .constructor(&DataCallbackStream_Create, allow_raw_pointers())
    ;
}

EMSCRIPTEN_BINDINGS(ILogOperationSerializer)
{
  class_<ILogOperationSerializer>("LogOperationSerializer")
    .function("asReadable", &ITransformStream_asReadable<
      ILogOperationSerializer, std::string_view>, allow_raw_pointers())
    .function("asWritable", &ITransformStream_asWritable<
      ILogOperationSerializer, RefCounted<const LogOperation>>, allow_raw_pointers())
    .function("setOutputSize", &ILogOperationSerializer::setOutputSize)
    ;
}

EMSCRIPTEN_BINDINGS(ILogOperationDeserializer)
{
  class_<ILogOperationDeserializer>("LogOperationDeserializer")
    .function("asReadable", &ITransformStream_asReadable<
      ILogOperationDeserializer, RefCounted<const LogOperation>>, allow_raw_pointers())
    .function("asWritable", &ITransformStream_asWritable<
      ILogOperationDeserializer, std::string_view>, allow_raw_pointers())
    ;
}

EMSCRIPTEN_BINDINGS(LogOperationCallbackStream)
{
  class_<CallbackWritableStream<RefCounted<const LogOperation>>,
    base<IWritableStream<RefCounted<const LogOperation>>>>("LogOperationCallbackStream")
    .constructor(&LogOperationCallbackStream_Create, allow_raw_pointers())
    ;
}

EMSCRIPTEN_BINDINGS(LogOperationTeeStream)
{
  class_<TeeStream<RefCounted<const LogOperation>>,
    base<IWritableStream<RefCounted<const LogOperation>>>>("LogOperationTeeStream")
    .constructor<IWritableStream<RefCounted<const LogOperation>> &, IWritableStream<RefCounted<const LogOperation>> &>()
    ;
}

EMSCRIPTEN_BINDINGS(TransformOperationStream)
{
  class_<TransformOperationStream>("TransformOperationStream")
    .constructor(&TransformOperationStreamWrapper::makeTransformOperationStream, allow_raw_pointers())
    .function("mapType", &TransformOperationStream::mapType)
    .function("mapTypeNodeId", &TransformOperationStream::mapTypeNodeId)
    .function("mapTypeEdgeId", &TransformOperationStream::mapTypeEdgeId)
    .function("asReadable", &ITransformStream_asReadable<
      TransformOperationStream, RefCounted<const LogOperation>>, allow_raw_pointers())
    .function("asWritable", &ITransformStream_asWritable<
      TransformOperationStream, RefCounted<const LogOperation>>, allow_raw_pointers())
    ;
}

EMSCRIPTEN_BINDINGS(FilterOperationStream)
{
  class_<FilterOperationStream>("FilterOperationStream")
    .constructor()
    .function("getFilter", &FilterOperationStream_getFilter, allow_raw_pointers())
    .function("asReadable", &ITransformStream_asReadable<
      FilterOperationStream, RefCounted<const LogOperation>>, allow_raw_pointers())
    .function("asWritable", &ITransformStream_asWritable<
      FilterOperationStream, RefCounted<const LogOperation>>, allow_raw_pointers())
    ;
}

EMSCRIPTEN_BINDINGS(UndoFilterOperationStream)
{
  class_<UndoFilterOperationStream>("UndoFilterOperationStream")
    .constructor<uint32_t>()
    .function("asReadable", &ITransformStream_asReadable<
      UndoFilterOperationStream, RefCounted<const LogOperation>>, allow_raw_pointers())
    .function("asWritable", &ITransformStream_asWritable<
      UndoFilterOperationStream, RefCounted<const LogOperation>>, allow_raw_pointers())
    ;
}

EMSCRIPTEN_BINDINGS(RedoFilterOperationStream)
{
  class_<RedoFilterOperationStream>("RedoFilterOperationStream")
    .constructor<uint32_t>()
    .function("asReadable", &ITransformStream_asReadable<
      RedoFilterOperationStream, RefCounted<const LogOperation>>, allow_raw_pointers())
    .function("asWritable", &ITransformStream_asWritable<
      RedoFilterOperationStream, RefCounted<const LogOperation>>, allow_raw_pointers())
    ;
}

EMSCRIPTEN_BINDINGS(WriteFilterOperationStream)
{
  class_<WriteFilterOperationStream>("WriteFilterOperationStream")
    .constructor()
    .function("asReadable", &ITransformStream_asReadable<
      WriteFilterOperationStream, RefCounted<const LogOperation>>, allow_raw_pointers())
    .function("asWritable", &ITransformStream_asWritable<
      WriteFilterOperationStream, RefCounted<const LogOperation>>, allow_raw_pointers())
    ;
}

template class QoSFilterOperationStream<std_time_0, std::time_t>;

EMSCRIPTEN_BINDINGS(QoSFilterOperationStream)
{
  class_<QoSFilterOperationStream<emscripten_get_now, double>,
    base<IWritableStream<RefCounted<const LogOperation>>>>("QoSFilterOperationStream")
    .constructor()
    .function("setFilterRate", &QoSFilterOperationStream<emscripten_get_now, double>::setFilterRate)
    .function("asReadable", &ITransformStream_asReadable<
      QoSFilterOperationStream<emscripten_get_now, double>, RefCounted<const LogOperation>>, allow_raw_pointers())
    .function("asWritable", &ITransformStream_asWritable<
      QoSFilterOperationStream<emscripten_get_now, double>, RefCounted<const LogOperation>>, allow_raw_pointers())
    ;
}