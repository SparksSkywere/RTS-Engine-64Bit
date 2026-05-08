#include "benchmark.h"

int RunBenchmark(int argc, char *argv[], float *floatResult, float *intResult, float *memResult)
{
    (void)argc;
    (void)argv;

    if (floatResult) {
        *floatResult = 0.0f;
    }
    if (intResult) {
        *intResult = 0.0f;
    }
    if (memResult) {
        *memResult = 0.0f;
    }

    return 0;
}
