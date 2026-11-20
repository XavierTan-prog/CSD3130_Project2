cmake_minimum_required(VERSION 3.20)
project(seam_carving CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs highgui)
find_package(Catch2 3 REQUIRED)

# Core library: everything except the entry points.
add_library(seamlib
    src/energy.cpp
    src/seam.cpp
    src/dp.cpp
    src/greedy.cpp
    src/graph_cut.cpp
    src/carve.cpp
    src/visualize.cpp
    src/paths.cpp
    # Headers are listed so they show up in the Visual Studio solution.
    include/seam_carving/energy.hpp
    include/seam_carving/seam.hpp
    include/seam_carving/dp.hpp
    include/seam_carving/greedy.hpp
    include/seam_carving/graph_cut.hpp
    include/seam_carving/carve.hpp
    include/seam_carving/visualize.hpp
    include/seam_carving/paths.hpp
)
target_include_directories(seamlib PUBLIC include)
# Lets the programs find images/ even when started from another folder.
target_compile_definitions(seamlib PRIVATE SC_PROJECT_DIR="${CMAKE_SOURCE_DIR}")
target_link_libraries(seamlib PUBLIC ${OpenCV_LIBS})
if(MSVC)
    target_compile_options(seamlib PRIVATE /W4)
else()
    target_compile_options(seamlib PRIVATE -Wall -Wextra)
endif()

# Command-line resizer.
add_executable(seam_carve src/main.cpp)
target_link_libraries(seam_carve PRIVATE seamlib)

# Timing comparison for the report.
add_executable(benchmark bench/benchmark.cpp)
target_link_libraries(benchmark PRIVATE seamlib)

# Visual Studio launch settings. run.bat only opens the solution; running
# is done with F5.
#   seam_carve  with no arguments asks which image, size and method to use.
#   benchmark   <input> <seams-to-remove>
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT seam_carve)
set_target_properties(seam_carve PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set_target_properties(benchmark PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    VS_DEBUGGER_COMMAND_ARGUMENTS "images/input/sample.png 10")

# Unit tests.
enable_testing()
add_executable(tests tests/test_seams.cpp)
target_link_libraries(tests PRIVATE seamlib Catch2::Catch2WithMain)
add_test(NAME tests COMMAND tests)

# Results harness (bench/results): figures and timings for the report.
# Not part of the assignment program. Run bench/results/run_results.bat.
foreach(name eval gcbig grid)
    add_executable(results_${name} bench/results/${name}.cpp)
    target_link_libraries(results_${name} PRIVATE seamlib)
    set_target_properties(results_${name} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endforeach()
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    add_executable(results_omp bench/results/omp.cpp)
    target_link_libraries(results_omp PRIVATE seamlib OpenMP::OpenMP_CXX)
    set_target_properties(results_omp PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()
