#include <stdlib.h>
#include <string.h>

int main(void) {
    const size_t chunk = 1024 * 1024;
    for (;;) {
        void *p = malloc(chunk);
        if (!p) return 2;
        memset(p, 0xA5, chunk);
    }
}
