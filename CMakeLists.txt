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
    # Headers are listed so they show up in the Visual Studio solution.
    include/seam_carving/energy.hpp
    include/seam_carving/seam.hpp
    include/seam_carving/dp.hpp
    include/seam_carving/greedy.hpp
    include/seam_carving/graph_cut.hpp
    include/seam_carving/carve.hpp
)
target_include_directories(seamlib PUBLIC include)
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
# is done with F5. Change the arguments below (then build once, so the
# solution regenerates) to resize a different image or use another method.
#   seam_carve <input> <output> <width> <height> [dp|greedy|graphcut]
#   benchmark  <input> <seams-to-remove>
# The working directory is the repo root, so relative paths work.
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT seam_carve)
set_target_properties(seam_carve PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    VS_DEBUGGER_COMMAND_ARGUMENTS "images/input/sample.png images/output/sample_dp.png 150 120 dp")
set_target_properties(benchmark PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    VS_DEBUGGER_COMMAND_ARGUMENTS "images/input/sample.png 10")

# Unit tests.
enable_testing()
add_executable(tests tests/test_seams.cpp)
target_link_libraries(tests PRIVATE seamlib Catch2::Catch2WithMain)
add_test(NAME tests COMMAND tests)
