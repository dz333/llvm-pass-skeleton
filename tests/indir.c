
int doStuff(int* x[], int y, int z) {
  return x[y][z];
}
int main() {
  int* tmp[9];
  int arr[7];
  tmp[3] = arr;
  arr[1] = 1337;
  return doStuff(tmp, 3, 1);
}
