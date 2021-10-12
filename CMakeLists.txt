cmake_minimum_required(VERSION 3.16)

project(myFTP)

add_executable(${PROJECT_NAME} "main.cpp" "server.cpp" "server.h")
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

add_link_options(-lstdc++fs)

find_package(Boost REQUIRED COMPONENTS chrono coroutine system thread)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::chrono Boost::coroutine Boost::system Boost::thread)
