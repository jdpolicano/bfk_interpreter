#ifndef DYSTRING_IMP
#define DYSTRING_IMP

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

typedef struct {
  char* data;
  size_t size;
  size_t capacity;
} dystring_t;

dystring_t* get_dystring();
dystring_t* dystring_read_file(char* filepath);
int init_dystring(dystring_t *s);
int dystring_push_cstr(char *data, dystring_t *s);
int dystring_push(char c, dystring_t *s);
void dystring_pop(dystring_t *s);
int dystring_filter(dystring_t *src, dystring_t *dest, char* exclude);
int dystring_filter_except(dystring_t *src, dystring_t *dest, char* include);
int null_terminate_buffer(int ret_code, dystring_t *s);
int dystring_expand(size_t req_size, dystring_t *s);
int prepare_push(size_t data_size, dystring_t *s);
int alloc_err(size_t size_failed);
void free_dystring(dystring_t *s);


#endif
