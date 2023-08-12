set(SRC
    Common/Common.h

    main.cpp
    Buffers/ReadOnlySequence.cpp
)

add_executable(UnAsyncTests ${SRC})

set_target_properties(UnAsyncTests PROPERTIES FOLDER "UraniumAsync")
target_link_libraries(UnAsyncTests gtest gmock UnAsync)

get_property("TARGET_SOURCE_FILES" TARGET UnAsyncTests PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

include(GoogleTest)
gtest_discover_tests(UnAsyncTests)
