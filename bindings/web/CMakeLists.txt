# debug: -g -O0
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s WASM_WORKERS -msimd128 -std=c++20 -O3 -Wall")

set(SOURCES
    ProjectDB.cpp
    JsObjectSerializer.cpp
    OperationFilterWrapper.cpp
    VectorTimestampExtensions.cpp
    TagExtensions.cpp
    Worker.cpp
)

set(INIT_SOURCE init.cpp)
add_executable(ProjectDBWeb ${LIB_SOURCES} ${SOURCES} ${INIT_SOURCE})

#debug
# -g -s DEMANGLE_SUPPORT=1

# MODULARIZE, EXPORT_ES6, ENVIRONMENT='web', USE_ES6_IMPORT_META
set_target_properties(ProjectDBWeb PROPERTIES LINK_FLAGS "--no-entry -s EXPORT_ES6=1 --bind -s SHARED_MEMORY=1 -s WASM_WORKERS -s ALLOW_MEMORY_GROWTH=1 -s WARN_ON_UNDEFINED_SYMBOLS=1 -s LLD_REPORT_UNDEFINED")

target_include_directories(ProjectDBWeb PUBLIC
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}/src"
)