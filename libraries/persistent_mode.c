#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

static inline ssize_t readFromFd(int fd, uint8_t * buf, size_t len)
{
    size_t readSz = 0;
    while (readSz < len) {
        ssize_t sz = read(fd, &buf[readSz], len - readSz);
        if (sz < 0 && errno == EINTR)
            continue;

        if (sz == 0)
            break;

        if (sz < 0)
            return -1;

        readSz += sz;
    }
    return (ssize_t) readSz;
}

static inline bool readFromFdAll(int fd, uint8_t * buf, size_t len)
{
    return (readFromFd(fd, buf, len) == (ssize_t) len);
}

static bool writeToFd(int fd, uint8_t * buf, size_t len)
{
    size_t writtenSz = 0;
    while (writtenSz < len) {
        ssize_t sz = write(fd, &buf[writtenSz], len - writtenSz);
        if (sz < 0 && errno == EINTR)
            continue;

        if (sz < 0)
            return false;

        writtenSz += sz;
    }
    return (writtenSz == len);
}

uint8_t buf[_HF_PERF_BITMAP_SIZE_16M];

void HF_ITER(uint8_t ** buf_ptr, size_t * len_ptr)
{
    /*
     * Send the 'done' marker to the parent
     */
    static bool initialized = false;

    if (initialized == true) {
        uint8_t z = 'A';
        if (writeToFd(_HF_PERSISTENT_FD, &z, sizeof(z)) == false) {
            fprintf(stderr, "readFromFdAll() failed\n");
            _exit(1);
        }
    }
    initialized = true;

    uint32_t rlen;
    if (readFromFdAll(_HF_PERSISTENT_FD, (uint8_t *) & rlen, sizeof(rlen)) == false) {
        fprintf(stderr, "readFromFdAll(size) failed\n");
        _exit(1);
    }
    size_t len = (size_t) rlen;
    if (len > _HF_PERF_BITMAP_SIZE_16M) {
        fprintf(stderr, "len (%zu) > buf_size (%zu)\n", len, (size_t) _HF_PERF_BITMAP_SIZE_16M);
        _exit(1);
    }

    if (readFromFdAll(_HF_PERSISTENT_FD, buf, len) == false) {
        fprintf(stderr, "readFromFdAll(buf) failed\n");
        _exit(1);
    }

    *buf_ptr = buf;
    *len_ptr = len;

/* Clear the custom counter */
#if defined(__x86_64__)
#define ARCH_SET_GS 0x1001
#define __NR_arch_prctl 158
    syscall(__NR_arch_prctl, ARCH_SET_GS, 0UL);
#endif
}
