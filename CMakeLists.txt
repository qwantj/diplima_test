cmake_minimum_required(VERSION 3.16)

project(DiplomDDoS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ---------- Qt6 ----------
# Prevent Qt from finding Vulkan headers in msys64 (causes MSVC/MinGW header conflict)
set(CMAKE_IGNORE_PATH "C:/msys64/mingw64/include" "C:/msys64/mingw64/lib")
find_package(Qt6 REQUIRED COMPONENTS Core Network Sql Widgets Charts Test)

# ---------- PcapPlusPlus (vcpkg) ----------
find_package(PcapPlusPlus REQUIRED CONFIG)

# ---------- PostgreSQL (vcpkg) ----------
find_package(PostgreSQL REQUIRED)

# ---------- ONNX Runtime (manual install) ----------
set(ONNXRUNTIME_ROOT "C:/Lib/onnxruntime" CACHE PATH "ONNX Runtime install root")
if(EXISTS "${ONNXRUNTIME_ROOT}/include")
    set(ONNXRUNTIME_FOUND TRUE)
    set(ONNXRUNTIME_INCLUDE_DIRS "${ONNXRUNTIME_ROOT}/include")
    set(ONNXRUNTIME_LIB "${ONNXRUNTIME_ROOT}/lib/onnxruntime.lib")
    set(ONNXRUNTIME_DLL "${ONNXRUNTIME_ROOT}/lib/onnxruntime.dll")
else()
    message(WARNING "ONNX Runtime not found at ${ONNXRUNTIME_ROOT}. Building without ML support.")
    set(ONNXRUNTIME_FOUND FALSE)
endif()

# ---------- spdlog (FetchContent) ----------
include(FetchContent)
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.15.2
)
FetchContent_MakeAvailable(spdlog)

# ---------- nlohmann/json (FetchContent) ----------
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# =========================================================
#  ddos_core  (static library)
# =========================================================
add_library(ddos_core STATIC
    src/common/AppLogger.cpp
    src/common/Protocol.cpp
    src/common/DatabaseManager.cpp
    src/common/FileBuffer.cpp
    src/common/SystemMetricsCollector.cpp
    src/common/TcpServer.cpp
    src/common/TcpClient.cpp
    src/common/DataBridge.cpp
    src/common/ConfigManager.cpp
    src/network/TrafficMonitor.cpp
    src/network/FeatureExtractor.cpp
    src/network/PcapDumper.cpp
    src/ml/ModelInferencer.cpp
    src/core/DetectionEngine.cpp
    src/core/FirewallManager.cpp
)

target_include_directories(ddos_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ddos_core PUBLIC
    Qt6::Core
    Qt6::Network
    Qt6::Sql
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    PcapPlusPlus::Pcap++
    PostgreSQL::PostgreSQL
)

if(ONNXRUNTIME_FOUND)
    target_include_directories(ddos_core PUBLIC ${ONNXRUNTIME_INCLUDE_DIRS})
    target_link_libraries(ddos_core PUBLIC ${ONNXRUNTIME_LIB})
    target_compile_definitions(ddos_core PUBLIC ONNX_RUNTIME_AVAILABLE)
endif()

# =========================================================
#  ddos_collector  (console executable)
# =========================================================
add_executable(ddos_collector
    src/collector_main.cpp
)

target_link_libraries(ddos_collector PRIVATE ddos_core)

# Post-build: copy runtime DLLs
if(ONNXRUNTIME_FOUND AND EXISTS "${ONNXRUNTIME_DLL}")
    add_custom_command(TARGET ddos_collector POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying onnxruntime.dll to collector build directory"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ONNXRUNTIME_DLL}" "$<TARGET_FILE_DIR:ddos_collector>"
    )
endif()

# Copy libpq.dll
find_file(LIBPQ_DLL_PATH libpq.dll PATHS "${PostgreSQL_INCLUDE_DIR}/../bin" "${PostgreSQL_INCLUDE_DIR}/../../bin")
if(LIBPQ_DLL_PATH)
    add_custom_command(TARGET ddos_collector POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying libpq.dll to collector build directory"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIBPQ_DLL_PATH}" "$<TARGET_FILE_DIR:ddos_collector>"
    )
endif()

# windeployqt for collector
add_custom_command(TARGET ddos_collector POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Running windeployqt for ddos_collector"
    # COMMAND Qt6::windeployqt "$<TARGET_FILE:ddos_collector>"
)

# =========================================================
#  ddos_monitor  (GUI executable)
# =========================================================
add_executable(ddos_monitor
    src/monitor_main.cpp
    src/monitor_ui/DashboardWidget.cpp
    src/monitor_ui/LogWidget.cpp
    src/monitor_ui/SessionWidget.cpp
    src/monitor_ui/EventHistoryWidget.cpp
    src/monitor_ui/ThemePalette.cpp
    src/monitor_ui/ReportGenerator.cpp
)
target_link_libraries(ddos_monitor PRIVATE
    ddos_core
    Qt6::Widgets
    Qt6::Charts
)

# Copy libpq.dll for monitor
if(LIBPQ_DLL_PATH)
    add_custom_command(TARGET ddos_monitor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying libpq.dll to monitor build directory"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIBPQ_DLL_PATH}" "$<TARGET_FILE_DIR:ddos_monitor>"
    )
endif()

# windeployqt for monitor
add_custom_command(TARGET ddos_monitor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Running windeployqt for ddos_monitor"
    # COMMAND Qt6::windeployqt "$<TARGET_FILE:ddos_monitor>"
)

# =========================================================
#  Tests & Models
# =========================================================
enable_testing()

# --- Copy models and config to build directory ---
add_custom_command(TARGET ddos_collector POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying models and config to collector build directory"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/models"
        "$<TARGET_FILE_DIR:ddos_collector>/models"
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
        "$<TARGET_FILE_DIR:ddos_collector>/config.json"
)

add_custom_command(TARGET ddos_monitor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying models and config to monitor build directory"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/models"
        "$<TARGET_FILE_DIR:ddos_monitor>/models"
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
        "$<TARGET_FILE_DIR:ddos_monitor>/config.json"
)

# =========================================================
#  ddos_unit_tests
# =========================================================
add_subdirectory(tests)
