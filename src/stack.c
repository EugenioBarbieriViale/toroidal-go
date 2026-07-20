#include "stack.h"
#include <stdio.h>
#include <stdlib.h>

void construct(Stack *s) {
  s->top = -1;
  s->length = INIT_STACK_SIZE;
  s->buffer = (int *)malloc(INIT_STACK_SIZE * sizeof(int));
  if (!s->buffer) {
    perror("Error allocating memory");
    abort();
  }
}

void reallocate(int size, Stack *s) {
  s->length += size;
  int *tmp_ptr = realloc(s->buffer, s->length * sizeof(int));
  if (tmp_ptr) {
    s->buffer = tmp_ptr;
  } else {
    perror("Error reallocating memory");
    abort();
  }
}

void push(int val, Stack *s) {
  if (s->top >= s->length - 1) {
    reallocate(INIT_STACK_SIZE, s);
    // printf("Adding memory to stack, lenght now is %d\n", s->length);
  }

  s->top++;
  s->buffer[s->top] = val;
}

int pop(Stack *s) {
  if (s->top <= -1) {
    perror("Stack underflow, something is wrong\n");
    abort();
  }

  int len_diff = s->length - 2 * INIT_STACK_SIZE;
  if (len_diff > 0 && s->top <= len_diff) {
    reallocate(-1 * INIT_STACK_SIZE, s);
    // printf("Removing unused memory from stack, lenght now is %d\n",
    // s->length);
  }

  int n = s->buffer[s->top--];
  return n;
}
