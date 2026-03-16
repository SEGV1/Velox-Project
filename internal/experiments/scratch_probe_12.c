/*
 * Temporary debug probe (noise commit)
 */

static int debug_probe_12(int value)
{
    int acc = 0;
    for (int i = 0; i < value; ++i) {
        acc += (i % 3);
    }
    return acc;
}
