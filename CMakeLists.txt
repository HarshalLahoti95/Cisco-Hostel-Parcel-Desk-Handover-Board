cmake_minimum_required(VERSION 3.16)
project(hostel_handover CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

add_compile_options(-Wall -Wextra -Wpedantic)

# The rules engine: pure, no file I/O. CL-037 depends on this staying that way.
add_library(hd_core STATIC
  src/model.cpp
  src/normalize.cpp
  src/validate.cpp
  src/engine.cpp
  src/handover.cpp
  src/report.cpp
)
target_include_directories(hd_core PUBLIC src)

# File I/O, isolated to one translation unit (CL-031, CL-039).
add_library(hd_io STATIC src/csv.cpp)
target_link_libraries(hd_io PUBLIC hd_core)

add_executable(handover src/main.cpp)
target_link_libraries(handover PRIVATE hd_io)

add_executable(run_tests
  tests/test_main.cpp
  tests/harness.cpp
  tests/fixtures.cpp
  tests/test_a_acceptance.cpp
  tests/test_b_validation.cpp
  tests/test_c_arrive.cpp
  tests/test_d_collect.cpp
  tests/test_e_ordering.cpp
  tests/test_f_report_cli.cpp
  tests/test_g_invariants.cpp
  tests/test_h_csv_cli.cpp
)
target_link_libraries(run_tests PRIVATE hd_io)
target_compile_definitions(run_tests PRIVATE HANDOVER_BIN="$<TARGET_FILE:handover>")
add_dependencies(run_tests handover)

enable_testing()
add_test(NAME scenarios COMMAND run_tests)
