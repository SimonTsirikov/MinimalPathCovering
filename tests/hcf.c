int n1 = 42, n2 = 100;

int main() {
  int hcf;

  if (n2 > n1) {   
    int temp = n2;
    n2 = n1;
    n1 = temp;
  }
    
  for (int i = 1; i <=  n2; ++i) {
    if (n1 % i == 0 && n2 % i ==0) {
      hcf = i;
    }
  }
  return 0;
}
