static unsigned int y;

void add4(unsigned int n)  {
  if (n < 4) {
    y = y + 4;
  } else {
    y = y - 4;
  }
}

int main() {
  y = 0;
  add4(10);
  return y;
}
