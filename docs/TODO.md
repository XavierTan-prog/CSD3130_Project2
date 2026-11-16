# Assignment 2 To-Do List

Seam carving with dynamic programming, greedy and graph cut.

- **Submission:** Fri 20 Nov, 23:59 (XSite Dropbox)
- **Project quiz:** Tue 24 Nov, during tutorial (closed book, on our own code and report)

Tick items off as they are done and put a name in **Owner**. Item numbers
in brackets refer to the assignment brief.

## Suggested schedule

| Day | Goal |
| --- | --- |
| Mon 16 Nov | Setup finished, tasks assigned, energy map merged |
| Tue 17 Nov | DP and greedy working; graph cut started |
| Wed 18 Nov | Graph cut working; benchmark and comparison images produced; bonus features |
| Thu 19 Nov | Report written; video recorded |
| Fri 20 Nov | Final checks; submit well before 23:59 |
| Sat 21 - Mon 23 Nov | Quiz preparation |
| Tue 24 Nov | Project quiz |

## 1. Setup

- [x] CMake + vcpkg build, `run.bat` / `clean.bat` / `rebuild.bat`
- [x] Project scaffold: headers, placeholder algorithms, seam removal, resize loop
- [x] Interactive `seam_carve` with side-by-side preview window
- [x] Starter unit tests and GitHub Actions workflow
- [ ] Commit and push the latest changes (including the test images)
- [ ] Confirm the GitHub Actions run is green
- [ ] Team ID and members (with SIT IDs) in `README.md`
- [ ] Every teammate has cloned the repo and run `run.bat` successfully
- [ ] Owners assigned for every section below

## 2. Core implementation

| Done | Task | File | Owner |
| --- | --- | --- | --- |
| [ ] | Energy map, e.g. Sobel gradient magnitude (1a). **Do first: everything depends on it.** | `src/energy.cpp` | |
| [ ] | DP minimum seam: fill the table, backtrack (1b) | `src/dp.cpp` | |
| [ ] | Greedy seam: row-by-row local choice (3) | `src/greedy.cpp` | |
| [ ] | Graph-cut seam: build the graph, max-flow, read the cut (4) | `src/graph_cut.cpp` | |

- [ ] Vertical and horizontal seams both work for all three methods (1). The transpose handling is already in `src/carve.cpp`; check the results.
- [ ] Remove the `[.]` tag from each hidden test in `tests/test_seams.cpp` as its method is finished
- [ ] Add tests for edge cases: one-column image, one-row image, ties in energy
- [ ] Check that graph cut and DP give the same seam cost on small images
- [ ] No library content-aware resize function is used anywhere

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
- [ ] Time and space complexity of each method worked out (5b)
- [ ] Improvements investigated (5d):
  - [ ] Do we need the whole DP table? (e.g. keep two rows plus backtracking information)
  - [ ] Does multithreading help? (e.g. the energy map; measure it)
- [ ] Results saved as figures and tables for the report

## 5. Bonus (7, up to 10 marks)

| Done | Task | Owner |
| --- | --- | --- |
| [ ] | Step-by-step visualisation showing every seam as it is removed | |
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

## 8. Submission (9) - Fri 20 Nov, 23:59

- [ ] Final code pushed; `main` builds from a fresh clone with `run.bat`
- [ ] GitHub repo is accessible to the markers
- [ ] **XSite Dropbox "Assignment 2 Report Only":** the report
- [ ] **XSite Dropbox "Assignment 2 Other Materials":** slides and test inputs/outputs, if any
- [ ] One person confirms both uploads went through

## 9. Quiz preparation (6) - Tue 24 Nov

The quiz is closed book, team based, 10-20 MCQs on our submitted code and
report, with no AI tools. The score scales the project mark (1.00 at 80%+,
0.95 at 60-79%, 0.85 at 40-59%, 0.70 below 40%).

- [ ] Everyone has read the whole report
- [ ] Everyone can explain each source file, not only their own
- [ ] Each owner walks the team through their method (recurrence, graph construction, complexity)
- [ ] Team practice: quiz each other on code snippets and report extracts
