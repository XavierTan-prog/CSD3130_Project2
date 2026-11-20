# Assignment 2 To-Do List

Seam carving with dynamic programming, greedy and graph cut.

- **Submission:** Sat 21 Nov, 23:59 (XSite Dropbox; the brief says Fri 20 Nov, but the dropboxes are due Sat 21 Nov and close Mon 23 Nov)
- **Project quiz:** Tue 24 Nov, during tutorial (closed book, on our own code and report)

Tick items off as they are done and put a name in **Owner**. Item numbers
in brackets refer to the assignment brief.

## Suggested schedule

| Day | Goal |
| --- | --- |
| Mon 16 Nov | Setup finished, tasks assigned, energy map merged |
| Tue 17 Nov | DP and greedy working; graph cut started |
| Wed 18 Nov | Graph cut working; benchmark and comparison images produced; bonus features |
| Fri 20 Nov (actual) | DP, greedy and graph cut done, all 31 tests pass. Remaining: evaluation figures, report, video, submission |
| Thu 19 Nov | Report written; video recorded |
| Sat 21 Nov | Final checks; submit well before 23:59 |
| Sun 22 - Mon 23 Nov | Quiz preparation |
| Tue 24 Nov | Project quiz |

## 1. Setup (every teammate does this once)

The repo is already scaffolded and pushed. Each person follows the steps
below on their own machine.

### 1.1 Install the tools

- [ ] **Visual Studio 2022 (version 17.6 or newer) or Visual Studio 2026**, with the **"Desktop development with C++"** workload. This includes the compiler, CMake and vcpkg, so nothing else needs installing.
- [ ] **Git** (or GitHub Desktop).

### 1.2 Get the code

