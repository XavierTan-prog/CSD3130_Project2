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
