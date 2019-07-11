include_directories(
   .
)

set(mymasterproject_SRCS
        mymasterproject.cpp
        static_model.c
)

IF(WIN32)
set_source_files_properties(${mymasterproject_SRCS}
                                       PROPERTIES LANGUAGE CXX)
ENDIF(WIN32)

add_executable(mymasterproject
  ${mymasterproject_SRCS}
)

target_link_libraries(mymasterproject  iec61850 )


set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(mymasterproject Threads::Threads)

cmake_minimum_required(VERSION 3.6) #this can be changed
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11") #this version can also be changed
set (CMAKE_C_STANDARD 11)  #
set(CMAKE_CXX_STANDARD 11)  #
