include_directories(
   .
)

set(server_example_goose_SRCS
        server_example_goose.cpp
        static_model.c
)

IF(WIN32)
set_source_files_properties(${server_example_goose_SRCS}
                                       PROPERTIES LANGUAGE CXX)
ENDIF(WIN32)

add_executable(server_example_goose
  ${server_example_goose_SRCS}
)

target_link_libraries(server_example_goose  iec61850 )


set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(server_example_goose Threads::Threads)

cmake_minimum_required(VERSION 3.6)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set (CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
