#include "pdf_io.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

pdf_buffer_t pdf_load(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "failed to open %s: %s\n", path, strerror(errno));
        exit(1);
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fprintf(stderr, "failed to seek %s\n", path);
        exit(1);
    }
    long size = ftell(f);
    if (size < 0 || (size_t)size < STUDENT_ID_OFFSET + STUDENT_ID_LENGTH) {
        fprintf(stderr, "%s is too small to contain a valid header\n", path);
        exit(1);
    }
    rewind(f);

    unsigned char *data = malloc((size_t)size);
    if (!data) {
        fprintf(stderr, "out of memory loading %s\n", path);
        exit(1);
    }

    if (fread(data, 1, (size_t)size, f) != (size_t)size) {
        fprintf(stderr, "failed to read %s\n", path);
        exit(1);
    }
    fclose(f);

    pdf_buffer_t buf;
    buf.data = data;
    buf.size = (size_t)size;
    return buf;
}

void pdf_free(pdf_buffer_t *buf)
{
    free(buf->data);
    buf->data = NULL;
    buf->size = 0;
}

void pdf_write(const char *path, const unsigned char *data, size_t size)
{
    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "failed to open %s for writing: %s\n", path, strerror(errno));
        exit(1);
    }
    if (fwrite(data, 1, size, f) != size) {
        fprintf(stderr, "failed to write %s\n", path);
        exit(1);
    }
    fclose(f);
}

void pdf_set_nonce(unsigned char *pdf, uint64_t nonce)
{
    static const char hex[] = "0123456789abcdef";

    for (int i = NONCE_LENGTH - 1; i >= 0; --i) {
        pdf[NONCE_OFFSET + i] = (unsigned char)hex[nonce & 0xf];
        nonce >>= 4;
    }
}

void pdf_set_student_id(unsigned char *pdf, const char *id)
{
    memcpy(pdf + STUDENT_ID_OFFSET, id, STUDENT_ID_LENGTH);
}
