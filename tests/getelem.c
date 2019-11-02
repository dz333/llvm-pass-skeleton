#include <stdlib.h>

typedef struct example {
  int f1;
  int *f2;
  char c1;
  char c2[2];
} example;

typedef struct lnode {
  int val;
  struct lnode *next;
} lnode;

int doStuff(int n) {
  int temp[10];
  temp[n] = 3;
  return temp[n];
}

void doList(lnode l, int v) {
  l.next->val = v;
}

char doStruct(example n) {
  return n.c2[1] + n.c1;
}

int main() {
  example n;
  n.f1 = 3;
  n.f2 = &(n.f1);
  n.c1 = 'a';
  n.c2[0] = 'b';
  n.c2[1] = 'c';
  char x = doStruct(n);
  x = 11;
  int y = doStuff(x);
  exit(0);
}
