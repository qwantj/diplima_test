cmake_minimum_required(VERSION 3.16)
project(DiplomDDoS_Tests)

find_package(Qt6 REQUIRED COMPONENTS Core Network Sql Widgets Charts Test)

add_executable(db_benchmark run_benchmark.cpp)
target_include_directories(db_benchmark PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(db_benchmark PRIVATE ddos_core Qt6::Core Qt6::Sql)

add_executable(db_benchmark_qt unit/benchmark_DatabaseManager.cpp)
target_link_libraries(db_benchmark_qt PRIVATE Qt6::Core Qt6::Sql Qt6::Test)
add_test(NAME db_benchmark_qt COMMAND db_benchmark_qt)

add_executable(ddos_unit_tests
    unit/main.cpp
    unit/test_ConfigManager.hpp
    unit/test_ConfigManager.cpp
    unit/ProtocolTests.hpp
    unit/ProtocolTests.cpp
    unit/CSVUtilsTests.hpp
    unit/CSVUtilsTests.cpp
    unit/test_FileBuffer.hpp
    unit/test_FileBuffer.cpp
    unit/test_ModelInferencer.hpp
    unit/test_ModelInferencer.cpp
)

target_link_libraries(ddos_unit_tests PRIVATE
    ddos_core
    Qt6::Core
    Qt6::Test
    nlohmann_json::nlohmann_json
)

# Run windeployqt for tests
add_custom_command(TARGET ddos_unit_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Running windeployqt for ddos_unit_tests"
    COMMAND Qt6::windeployqt --test "$<TARGET_FILE:ddos_unit_tests>"
)

add_custom_command(TARGET db_benchmark_qt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Running windeployqt for db_benchmark_qt"
    COMMAND Qt6::windeployqt --test "$<TARGET_FILE:db_benchmark_qt>"
)

# Copy onnxruntime.dll and libpq.dll to tests directory
if(ONNXRUNTIME_FOUND AND EXISTS "${ONNXRUNTIME_DLL}")
    add_custom_command(TARGET ddos_unit_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ONNXRUNTIME_DLL}" "$<TARGET_FILE_DIR:ddos_unit_tests>"
    )
endif()

# We need LIBPQ_DLL_PATH from root, but it's not exported. 
# Let's find it again or assume it's in vcpkg
find_file(LIBPQ_DLL_TEST libpq.dll PATHS "C:/Dev/vcpkg/installed/x64-windows/bin")
if(LIBPQ_DLL_TEST)
    add_custom_command(TARGET ddos_unit_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIBPQ_DLL_TEST}" "$<TARGET_FILE_DIR:ddos_unit_tests>"
    )
    add_custom_command(TARGET db_benchmark_qt POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIBPQ_DLL_TEST}" "$<TARGET_FILE_DIR:db_benchmark_qt>"
    )
endif()

# Optional: Add any definitions if needed
# target_compile_definitions(ddos_unit_tests PRIVATE ...)

add_test(NAME ddos_unit_tests COMMAND ddos_unit_tests)
