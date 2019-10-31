int doStuff(int n) {
  int temp[10];
  temp[n] = 3;
  return temp[n];
}

int main() {
  return doStuff(11);
}
