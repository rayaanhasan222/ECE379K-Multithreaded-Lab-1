#include <cstdio>
#include "locks.h"
int main() {
    std::printf("TTASLock size=%zu alignment=%zu CACHE_LINE=%zu\n",
                sizeof(TTASLock), alignof(TTASLock), CACHE_LINE);
}
