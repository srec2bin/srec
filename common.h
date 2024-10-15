#ifndef COMMON_H
#define COMMON_H

#define SREC_VER "1.21"

#include <stdio.h>
#include <stdint.h>

#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
#define TRUE (~FALSE)
#endif

#define max(a,b) (((a)>(b))?(a):(b))
#define min(a,b) (((a)<(b))?(a):(b))

unsigned int char_to_uint(char s);
uint32_t str_to_uint32(char *s);
uint32_t file_size(FILE *f);

#endif
