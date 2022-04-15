int n1 = 12, n2 = 18;

int main()
{
    int max;
    max = (n1 > n2) ? n1 : n2;
    do
    {
        if (max % n1 == 0 && max % n2 == 0)
            break;
        else
            ++max;
    } while (1);
    return 0;
}