- [ ] Clone the repo: `git clone https://github.com/XavierTan-prog/CSD3130_Project2.git`
- [ ] Put it in a normal folder such as `C:\dev\`, **not** inside OneDrive, Google Drive or Dropbox. Syncing slows the build a lot and can corrupt the Git data.

### 1.3 Generate the solution

- [ ] Double-click **`run.bat`** in the repo folder. It needs no arguments.
- [ ] Wait for it to finish. **The first run compiles OpenCV and takes a long time**, with long silent stretches at `Building x64-windows-dbg` and `Building x64-windows-rel`. This is normal. Later runs take seconds.
- [ ] Do not click inside the black console window while it runs: that pauses it (the title bar shows "Select"). Press Enter to resume if it happens.
- [ ] When it finishes, Visual Studio opens `build\seam_carving.sln` by itself.

### 1.4 Check that it works

- [ ] In Visual Studio: **Build > Build Solution**. It should finish with no errors.
- [ ] Press **Ctrl+F5** (startup project `seam_carve`). Press Enter at each prompt to accept the defaults. A window should open showing the original image next to the result.
  - The default DP result looks like a plain crop until DP is implemented.
- [ ] Right-click the **`tests`** project > **Set as Startup Project**, press **Ctrl+F5**. Expect `All tests passed`. The DP and greedy tests stay hidden until their methods are ready.
- [ ] Set `seam_carve` back as the startup project.

### 1.5 If something goes wrong

| Problem | What to do |
| --- | --- |
| `CMake was not found` | The C++ workload is missing. Add it in the Visual Studio Installer. |
| `vcpkg was not found` | Visual Studio 2022 is older than 17.6. Update it, or install vcpkg at `C:\dev\vcpkg`. |
| Build is broken or behaving oddly | Close Visual Studio, then double-click **`rebuild.bat`** (deletes `build\` and regenerates everything). |
| `clean.bat` says it could not delete `build\` | Close Visual Studio first; it locks the files. |
| A new source file does not appear in the solution | Add it to `CMakeLists.txt`, then build. Do not add files through the solution; it is regenerated. |

### 1.6 How we work in this repo

- [ ] Add your name and SIT ID to the **Team** line in `README.md`.
- [ ] Put your name in the **Owner** column for your tasks in the sections below.
- [ ] Work on your own branch, e.g. `git checkout -b dp`, and pull `main` before you start each session.
- [ ] Before merging into `main`: the solution builds, the tests pass, and the GitHub Actions check on your push is green.
- [ ] When you finish a method, delete the `[.]` tag from its hidden test in `tests/test_seams.cpp`.
- [ ] Log every AI prompt you use in `report/ai_usage.md` as you go. The report needs them.
- [ ] Only `build\` and the output images are ignored by Git. Do not commit anything from `build\`.

### 1.7 Team-level setup

- [x] Scaffold, build scripts, preview window, starter tests and CI workflow pushed to `main`
- [ ] GitHub Actions run on `main` confirmed green
- [ ] All teammates added as collaborators on the GitHub repo
- [ ] Owners assigned for every section below

## 2. Core implementation

| Done | Task | File | Owner |
| --- | --- | --- | --- |
| [x] | Energy map, e.g. Sobel gradient magnitude (1a). **Do first: everything depends on it.** | `src/energy.cpp` | Abhishek |
| [x] | DP minimum seam: fill the table, backtrack (1b) | `src/dp.cpp` | Nelson |
| [x] | Greedy seam: row-by-row local choice (3) | `src/greedy.cpp` | Nelson |
| [x] | Graph-cut seam: build the graph, max-flow (Dinic), read the cut (4) | `src/graph_cut.cpp` | chuuniby |

- [x] Vertical and horizontal seams both work for all three methods (1). Horizontal seams use the transpose in `src/carve.cpp`; tested for DP and greedy, and checked on the tower photo (1428x968 to 1428x868 with DP).
- [x] Remove the `[.]` tag from each hidden test in `tests/test_seams.cpp` as its method is finished (no hidden tests left)
- [x] Add tests for edge cases: one-column image, one-row image, ties in energy (also: greedy trap case, DP vs brute force, 50 random maps, horizontal seams, last-column seam for graph cut)
- [x] Check that graph cut and DP give the same seam cost on small images (tested on 3 maps)
- [x] No library content-aware resize function is used anywhere
- [x] `benchmark` launch argument switched to `images/input/sample.png 10` (graph cut is too slow for the tower photo)

## 3. Theory questions (2)

| Done | Question | Owner |
| --- | --- | --- |
| [ ] | 2a(i): show the number of seams grows at least exponentially in m (n > 1) | |
| [ ] | 2a(ii): the DP subproblem relation (recurrence) for the minimum seam | |
| [ ] | 2b: does divide and conquer improve the convolution / gradient computation? | |

## 4. Evaluation and comparison (3, 4, 5)

- [ ] Choose the test images (at least one small image for graph cut)
- [ ] Seams compared: overlay the seams each method picks on the same image (`drawSeam` in `src/visualize.cpp`)
- [ ] Visual quality compared: DP vs greedy vs graph cut outputs at the same target size, plus plain cropping and scaling for reference
- [ ] Running time compared with `benchmark` (Release build), across a few image sizes
  - First result, `sample.png` (200x150), 10 seams: greedy 6 ms, DP 13 ms, graph cut 1,791 ms. Still needed: more sizes (e.g. 100x75, 400x300), and DP/greedy alone on the tower photo.
- [ ] Time and space complexity of each method worked out (5b)
- [ ] Improvements investigated (5d):
  - [ ] Do we need the whole DP table? (e.g. keep two rows plus backtracking information)
  - [ ] Does multithreading help? (e.g. the energy map; measure it)
- [ ] Results saved as figures and tables for the report

## 5. Bonus (7, up to 10 marks)

| Done | Task | Owner |
| --- | --- | --- |
| [x] | Step-by-step visualisation showing every seam as it is removed (`seam_carve`: answer "y" to "Show each seam", or `--steps`; Space pause, N next, Esc skip) | Nelson |
| [ ] | Interactive resizing: user changes the image size live (e.g. trackbars) | |
| [ ] | CI/CD: workflow builds and tests on every push (started; keep it green) | |

## 6. Report (9a)

- [ ] Team ID and team members with SIT IDs
- [ ] Answers to the theory questions (section 3 above)
- [ ] Algorithm, design and implementation of each method
- [ ] Results and evaluation, with figures
- [ ] Comparison and discussion: DP vs greedy vs graph cut
- [ ] Time and space complexity
- [ ] Improvements (DP table, multithreading)
- [ ] GitHub link
- [ ] Presentation video link
- [ ] AI usage declaration with the main prompts (from `report/ai_usage.md`; everyone adds their own)
- [ ] Sources credited, including the test photos
- [ ] Proofread by someone who did not write that section

## 7. Presentation (8)

- [ ] Slides (optional) covering the key content only
- [ ] Each person speaks for at most 2 minutes; total at most 12 minutes
- [ ] Live demo showing the program running successfully
- [ ] Recorded and uploaded (e.g. YouTube); link works for someone outside the team
- [ ] Link added to the report

## 8. Submission (9) - Sat 21 Nov, 23:59

- [ ] Final code pushed; `main` builds from a fresh clone with `run.bat`
- [ ] GitHub repo is accessible to the markers
- [ ] **XSite Dropbox "Assignment 2 Report Only":** the report
- [ ] **XSite Dropbox "Assignment 2 Sources and Other Materials":** slides and test inputs/outputs, if any
- [ ] One person confirms both uploads went through

## 9. Quiz preparation (6) - Tue 24 Nov

The quiz is closed book, team based, 10-20 MCQs on our submitted code and
report, with no AI tools. The score scales the project mark (1.00 at 80%+,
0.95 at 60-79%, 0.85 at 40-59%, 0.70 below 40%).

- [ ] Everyone has read the whole report
- [ ] Everyone can explain each source file, not only their own
- [ ] Each owner walks the team through their method (recurrence, graph construction, complexity)
- [ ] Team practice: quiz each other on code snippets and report extracts
