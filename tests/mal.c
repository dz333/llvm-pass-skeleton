#include <stdlib.h>

int* other(int size) {
  int *z = malloc(size * sizeof(int));
  for (int i = 0; i < size; i++) {
    z[i] = 4;
  }
  return z;
}

int main() {
  int y = 10;
  return other(y)[11];
}
