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

size_t const INIT_SIZE = 16;
char* const SIZE_ERR = "Failed to allocate string memory";

dystring_t* get_dystring() {
  dystring_t *s = malloc(sizeof(dystring_t));
  if (s == NULL) {
    alloc_err(sizeof(dystring_t)); // print memory error.
    return NULL;
  }

  if (init_dystring(s) < 0) {
    return NULL;
  }

  return s;
}

// builds a dystring from a file path
dystring_t* dystring_read_file(char* filepath) {
  FILE *fp = fopen(filepath, "r");
  if (fp == NULL) {
    printf("[ERROR]: %s\n", strerror(errno));
    return NULL;
  }

  dystring_t *s = get_dystring();
  if (s == NULL) {
    return s;
  }

  size_t chunk_size = 4096; // should be generally good for block sizes;
  if (prepare_push(chunk_size, s) < 0) {
    return NULL;
  }

  size_t bytes_read = 0;
  while ((bytes_read = fread(s->data + s->size, sizeof(char), chunk_size, fp))) {
      // prepare for the next n bytes
      s->size += bytes_read;
      if (prepare_push(chunk_size, s) < 0) {
        return NULL;
      }
  }

  if (null_terminate_buffer(1, s) < 0) {
    return NULL;
  }

  return s;
}

// initalizes an empty dystring with a new dyanmic array
int init_dystring(dystring_t *s) {
  char* data = malloc(sizeof(char) * INIT_SIZE);

  if (data == NULL) {
    return alloc_err(INIT_SIZE);
  }

  s->data = data;
  s->size = 0;
  s->capacity = INIT_SIZE;

  return INIT_SIZE;
}

// pushs a null terminated string into the dystring returning the number of bytes written or a negative number indicating failure.
int dystring_push_cstr(char *data, dystring_t *s) {
  size_t data_size = strlen(data);
  // must add 1 extra character for the null terminated byte.
  if (prepare_push(data_size, s) < 0) {
   return -1;
  }
  // loop through the data and write the chars to the buffer.
  for (size_t i = 0; i < data_size; i++) {
    s->data[s->size] = data[i];
    s->size += 1;
  }

  return null_terminate_buffer(data_size, s);
}

// pushs in a single character to the dystring 
int dystring_push(char c, dystring_t *s) {
  if (prepare_push(1, s) < 0) {
    return -1;
  };
  s->data[s->size] = c;
  s->size += 1;
  return null_terminate_buffer(1, s); 
}

void dystring_pop(dystring_t *s) {
  if (s->size > 0) {
    s->size -= 1;
    s->data[s->size] = '\0';
  }
}

int dystring_filter(dystring_t *src, dystring_t *dest, char* exclude) {
  for (size_t i = 0; i < src->size; i++) {
    bool matched = false;
    for (size_t j = 0; j < strlen(exclude); j++) {
      if (src->data[i] == exclude[j]) {
        matched = true;
        break;
      }
    }

    if (!matched) {
      if (dystring_push(src->data[i], dest) < 0) {
        return -1;
      }
    }
  }

  return dest->size;
}

int dystring_filter_except(dystring_t *src, dystring_t *dest, char* include) {
    for (size_t i = 0; i < src->size; i++) {
    bool matched = false;
    for (size_t j = 0; j < strlen(include); j++) {
      if (src->data[i] == include[j]) {
        matched = true;
        break;
      }
    }

    if (matched) {
      if (dystring_push(src->data[i], dest) < 0) {
        return -1;
      }
    }
  }
  return dest->size;
}

// passthrough function that adds a null terminated element to the buffer to maintain it as a cstr.
// if it encounters an error, it will return the error, otherwise it will return ret_code. 
int null_terminate_buffer(int ret_code, dystring_t *s) {
  if (s->size == s->capacity) {
    if (prepare_push(1, s) < 0) {
      return -1;
    }
  }
  s->data[s->size] = '\0';
  return ret_code;
}

// prepares a dystring for a push operation by checking size requirements. Returns the new capacity of the string if successful and
// a negative number otherwise.
int prepare_push(size_t data_size, dystring_t *s) {
  // check if we need more space for the string
  return dystring_expand(s->size + data_size, s);
}


int dystring_expand(size_t req_size, dystring_t *s) {
  if (req_size <= s->capacity) {
    return 0;
  }
  
  size_t original_capacity = s->capacity;
  // start doubling capacity until we are greater than the req_size
  while (req_size > s->capacity) {
    s->capacity *= 2;
  }

  char* n_data = realloc(s->data, sizeof(char) * s->capacity);
  if (n_data == NULL) {
    return alloc_err(s->capacity);
  }

  s->data = n_data;
  // return the number of new bytes allocated;
  return s->capacity - original_capacity;
}


int alloc_err(size_t size_failed) {
  printf("%s: %zu bytes needed", SIZE_ERR, size_failed);
  return -1;
}

void free_dystring(dystring_t *s) {
  free(s->data);
  free(s);
  return;
}