cmake_minimum_required(VERSION 3.10)
project (UPI_test)
include_directories(.)
set(test_src test/main.c)
set(upi_src universalProtocolInteractor.c)

add_executable(${PROJECT_NAME} ${upi_src} ${test_src})
