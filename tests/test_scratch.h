// One scratch directory PER TEST PROCESS (CG.TMPRACE).
//
// The pre-push gate runs copies of the test binaries concurrently. Any fixed
// path under the system temp dir is shared by every copy: test_renderer.mm
// wrote and OCRed one fixed PNG, so copy A read copy B's image and failed
// while B passed. Every scratch file a test writes lives under
// TestScratchDir(), made once per process by mkdtemp under $TMPDIR.
// tools/check_test_tmp_paths.py refuses a literal temp path under tests/.
#pragma once

#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

inline const std::string& TestScratchDir() {
    static const std::string dir = [] {
        const char* env = getenv("TMPDIR");
        std::string base = (env != nullptr && *env != '\0') ? env : "/tmp";
        while (base.size() > 1 && base.back() == '/') base.pop_back();
        // Short: AF_UNIX sun_path is 104 bytes and sockets live in here too.
        std::string tmpl = base + "/cg-XXXXXX";
        if (mkdtemp(tmpl.data()) == nullptr) {
            perror("TestScratchDir: mkdtemp");
            abort();
        }
        return tmpl;
    }();
    return dir;
}

// A path inside this process's scratch directory.
inline std::string TestScratchPath(const std::string& name) {
    return TestScratchDir() + "/" + name;
}

// A fresh, unique directory inside this process's scratch directory, for a
// test that needs one of its own. Empty on failure.
inline std::string TestScratchMkdtemp(const std::string& stem) {
    std::string tmpl = TestScratchPath(stem + "_XXXXXX");
    if (mkdtemp(tmpl.data()) == nullptr) return {};
    return tmpl;
}
