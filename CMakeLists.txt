cmake_minimum_required(VERSION 3.10)

project(ProjectDB)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# temporary
set(LIB_SOURCES
    "${PROJECT_SOURCE_DIR}/src/Core.cpp"
    "${PROJECT_SOURCE_DIR}/src/TypeLogGenerator.cpp"
    "${PROJECT_SOURCE_DIR}/src/Effect.cpp"
    "${PROJECT_SOURCE_DIR}/src/Position.cpp"
    "${PROJECT_SOURCE_DIR}/src/Attribute.cpp"
    "${PROJECT_SOURCE_DIR}/src/Tag.cpp"
    "${PROJECT_SOURCE_DIR}/src/NodeType.cpp"
    "${PROJECT_SOURCE_DIR}/src/PrimitiveNodeTypes.cpp"
    "${PROJECT_SOURCE_DIR}/src/Timestamp.cpp"
    "${PROJECT_SOURCE_DIR}/src/VectorTimestamp.cpp"
    "${PROJECT_SOURCE_DIR}/src/NodeId.cpp"
    "${PROJECT_SOURCE_DIR}/src/InheritanceContext.cpp"
    "${PROJECT_SOURCE_DIR}/src/Operation.cpp"
    "${PROJECT_SOURCE_DIR}/src/LogOperation.cpp"
    "${PROJECT_SOURCE_DIR}/src/OperationIterator.cpp"
    "${PROJECT_SOURCE_DIR}/src/OperationFilter.cpp"
    "${PROJECT_SOURCE_DIR}/src/OperationBuilder.cpp"
    "${PROJECT_SOURCE_DIR}/src/OperationLog.cpp"
    "${PROJECT_SOURCE_DIR}/src/Value.cpp"
    "${PROJECT_SOURCE_DIR}/src/BlockValue.cpp"
    "${PROJECT_SOURCE_DIR}/src/Json.cpp"
    "${PROJECT_SOURCE_DIR}/src/Event.cpp"
    "${PROJECT_SOURCE_DIR}/src/JsonSerializer.cpp"
    "${PROJECT_SOURCE_DIR}/src/Nodes/Node.cpp"
    "${PROJECT_SOURCE_DIR}/src/Nodes/ContainerNode.cpp"
    "${PROJECT_SOURCE_DIR}/src/Nodes/SetNode.cpp"
    "${PROJECT_SOURCE_DIR}/src/Nodes/ListNode.cpp"
    "${PROJECT_SOURCE_DIR}/src/Nodes/MapNode.cpp"
    "${PROJECT_SOURCE_DIR}/src/Nodes/ReferenceNode.cpp"
    "${PROJECT_SOURCE_DIR}/src/Nodes/OrderedFloat64MapNode.cpp"
    "${PROJECT_SOURCE_DIR}/src/Nodes/ValueNode.cpp"
    "${PROJECT_SOURCE_DIR}/src/Nodes/BlockValueNode.cpp"
    "${PROJECT_SOURCE_DIR}/src/Serialization/LogOperationSerialization.cpp"
    "${PROJECT_SOURCE_DIR}/src/Serialization/standard_v1/LogOperationSerialization.cpp"
    "${PROJECT_SOURCE_DIR}/src/Serialization/standard_v1/LogOperation.cpp"
    "${PROJECT_SOURCE_DIR}/src/Serialization/standard_v1/LogOperationDeserializer.cpp"
    "${PROJECT_SOURCE_DIR}/src/Serialization/standard_v1/LogOperationSerializer.cpp"
    "${PROJECT_SOURCE_DIR}/src/Streams/CallbackWritableStream.cpp"
    "${PROJECT_SOURCE_DIR}/src/Streams/TransformOperationStream.cpp"
    "${PROJECT_SOURCE_DIR}/src/Streams/FilterOperationStream.cpp"
    "${PROJECT_SOURCE_DIR}/src/Streams/ReadableStreamBase.cpp"
    "${PROJECT_SOURCE_DIR}/src/Streams/RedoFilterOperationStream.cpp"
    "${PROJECT_SOURCE_DIR}/src/Streams/UndoFilterOperationStream.cpp"
    "${PROJECT_SOURCE_DIR}/src/Streams/WriteFilterOperationStream.cpp"
    "${PROJECT_SOURCE_DIR}/src/Streams/TeeStream.cpp"
)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(bindings/web)