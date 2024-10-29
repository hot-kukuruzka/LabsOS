#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

mode_t parse_perms(const char* s, mode_t old_mode) {
    char* eptr = NULL;
    long x = strtol(s, &eptr, 8);

    if (!(*eptr)) {
        return x;
    }

    mode_t new_mode = old_mode & 0b111111111;

    int all = 1;
    int who[3] = {0};
    int type = 0;
    mode_t new_mode_iter = 0;
    for (ssize_t i = 0; i <= strlen(s); i++) {
        if (s[i] == ',' || i == strlen(s)) {
            if (all) {
                who[2] = 1;
                who[1] = 1;
                who[0] = 1;
            }

            for (int j = 0; j < 3; j++) {
                if (!who[j]) {
                    continue;
                }

                if (type == 1) {
                    new_mode |= (new_mode_iter << (3 * j));
                } else if (type == 0) {
                    new_mode &= ~(0b111 << (3 * j));
                    new_mode |= (new_mode_iter << (3 * j));
                } else if (type == -1) {
                    new_mode &= ~(new_mode_iter << (3 * j));
                }
            }

            who[2] = 0;
            who[1] = 0;
            who[0] = 0;
            all = 1;
            new_mode_iter = 0;
            type = 0;
            continue;
        }

        if (s[i] == 'u') {
            who[2] = 1;
            all = 0;
        } else if (s[i] == 'g') {
            who[1] = 1;
            all = 0;
        } else if (s[i] == 'o') {
            who[0] = 1;
            all = 0;
        } else if (s[i] == 'a') {
            all = 1;
        } else if (s[i] == '-') {
            type = -1;
        } else if (s[i] == '=') {
            type = 0;
        } else if (s[i] == '+') {
            type = 1;
        } else

            if (s[i] == 'r') {
            new_mode_iter |= 0b100;
        } else if (s[i] == 'w') {
            new_mode_iter |= 0b10;
        } else if (s[i] == 'x') {
            new_mode_iter |= 0b1;
        } else {
            return -1;
        }
    }

    return new_mode;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <mode> <file>\n", argv[0]);
        return 1;
    }

    char* path = argv[2];

    struct stat st;
    int err = stat(path, &st);
    if (err) {
        perror("mychmod: stat: ");
        return 1;
    }

    mode_t perms = parse_perms(argv[1], st.st_mode);
    if (perms == -1) {
        fprintf(stderr, "%s: invalid mode\n", argv[0]);
        return 1;
    }

    err = chmod(path, perms);
    if (err) {
        perror("mychmod: chmod: ");
        return 1;
    }

    return 0;
}
