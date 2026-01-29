cmake_minimum_required(VERSION 3.24)

project(dos_detector LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

option(ENABLE_PORTABLE_RUNTIME "Link libgcc/libstdc++ statically for MinGW." ON)

if(MINGW AND ENABLE_PORTABLE_RUNTIME)
  add_link_options(-static-libgcc -static-libstdc++)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Charts Sql)
find_package(PcapPlusPlus REQUIRED CONFIG)
find_package(ONNXRuntime QUIET CONFIG)

add_executable(dos_detector
  src/main.cpp
  src/TrafficMonitor.cpp
  src/FeatureExtractor.cpp
)

target_include_directories(dos_detector PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(dos_detector PRIVATE
  Qt6::Widgets
  Qt6::Charts
  Qt6::Sql
)

if(TARGET PcapPlusPlus::PcapPlusPlus)
  target_link_libraries(dos_detector PRIVATE PcapPlusPlus::PcapPlusPlus)
else()
  target_include_directories(dos_detector PRIVATE ${PCAPPLUSPLUS_INCLUDE_DIRS})
  target_link_libraries(dos_detector PRIVATE ${PCAPPLUSPLUS_LIBRARIES})
endif()

if(ONNXRuntime_FOUND)
  target_link_libraries(dos_detector PRIVATE ONNXRuntime::onnxruntime)
  target_compile_definitions(dos_detector PRIVATE ONNX_RUNTIME_AVAILABLE)
endif()
