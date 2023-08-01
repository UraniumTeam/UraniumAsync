set(SRC
    UnAsync/Cancellation/CancellationSource.h
    UnAsync/Cancellation/CancellationToken.h
    UnAsync/Cancellation/CancellationSource.cpp
    UnAsync/Cancellation/CancellationToken.cpp

    UnAsync/Internal/BoolPointer.h
    UnAsync/Internal/ManualResetEvent.cpp
    UnAsync/Internal/ManualResetEvent.h
    UnAsync/Internal/PlatformInclude.h
    UnAsync/Internal/SyncWaitTask.h
    UnAsync/Internal/TaskPromise.h
    UnAsync/Internal/TaskPromiseBase.h
    UnAsync/Internal/WhenAllCounter.h
    UnAsync/Internal/WhenAllReadyAwaiter.h
    UnAsync/Internal/WhenAllTask.h
    UnAsync/Internal/TaskMapAwaiter.h
    UnAsync/Internal/CancellationState.cpp
    UnAsync/Internal/CancellationState.h

    UnAsync/Jobs/Job.h
    UnAsync/Jobs/JobTree.h
    UnAsync/Jobs/IJobScheduler.h
    UnAsync/Jobs/JobScheduler.h
    UnAsync/Jobs/JobScheduler.cpp

    UnAsync/Parallel/Semaphore.h
    UnAsync/Parallel/Semaphore.cpp
    UnAsync/Parallel/SpinMutex.h

    UnAsync/Pipes/Pipe.cpp
    UnAsync/Pipes/Pipe.h
    UnAsync/Pipes/Internal/BufferSegment.h

    UnAsync/SyncWait.h
    UnAsync/Task.h
    UnAsync/Traits.h
    UnAsync/WhenAll.h
    UnAsync/TaskMap.h
    UnAsync/AsyncEvent.h
    UnAsync/AsyncEvent.cpp
    UnAsync/Buffers/IReadOnlySequenceSegment.h
    UnAsync/Buffers/ReadOnlySequence.h
    UnAsync/Pipes/PipeResults.h
)

add_library(UnAsync STATIC ${SRC})

un_configure_target(UnAsync)

target_include_directories(UnAsync PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

set_target_properties(UnAsync PROPERTIES FOLDER "UraniumAsync")

if (UN_WINDOWS)
    target_link_libraries(UnAsync UnTL Ws2_32 Mswsock Synchronization)
else ()
    target_link_libraries(UnAsync UnTL)
endif ()

get_property("TARGET_SOURCE_FILES" TARGET UnAsync PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

if (UN_BUILD_TESTS)
    add_subdirectory(Tests)
endif ()
