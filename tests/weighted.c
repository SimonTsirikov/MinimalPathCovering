void help(int a) {
    if (a == 1) {
        return;
    } else {
        while (a == 3) {};
    }
}

int main(int argc, char** argv) {
    if (argc == 1) {
        LABEL:
        for (int i=0; i<100; i++) {}
    } else {
        if (strcmp(argv[1], "exploit")) {
            goto LABEL;
        } else {
            int buf = argc;
            while (buf > 0) {
                buf--;
                help(buf);
            }
        }
    }

    return 0;
}
