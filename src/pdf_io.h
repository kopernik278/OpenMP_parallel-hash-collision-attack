#ifndef PDF_IO_H
#define PDF_IO_H

#include <stddef.h>
#include <stdint.h>

#define NONCE_OFFSET 16
#define NONCE_LENGTH 16
#define STUDENT_ID_OFFSET 45
#define STUDENT_ID_LENGTH 8

typedef struct {
    unsigned char *data;
    size_t size;
} pdf_buffer_t;

pdf_buffer_t pdf_load(const char *path);
void pdf_free(pdf_buffer_t *buf);
void pdf_write(const char *path, const unsigned char *data, size_t size);

void pdf_set_nonce(unsigned char *pdf, uint64_t nonce);
void pdf_set_student_id(unsigned char *pdf, const char *id);

#endif
