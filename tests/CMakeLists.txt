add_executable(ddos_unit_tests
    unit/main.cpp
    unit/ProtocolTests.cpp
)

target_link_libraries(ddos_unit_tests PRIVATE
    ddos_core
    Qt6::Core
    Qt6::Test
    nlohmann_json::nlohmann_json
)

# Optional: Add any definitions if needed
# target_compile_definitions(ddos_unit_tests PRIVATE ...)

add_test(NAME ddos_unit_tests COMMAND ddos_unit_tests)
