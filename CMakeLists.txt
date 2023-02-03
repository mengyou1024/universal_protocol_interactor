cmake_minimum_required(VERSION 3.10)
project (UPI_test)
include_directories(.)
set(test_src test/main.c)
set(upi_src universalProtocolInteractor.c)
set(CMAKE_EXE_LINKER_FLAGS_DEBUG " -T ${CMAKE_SOURCE_DIR}/link.lds -static")

add_executable(${PROJECT_NAME} ${upi_src} ${test_src})
