#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

int numbers = 0, numbers_skip_empty = 0, show_end = 0;
int line_counter = 1;

int is_opt(char* name) {
    return name[0] == '-';
}

int is_eoln(char x) { // End of line
    return x == '\n';
}

FILE *open_file(char *name) {
    FILE *f = fopen(name, "r");

    if (!f) {
        fprintf(stderr, "file %s not found", name);
        exit(1);
    }

    return f;
}

void print_file(FILE *f) {
    int is_prev_eoln = 1;

    while (1) {
        int chr = fgetc(f);

        if (chr == EOF) {
            break;
        }

        if (chr == '\r') { // fix for windows
            continue;
        }

        if (numbers && is_prev_eoln && (!numbers_skip_empty || !is_eoln(chr))) {
            printf("% 6d ", line_counter);
            is_prev_eoln = 0;
        }

        if (is_eoln(chr)) {
            if (!numbers_skip_empty || !is_prev_eoln) {
                line_counter++;
            }

            is_prev_eoln = 1;

            if (show_end) {
                printf("$");
            }
        }


        fputc(chr, stdout);
    }
}

void parse_flags(int argc, char **argv) {
    int c;
    while ((c = getopt(argc, argv, "nbE")) != -1) {
        switch (c) {
            case 'n': // нумеровать все строки
                numbers = 1;
                break;
            case 'b': // нумеровать только непустые строки (отменяет флаг n)
                numbers = 1;
                numbers_skip_empty = 1;
                break;
            case 'E': // показывать символ $ в конце каждой строки
                show_end = 1;
                break;
            case '?': // ХЗ
                printf("Error %d", optopt);
                break;
        }
    }
}

int main(int argc, char **argv) {
    parse_flags(argc, argv);

    char* file_path = NULL;

    for (size_t i = 1; i < argc; i++) {
        if (is_opt(argv[i])) {
            continue;
        }

        FILE *file = open_file(argv[i]);

        print_file(file);

        fclose(file);
    }

    return 0;
}
