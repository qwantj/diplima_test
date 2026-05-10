cmake_minimum_required(VERSION 3.16)
project(DiplomDDoS_Tests)

find_package(Qt6 REQUIRED COMPONENTS Core Network Sql Widgets Charts)

add_executable(db_benchmark run_benchmark.cpp)
target_include_directories(db_benchmark PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(db_benchmark PRIVATE ddos_core Qt6::Core Qt6::Sql)
