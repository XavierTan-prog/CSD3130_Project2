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
| 2026-11-19 | Status review | Codex | "Can you tell me what has been completed and what is the part that I need to do?" | Checked the assignment, repository, TODO, and starter tests to identify the remaining algorithms. |
| 2026-11-19 | Graph Cut (4) | Codex | "Inspect the existing project architecture, reuse its existing infrastructure, implement Graph Cut using min-cut/max-flow, and verify that it works correctly." Use Edmonds-Karp, handle the last column, and verify against a brute-force seam oracle and the CLI. | Graph Cut implementation and boundary explanation, Catch2 coverage, sample benchmark setup, and Release build, resize, and timing checks. |
| 2026-11-19 | Graph Cut performance (4) | Codex | "Optimize the performance of our CSD3130 Assignment 2 Graph Cut seam-carving implementation." Profile the Release build, compare exact max-flow solvers and graph storage, and keep the previous version if the candidate is slower. | Reserved adjacency lists and iterative Dinic with levels measured from the sink. Median of three 100-by-39 sample resizes went from 26.35 s to 11.25 s with identical output. Verified with the existing suite, the 4-by-4 example, and 560 exhaustive comparisons. |
