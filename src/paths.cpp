#include "seam_carving/paths.hpp"

#include <filesystem>
#include <system_error>

namespace sc {

void useProjectDir() {
#ifdef SC_PROJECT_DIR
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path project = SC_PROJECT_DIR;
    if (!fs::exists("images/input", ec) && fs::exists(project / "images" / "input", ec)) {
        fs::current_path(project, ec);
    }
#endif
}

} // namespace sc
