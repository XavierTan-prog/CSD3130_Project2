#pragma once

namespace sc {

// Makes relative paths such as images/input/sample.png work no matter
// where the program is started from: if the current folder has no
// images/input but the project folder (recorded at build time) does, the
// working directory is switched to the project folder.
void useProjectDir();

} // namespace sc
