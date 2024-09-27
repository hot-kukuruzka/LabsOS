#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <sys/types.h>

regex_t pattern_regex;

int is_opt(char* name) {
    return name[0] == '-';
}

FILE *open_file(char *name) {
    FILE *f = fopen(name, "r");

    if (!f) {
        fprintf(stderr, "file %s not found", name);
        exit(1);
    }

    return f;
}

void red_color() {
    printf("\x1b[31m");
}

void reset_color() {
    printf("\x1b[37m");
}

void process_line(char *line, size_t line_size) {
    regmatch_t match = {
        .rm_so = 0, // regmatch start offset
        .rm_eo = 0 // regmatch end offset
    };
    int found = 0;

    size_t ind = 0;
    while (ind < line_size) {
        if (match.rm_eo == ind) {
            reset_color();

            if (regexec(&pattern_regex, (line + ind), 1, &match, 0) == REG_NOMATCH) {
                break;
            }

            found += 1;
            match.rm_so += ind;
            match.rm_eo += ind;
        }

        if (match.rm_so == ind) {
            red_color();
        }

        fputc(line[ind], stdout);
        ind++;
    }

    if (ind != line_size && found > 0) {
        printf("%s", (line + ind));
    }
}

void process_file(FILE *f) {
    while (1) {
        char *line = NULL;
        size_t line_size = 0;

        if (getline(&line, &line_size, f) == -1) {
            free(line);
            break;
        }

        process_line(line, line_size);
        free(line);
    }
}


int main(int argc, char **argv) {
    int regex_found = 0;

    const int BUF_SIZE = 1024;
    char buf[BUF_SIZE];

    int any_file_processed = 0;

    for (size_t i = 1; i < argc; i++) {
        if (is_opt(argv[i])) {
            continue;
        }

        if (!regex_found) {
            int err = 0;
            if ((err = regcomp(&pattern_regex, argv[i], 0)) != 0) {
                regerror(err, &pattern_regex, buf, BUF_SIZE);
                fprintf(stderr, "pattern error: %s\n", buf);
                regfree(&pattern_regex);
                exit(1);
            }

            regex_found = 1;
            continue;
        }

        any_file_processed = 1;

        FILE *file = open_file(argv[i]);

        process_file(file);

        fclose(file);
    }

    if (!regex_found) {
        fprintf(stdout, "usage: %s <pattern> [file ...]\n", argv[0]);
        exit(1);
    }

    if (!any_file_processed) {
        process_file(stdin);
    }

    regfree(&pattern_regex);

    return 0;
}
