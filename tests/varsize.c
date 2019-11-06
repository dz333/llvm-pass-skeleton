int doStuff(int x[1100], int idx) {
  return x[idx];
}

int doThings(int x, int y) {
  int tmp[x];
  tmp[y] = 3;
  return tmp[y];
}

int main() {
  int tmp[11];
  for (int i = 0; i < 11; i++) {
    tmp[i] = i;
  }
  return doStuff(tmp, 42);
}
