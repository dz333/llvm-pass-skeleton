int foo(int x, int y) {
  int tmp[10][8];
  return tmp[x][y];
}

int main() {
  return foo(3, 4);
}
  
