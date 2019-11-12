#include <stdlib.h>

void other(int* z, int x) {
  int sum = 0;
  for (int i = 0; i < x; i++) {
    sum += i;
    z[i] = sum;
  }
}

int main(int argc, char *argv[]) {
  int y = 10;
  int *z = malloc(argc * sizeof(int));
  other(z, argc);
  if (argc % 17 == 0) {
    z[argc-1] = 4;
  } else {
    z[argc-1] = 10;
  }
  return z[2] + z[argc-1];
}
