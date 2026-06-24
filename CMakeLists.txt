cmake_minimum_required(VERSION 3.14)

project(TradeBot-AIIO-Core
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

find_package(Threads REQUIRED)
find_library(DL_LIB dl)

set(CORE_SOURCES
    src/CsvReader.cpp
    src/AsyncNetworkClient.cpp
    src/AuthManager.cpp
    src/LiveDataAdapter.cpp
    src/BrokerGateway.cpp
    src/EventLoop.cpp
    src/RegimeDetector.cpp
    src/RiskEngine.cpp
    src/SmaCrossStrategy.cpp
    src/MeanReversionStrategy.cpp
    src/PortfolioAllocator.cpp
    src/PortfolioManager.cpp
    src/ExecutionEngine.cpp
    src/AnalyticsEngine.cpp
    src/MetricsAggregator.cpp
    src/LocalMetricsExporter.cpp
    src/StateSerializer.cpp
    src/LocalDataReplayAdapter.cpp
    src/L2OrderBook.cpp
    src/TriggerOrderManager.cpp
    src/OrderLifecycleStore.cpp
)

add_library(tradebot_core_lib ${CORE_SOURCES})
target_include_directories(tradebot_core_lib PUBLIC include)
target_link_libraries(tradebot_core_lib PUBLIC Threads::Threads)
if (DL_LIB)
    target_link_libraries(tradebot_core_lib PUBLIC ${DL_LIB})
endif()

add_executable(tradebot_core src/main.cpp)
target_link_libraries(tradebot_core PRIVATE tradebot_core_lib)

add_executable(throughput_bench src/benchmarks/throughput_bench.cpp)
target_link_libraries(throughput_bench PRIVATE tradebot_core_lib)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(throughput_bench PRIVATE -O3 -DNDEBUG -march=native)
elseif (MSVC)
    target_compile_options(throughput_bench PRIVATE /O2 /DNDEBUG)
endif()

add_executable(phase18_burnin src/benchmarks/phase18_burnin.cpp)
target_link_libraries(phase18_burnin PRIVATE tradebot_core_lib)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(phase18_burnin PRIVATE -O3 -DNDEBUG -march=native)
elseif (MSVC)
    target_compile_options(phase18_burnin PRIVATE /O2 /DNDEBUG)
endif()

add_executable(apply_bbo_microbench src/benchmarks/apply_bbo_microbench.cpp)
target_link_libraries(apply_bbo_microbench PRIVATE tradebot_core_lib)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(apply_bbo_microbench PRIVATE -O3 -DNDEBUG -march=native)
elseif (MSVC)
    target_compile_options(apply_bbo_microbench PRIVATE /O2 /DNDEBUG)
endif()

include(CTest)
if (BUILD_TESTING)
    add_executable(phase13_tests tests/phase13_tests.cpp)
    target_link_libraries(phase13_tests PRIVATE tradebot_core_lib)
    add_test(NAME phase13_tests COMMAND phase13_tests)

    add_executable(phase15_tests tests/phase15_tests.cpp)
    target_link_libraries(phase15_tests PRIVATE tradebot_core_lib)
    add_test(NAME phase15_tests COMMAND phase15_tests)

    add_executable(phase16_tests tests/phase16_tests.cpp)
    target_link_libraries(phase16_tests PRIVATE tradebot_core_lib)
    add_test(NAME phase16_tests COMMAND phase16_tests)

    add_executable(phase17_tests tests/phase17_tests.cpp)
    target_link_libraries(phase17_tests PRIVATE tradebot_core_lib)
    add_test(NAME phase17_tests COMMAND phase17_tests)

    add_executable(phase18_tests tests/phase18_tests.cpp)
    target_link_libraries(phase18_tests PRIVATE tradebot_core_lib)
    add_test(NAME phase18_tests COMMAND phase18_tests)

    add_executable(phase22_tests tests/phase22_tests.cpp)
    target_link_libraries(phase22_tests PRIVATE tradebot_core_lib)
    add_test(NAME phase22_tests COMMAND phase22_tests)
endif()
