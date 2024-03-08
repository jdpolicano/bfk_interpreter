#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "parser.h"
#include "dystring.h"

char* const USAGE = "./bf [filepath]\n";
int const INT_MAX_SIZE = 0xff;
int const INT_SIZE_CEIL = INT_MAX_SIZE + 1; // 8 bit int max. modulo should overflow correctly with this.
size_t const DEFAULT_MEMORY_SIZE = 64000; // this is 1970.


typedef struct {
  bfk_instuctions *program;
  size_t program_counter;
  size_t address_pointer;
  int *memory;
  bool should_stop;
  int exit_code;
  char *err_msg;
} prog_state;

int run_program(bfk_instuctions *program);
void move_right(prog_state *state, bfk_token_t *toke);
void move_left(prog_state *state, bfk_token_t *toke);
void increment(prog_state *state, bfk_token_t *toke);
void decrement(prog_state *state, bfk_token_t *toke);
void read(prog_state *state, bfk_token_t *toke);
void write(prog_state *state, bfk_token_t *toke);
void jump_if_zero(prog_state *state, bfk_token_t *toke);
void jump_if_not_zero(prog_state *state, bfk_token_t *toke);
int print_err_if_needed(prog_state *state);
void print_state(prog_state *state);

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("usage: %s", USAGE);
    return 1;
  }

  bfk_instuctions *prog = parse(argv[1]);
  if (prog == NULL) {
    return 2;
  }

  return run_program(prog);
}


int run_program(bfk_instuctions *program) {
  prog_state state = {0};
  int memory[DEFAULT_MEMORY_SIZE] = {0};

  state.program_counter = 0;
  state.address_pointer = DEFAULT_MEMORY_SIZE / 2; // start in the middle so it can grow both directions
  state.memory = memory;
  state.should_stop = false;
  state.program = program;
  state.exit_code = 0;
  state.err_msg = NULL;

  while (!state.should_stop) {
    bfk_token_t next_token = state.program->instructions[state.program_counter];
    switch(next_token.type) {
      case MOVR:
        move_right(&state, &next_token);
        break;
      case MOVL:
        move_left(&state, &next_token);
        break;
      case INC:
        increment(&state, &next_token);
        break;  
      case DEC:
        decrement(&state, &next_token);
        break;
      case WRITE:
        write(&state, &next_token);
        break; 
      case READ:
        read(&state, &next_token);
      case JEQ:
        jump_if_zero(&state, &next_token);
        break;
      case JNE:
        jump_if_not_zero(&state, &next_token);
        break;
      default:
        state.should_stop = true;
        break;
    }

    state.program_counter += 1;
  }

  return print_err_if_needed(&state);
}

void move_right(prog_state *state, bfk_token_t *toke) {
  if (state->address_pointer + toke->value >= DEFAULT_MEMORY_SIZE) {
    state->should_stop = true;
    state->exit_code = toke->type;
    state->err_msg = "move right out of bounds";
    return;
  }
  state->address_pointer += toke->value;
};

void move_left(prog_state *state, bfk_token_t *toke) {
  if (state->address_pointer - toke->value < 0) {
    state->should_stop = true;
    state->exit_code = toke->type;
    state->err_msg = "move left out of bounds";
  } else {
    state->address_pointer -= toke->value;
  }
};

void increment(prog_state *state, bfk_token_t *toke) {
  state->memory[state->address_pointer] += toke->value;
  state->memory[state->address_pointer] = 
    state->memory[state->address_pointer] % INT_SIZE_CEIL;
}

void decrement(prog_state *state, bfk_token_t *toke) {
  state->memory[state->address_pointer] -= toke->value;
  state->memory[state->address_pointer] &= INT_MAX_SIZE;
}

void write(prog_state *state, bfk_token_t *toke) {
  for (int i = 0; i < toke->value; i++) {
    if (fwrite(state->memory + state->address_pointer, 1, sizeof(char), stdout) != 1) {
      state->should_stop = true;
      state->exit_code = toke->type;
      state->err_msg = "write to stdout failed...";
      break;
    };
  }
}

void read(prog_state *state, bfk_token_t *toke) {
  printf("not implemented read yet..."); 
}

void jump_if_zero(prog_state *state, bfk_token_t *toke) {
  if (state->memory[state->address_pointer] == 0) {
    state->program_counter = toke->value;
  }
}

void jump_if_not_zero(prog_state *state, bfk_token_t *toke) {
  if (state->memory[state->address_pointer] != 0) {
    state->program_counter = toke->value;
  }
}

int print_err_if_needed(prog_state *state) {
  if (state->exit_code != 0) {
    printf("[ERROR]: %s\n", state->err_msg);
  }
  return state->exit_code;
}

void print_state(prog_state *state) {
  printf("Program Counter: %zu\n", state->program_counter);
  printf("Address Pointer: %zu\n", state->address_pointer);
  printf("Should Stop: %s\n", state->should_stop ? "true" : "false");
  printf("Exit Code: %d\n", state->exit_code);
  printf("Error Message: %s\n", state->err_msg);
}