int a = 0;

void help() {
    if (a == 1) {
        return;
    } else {
        while (a == 3) {};
    }
}

int main() {
    if (a == 0) {
        a = 6;
    } else {
        help();
    }
    if (a == 3) {
        a = 5;
        return 0;
    }
    while (a == 3) {};
    return a;
}