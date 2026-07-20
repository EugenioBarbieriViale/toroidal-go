#ifndef STACK_H
#define STACK_H

#define INIT_STACK_SIZE 8

typedef struct {
  int top;
  unsigned int length;
  int *buffer;
} Stack;

void construct(Stack *);
void reallocate(int, Stack *);
void push(int, Stack *);
int pop(Stack *);

#endif
