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

char* const VALID_TOKENS = "<>+-.,[]";

bfk_instuctions* parse(char *src) {
  dystring_t *code = dystring_read_file(src);
  if (code == NULL) {
    print_err("parse", "failed to read source file into string");
    return NULL;
  };

  bfk_tokenizer *tokenizer = get_tokenizer(code);
  if (tokenizer == NULL) {
    free_dystring(code);
    return NULL;
  };

  bfk_instuctions *prog = get_instructions(tokenizer);
  if (prog == NULL) {
    free_dystring(code);
    free_tokenizer(tokenizer);
    print_err("parse", "failed to get program instructions from token stream");
    return NULL;
  }

  free_dystring(code);
  free_tokenizer(tokenizer);
  return prog;
}

// gets the instruction set for the program to execute.
bfk_instuctions* get_instructions(bfk_tokenizer *tokenizer) {
  bfk_instuctions *prog = malloc(sizeof(bfk_instuctions));
  if (prog == NULL) {
    print_err("get_instructions", "unable to malloc prog");
    return NULL;
  }

  bfk_token_t *tokens = malloc(sizeof(bfk_token_t) * tokenizer->size);
  if (tokens == NULL) {
    print_err("get_instructions", "unable to malloc token array");
    return NULL;
  }

  prog->instructions = tokens;
  prog->size = 0;
  prog->capacity = tokenizer->size;

  // this should probably be extracted, but local imp is fine for now.
  size_t stack_pointer = 0;
  size_t stack_size = prog->capacity;
  size_t *jump_stack = malloc(sizeof(int) * stack_size);
  if (jump_stack == NULL) {
    print_err("get_instructions", "unable to allocate a stack for jump processing");
    return NULL;
  }

  enum BFK_TYPE type;
  while ((type = get_next_token(tokenizer)) != BFK_EOF) {
    switch (type) {
      case MOVR:
      case MOVL:
      case INC:
      case DEC:
      case WRITE:
      case READ: {
        size_t count = continue_while_type(tokenizer, type);
        push_token(type, count, prog);
        break; 
      }
      
      case JEQ:
        if (stack_pointer >= stack_size) {
          print_err("get_instructions", "unexpected number of jump instructions");
          return NULL;
        }
        jump_stack[stack_pointer] = prog->size;
        stack_pointer += 1;
        push_token(type, 0, prog); 
        break;

      case JNE:
        if (stack_pointer <= 0) {
          print_err("get_instructions", "unexpected number of jump instructions");
          return NULL;
        }
        stack_pointer -= 1;
        size_t address = jump_stack[stack_pointer];
        prog->instructions[address].value = prog->size;
        push_token(type, address, prog);
        break;

      default:
        print_err("get_instructions", "unexpected EOF reached");
        break;
    }
  }

  push_token(type, 0, prog);
  free(jump_stack);
  return prog;
}
/*
continues advancing the tokenizer until it reaches a new type and accumulates the number of 
times it encountered the same type. Used to fold together types that are repeated and can be a single calculation.
(i.e., moving the address pointer or incrementing the value at a given address)
*/
size_t continue_while_type(bfk_tokenizer *tokenizer, enum BFK_TYPE type) {
  size_t count = 1;
  while (type == peek_next_token(tokenizer)) {
    count += 1;
    get_next_token(tokenizer);
  }
  return count;
}

// writes a token at the current position in the program. The program struct is somewhat static,
// based on the number of tokens. There is no need to handle resizing because we know exactly 
// how many instructions there are going to be at runtime. That said the amount in the instruction 
// set CAN be less than the available space for the instructions array. 
void push_token(enum BFK_TYPE type, size_t count, bfk_instuctions *prog) {
  if (prog->size >= prog->capacity) {
    // this should be unreachable. 
    print_err("push_token", "attempt to push beyond instruction set size.");
    return;
  }
  prog->instructions[prog->size].type = type;
  prog->instructions[prog->size].value = count;
  prog->size += 1;
}

// gets a view over an underlying char array from a dystring. Must be careful to free the dystring
// only after tokenizer is done.
bfk_tokenizer* get_tokenizer(dystring_t *src) {
  dystring_t *tokens = get_dystring();
  if (tokens == NULL) {
    print_err("get_tokenizer", "failed to get dystring");
    return NULL;
  };

  if (dystring_filter_except(src, tokens, VALID_TOKENS) < 0) {
    print_err("get_tokenizer", "failed to filter source file");
    return NULL; 
  };

  bfk_tokenizer *tokenizer = malloc(sizeof(bfk_tokenizer));
  if (tokenizer == NULL) {
    print_err("get_tokenizer", "failed to malloc tokenizer");
    return NULL;
  }

  tokenizer->src = tokens;
  tokenizer->data = tokens->data;
  tokenizer->size = tokens->size;
  tokenizer->index = 0;
  return tokenizer;
}

// tokens are returned via their type, no need work with actually chars at this stage...
enum BFK_TYPE get_next_token(bfk_tokenizer *tokenizer) {
  if (tokenizer->index >= tokenizer->size) {
    return BFK_EOF;
  }

  switch (tokenizer->data[tokenizer->index]) {
    case '>':
      tokenizer->index += 1;
      return MOVR;
    case '<':
      tokenizer->index += 1;
      return MOVL;
    case '+':
      tokenizer->index += 1;
      return INC;
    case '-':
      tokenizer->index += 1;
      return DEC;
    case '.':
      tokenizer->index += 1;
      return WRITE;
    case ',':
      tokenizer->index += 1;
      return READ;
    case '[':
      tokenizer->index += 1;
      return JEQ;
    case ']':
      tokenizer->index += 1;
      return JNE;
    default:
      print_err("get_next_token", "unexpected end of file");
      return BFK_EOF;
  }
}

// reads the next token without advancing the index pointer.
enum BFK_TYPE peek_next_token(bfk_tokenizer *tokenizer) {
  enum BFK_TYPE next = get_next_token(tokenizer);
  if (next == BFK_EOF) {
    return next;
  }
  tokenizer->index--;
  return next;
}

void free_tokenizer(bfk_tokenizer *tokenizer) {
  free_dystring(tokenizer->src);
  free(tokenizer);
}

void free_instructions(bfk_instuctions *prog) {
  free(prog->instructions);
  free(prog);
}

void print_err(char* src, char* reason) {
  printf("[ERROR]: \"%s\" %s\n", src, reason);
}

void print_token(bfk_token_t *toke) {
  printf("{ type: %d, value: %d }\n", toke->type, toke->value);
}