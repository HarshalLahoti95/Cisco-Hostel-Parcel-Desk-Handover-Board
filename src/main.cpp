// STEP 5 — not implemented yet. Exits 2 so the CLI scenarios fail loudly
// rather than appearing to succeed.
#include <cstdio>

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    std::fprintf(stderr, "handover: CLI not implemented (step 5)\n");
    return 2;
}
