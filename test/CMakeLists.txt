include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -O0 -fsanitize=address -g")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -std=c++20 -Wall")

set(SOURCES
    tests.cpp
    helpers.cpp
    CoreTests.cpp
    BlockValueTests.cpp
    OperationFilterTests.cpp
    SetNodeTests.cpp
    ListNodeTests.cpp
    MapNodeTests.cpp
    AttributeTests.cpp
    NodeTests.cpp
    ReferenceNodeTests.cpp
    SerializationTests.cpp
    TimestampTests.cpp
    ValueTests.cpp
    VectorTimestampTests.cpp
    TypeLogGeneratorTests.cpp
    PromiseTests.cpp
)

add_executable(ProjectDBTest ${LIB_SOURCES} ${SOURCES})

target_link_libraries (ProjectDBTest LINK_PUBLIC gtest gtest_main pthread) # ProjectDB
include(GoogleTest)
target_include_directories(ProjectDBTest PUBLIC
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}/src"
)
