cmake_minimum_required(VERSION 3.10.0)
project(socket_pratice VERSION 0.1.0 LANGUAGES C CXX)

add_executable(client
client.cpp)

add_executable(server
server.cpp)

add_executable(server_thread
server_thread.cpp)

add_executable(server_threadpool
ThreadPool/ThreadPool.cpp
ThreadPool/TaskQueue.cpp
server_threadpool.cpp
)

add_executable(server_select
server_select.cc
)

add_executable(client_select
client_select.cc
)

add_executable(server_select_thread
server_select_thread.cc
)