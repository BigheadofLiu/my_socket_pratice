cmake_minimum_required(VERSION 3.10.0)
project(socket_pratice VERSION 0.1.0 LANGUAGES C CXX)

add_executable(cilent
client.cpp)

add_executable(server
server.cpp)

