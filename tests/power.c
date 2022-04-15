int exponent = 10;
float base = 2;

int main() 
{
    float result = 1;
    while (exponent != 0) {
        result *= base;
        --exponent;
    }
    return 0;
}
