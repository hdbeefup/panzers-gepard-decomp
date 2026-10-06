// recdump: .rec dump and round trip (src/tools/recdump/CMakeLists.txt).
#include <stdio.h>
#include "packets_rec.h"

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: recdump <in.rec> [<dump.txt> [<out.rec>]]\n");
        return 2;
    }
    int r = pz::PzRecDump(argv[1], argc > 2 ? argv[2] : nullptr, argc > 3 ? argv[3] : nullptr);
    printf("recdump: %s\n", r == 0 ? "ok" : "errors");
    return r;
}
