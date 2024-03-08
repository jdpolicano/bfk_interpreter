#ifndef BFK_PARSER_IMPL
#define BFK_PARSER_IMPL
#include "dystring.h"

enum BFK_TYPE {
  BFK_EOF, // end of input;
  MOVR,
  MOVL,
  INC,
  DEC,
  WRITE,
  READ,
  JEQ,
  JNE,
};

typedef struct {
  enum BFK_TYPE type;
  int value;
} bfk_token_t;

typedef struct {
  // we need this to ultimately free it when we free the tokenizer.
  dystring_t *src;
  char *data;
  size_t size;
  size_t index;
} bfk_tokenizer;

typedef struct {
  bfk_token_t *instructions;
  size_t size;
  size_t capacity;
} bfk_instuctions;


bfk_instuctions* parse(char *src);
bfk_instuctions* get_instructions(bfk_tokenizer *tokenizer);
size_t continue_while_type(bfk_tokenizer *tokenizer, enum BFK_TYPE type);
void push_token(enum BFK_TYPE type, size_t count, bfk_instuctions *prog);
bfk_tokenizer* get_tokenizer(dystring_t *src);
enum BFK_TYPE get_next_token(bfk_tokenizer *tokenizer);
enum BFK_TYPE peek_next_token(bfk_tokenizer *tokenizer);
void free_tokenizer(bfk_tokenizer *tokenizer);
void free_instructions(bfk_instuctions *prog);
void print_err(char* src, char* reason);
void print_token(bfk_token_t *toke);

#endif