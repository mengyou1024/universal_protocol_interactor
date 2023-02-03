cmake_minimum_required(VERSION 3.10)

project(test)

include_directories(..)

add_executable(${PROJECT_NAME} ../universalProtocolInteractor.c main.c)
