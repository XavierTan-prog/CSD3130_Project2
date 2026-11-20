# AI Usage Log

Record every use of AI tools here as it happens. The report needs a
declaration of how AI was used and the main prompts.

| Date | Stage | Tool | Main prompt (summary) | What was used from the output |
| --- | --- | --- | --- | --- |
| 2026-11-16 | Setup | Claude | "What do I need to set up in my repo before implementation?" then "How do I set it up in C++?" | Project structure and tooling choices (CMake, vcpkg, OpenCV, Catch2) |
| 2026-11-16 | Setup | Claude | "Help me create these [scaffold files]" | Repo scaffold: CMake build, headers, placeholder algorithm stubs, seam-removal helper, CLI, benchmark, starter tests, CI workflow |
| 2026-11-16 | Setup | Claude | "Create a run.bat that builds into a build folder and runs the commands" | `run.bat` build-and-run script |
| 2026-11-16 | Setup | Claude | "Make it build a solution file so I can edit the project" | `run.bat sln` command and Visual Studio settings in `CMakeLists.txt` |
| 2026-11-16 | Setup | Claude | "Make run.bat need no arguments; launch the commands from the project instead" | Argument-free `run.bat`, Visual Studio launch arguments in `CMakeLists.txt`, synthetic `sample.png` |
| 2026-11-16 | Setup | Claude | "Add a clean and a rebuild batch file" | `clean.bat`, `rebuild.bat` |
| 2026-11-16 | Setup | Claude | Pasted the vcpkg "requires a manifest with a specified baseline" error and asked what it means | `builtin-baseline` in `vcpkg.json`; `run.bat` prefers Visual Studio's own CMake |
| 2026-11-16 | Setup | Claude | "The first run is very slow, what are my options to speed it up?" | Trimmed OpenCV feature list in `vcpkg.json` |
| 2026-11-16 | Setup | Claude | "Make it possible to display the image in an output visual before the work is split" | `visualize.hpp/.cpp` (side-by-side view, seam overlay, window), preview in `main.cpp`, two tests |
| 2026-11-16 | Setup | Claude | Pasted the "could not read image" error and asked for the program to take an input to choose the image | Interactive prompts in `main.cpp`, `paths.hpp/.cpp` working-directory fix |
| 2026-11-16 | Setup | Claude | Screenshot of the preview window being too big for the screen; asked to make it smaller | Fit-to-screen scaling in `sideBySide`, one test |
| 2026-11-16 | Setup | Claude | "Repoint the benchmark and README to the new image and add the small sample back" | Updated launch arguments and README; restored `sample.png` |
| 2026-11-16 | Planning | Claude | "Draft a to-do list for this assignment and put it in a docs folder" | `docs/TODO.md` |
| 2026-11-16 | Planning | Claude | "Check the pushes and rewrite the setup part of the to-do so teammates know what to do" | Setup instructions in `docs/TODO.md` |
| 2026-11-16 | CI/CD | Claude | "What is needed for the CI/CD bonus? Add the badge and tell me how to set up branch protection" | CI status badge in `README.md`; branch protection steps |
| 2026-11-17 | Energy map (1a) | Claude | "Implement only the energy-map task from `docs/claude_energy_context.md`": grayscale Sobel energy with a baseline Sobel L1 (`\|gx\| + \|gy\|`, default) and a Sobel L2 (`sqrt(gx^2 + gy^2)`) option, Doxygen docs , step comments, focused tests, keep `computeEnergy(img)` working | `EnergyOperator` enum and documented `computeEnergy` in `energy.hpp`, implementation in `energy.cpp`, six energy-map tests in `tests/test_seams.cpp` |
| 2026-11-20 | DP (1b) and greedy (3) | Claude | Asked how the DP and greedy seam finders differ for this project, then asked for both to be implemented in the existing `findSeamDP` and `findSeamGreedy` stubs | `findSeamDP` in `src/dp.cpp` (table fill with the recurrence, backtracking, leftmost tie-break) and `findSeamGreedy` in `src/greedy.cpp` (start at the cheapest top pixel, cheapest of 3 below each row); un-hid the DP and greedy tests and added tests for the greedy trap case, DP vs brute force, one-column and one-row images, ties and invalid input |
| 2026-11-20 | Graph cut (4) | Claude | Asked for the graph-cut seam finder to be implemented in the `findSeamGraphCut` stub, following the min-cut construction outlined in its comments | `findSeamGraphCut` in `src/graph_cut.cpp` (pixel graph with source/sink, forward edges weighted by energy, infinite backward and diagonal edges, last column linked to the sink, Edmonds-Karp max-flow, seam read from the residual graph); un-hid the graph-cut test and added tests for matching the DP cost, a seam in the last column, one-column/one-row images and invalid input; benchmark launch argument switched to `sample.png` |
| 2026-11-20 | Testing DP and greedy | Claude | Asked whether both seam directions and ties are handled, and for extra tests covering DP and greedy | Four tests in `tests/test_seams.cpp`: leftmost tie-break while backtracking, horizontal seams via the transposed map, carving width and height together, and DP vs brute force (greedy never cheaper) on 50 random maps |
| 2026-11-20 | Bonus: step-by-step visualisation (7) | Claude | Asked for the step-by-step visualisation bonus, showing every seam as it is removed | Seam callback overload of `carve()` in `carve.hpp/.cpp`; `drawHorizontalSeam`, `renderSeamStep` and `SeamStepViewer` (Space pause, N step, Esc skip) in `visualize.hpp/.cpp`; "Show each seam" prompt and `--steps[=MS]` option in `main.cpp`; six tests for the callback, horizontal seam drawing and the rendered frame; README and TODO updated |
