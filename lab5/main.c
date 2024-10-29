#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <linux/limits.h>

const char* manual = "usage: %s <archive name> [-i(--input) <file> | -e(--extract) <file> | -s(--stat) | -h(--help)]\n"
                     "uncompressed archiver\n\n"
                     "Add file to archive:\n"
                     "\t%s <archive> --input <file>\n\n"
                     "Extract file from archive:\n"
                     "\t%s <archive> --extract <file>\n\n"
                     "Stat archive:\n"
                     "\t%s <archive> --stat\n\n"
                     "Show this help:\n"
                     "\t%s <archive> --help\n";
int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr,
                "usage: %s <archive name> [-i(--input) <file> | -e(--extract) <file> | -s(--stat) | -h(--help)]\n",
                argv[0]);
        return 1;
    }

    char* archive_file = argv[1];

    int mode = 0, help = 0;

    const char* short_options = "i:e:sh";
    struct option long_options[] = {{"insert", required_argument, NULL, 'i'},
                                    {"extract", required_argument, NULL, 'e'},
                                    {"help", no_argument, NULL, 'h'},
                                    {"stat", no_argument, NULL, 's'},
                                    {NULL, 0, NULL, 0}};

    int res = 0;
    char* file = NULL;
    while ((res = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
        switch (res) {
        case 'i':
            mode = 1;
            file = optarg;
            break;
        case 'e':
            mode = -1;
            file = optarg;
            break;
        case 's':
            mode = 0;
            break;
        case 'h':
            help = 1;
            break;
        case '?':
            fprintf(stderr, "%s: unknown argument\n", argv[0]);
            return 1;
        }
    }

    if (help) {
        printf(manual, argv[0], argv[0], argv[0], argv[0], argv[0]);
        return 0;
    }

    int fd = open(archive_file, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("archiver");
        return 1;
    }

    char buf[1024];
    struct stat st;
    short name_len = 0;
    char name_buf[PATH_MAX];

    if (mode == 0) { // stat
        int cnt = 0;
        printf("List of archive %s files:\n", archive_file);
        while (1) {
            int n = read(fd, &name_len, sizeof(name_len));
            if (n == 0) {
                break;
            }

            if (n < 0) {
                perror("archiver: read name_len");
                return 1;
            }

            n = read(fd, name_buf, name_len);
            if (n != name_len) {
                perror("archiver: read name");
                return 1;
            }

            n = read(fd, &st, sizeof(st));
            if (n != sizeof(st)) {
                perror("archiver: read stat");
                return 1;
            }

            n = lseek(fd, st.st_size, SEEK_CUR);
            if (n < 0) {
                perror("archiver: lseek");
                return 1;
            }

            printf("\t%s (%ld bytes)\n", name_buf, st.st_size);
            cnt += 1;
        }
        printf("Total %d files\n", cnt);
    } else if (mode == 1) { // input
        name_len = strlen(file) + 1;
        int n = lseek(fd, 0, SEEK_END);
        if (n < 0) {
            perror("archiver: lseek");
            return 1;
        }

        n = write(fd, &name_len, sizeof(name_len));
        if (n < 0) {
            perror("archiver: write name_len");
            return 1;
        }

        n = write(fd, file, name_len);
        if (n != name_len) {
            perror("archiver: write name");
            return 1;
        }

        n = stat(file, &st);
        if (n < 0) {
            perror("archiver: stat file");
            return 1;
        }

        n = write(fd, &st, sizeof(st));
        if (n != sizeof(st)) {
            perror("archiver: write stat");
            return 1;
        }

        int ffd = open(file, O_RDONLY);
        while (1) {
            n = read(ffd, buf, sizeof(buf));
            if (n == 0) {
                break;
            }
            if (n < 0) {
                perror("archiver: read content");
                return 1;
            }

            int new_n = write(fd, buf, n);
            if (n != new_n) {
                perror("archiver: write content");
                return 1;
            }
        }

        close(ffd);
    } else if (mode == -1) { // extract
        int found = 0;
        int tfd = open("/tmp", O_TMPFILE | O_RDWR | O_TRUNC, 0644);
        if (tfd < 0) {
            perror("archiver: open temp file");
            return 1;
        }

        while (1) {
            int n = read(fd, &name_len, sizeof(name_len));
            if (n == 0) {
                break;
            }

            if (n < 0) {
                perror("archiver: read name_len");
                return 1;
            }

            n = read(fd, name_buf, name_len);
            if (n != name_len) {
                perror("archiver: read name");
                return 1;
            }

            n = read(fd, &st, sizeof(st));
            if (n != sizeof(st)) {
                perror("archiver: read stat");
                return 1;
            }

            if (strcmp(file, name_buf)) {
                n = write(tfd, &name_len, sizeof(name_len));
                if (n < 0) {
                    perror("archiver: write name_len");
                    return 1;
                }
                n = write(tfd, name_buf, name_len);
                if (n != name_len) {
                    perror("archiver: write name");
                    return 1;
                }
                n = write(tfd, &st, sizeof(st));
                if (n != sizeof(st)) {
                    perror("archiver: write stat");
                    return 1;
                }
                ssize_t k = st.st_size;
                while (k != 0) {
                    int n = read(fd, buf, (sizeof(buf) < k ? sizeof(buf) : k));
                    if (n < 0) {
                        perror("archiver: read content");
                        return 1;
                    }
                    int new_n = write(tfd, &buf, n);
                    if (n != new_n) {
                        perror("archiver: write content");
                        return 1;
                    }

                    k -= n;
                }

                continue;
            }

            found = 1;
            break;
        }

        if (!found) {
            fprintf(stderr, "%s not found in archive\n", file);
            return 1;
        }

        int ffd = open(file, O_WRONLY | O_CREAT | O_EXCL, 600);
        ssize_t k = st.st_size;
        while (k != 0) {
            int n = read(fd, buf, (sizeof(buf) < k ? sizeof(buf) : k));
            if (n < 0) {
                perror("archiver: read content");
                return 1;
            }

            int new_n = write(ffd, buf, n);
            if (n != new_n) {
                perror("archiver: write content");
                return 1;
            }

            k -= n;
        }
        close(ffd);
        int err = chmod(file, st.st_mode & 0xfff);
        if (err < 0) {
            perror("archiver: chmod");
            return 1;
        }
        struct timeval ts[2];
        TIMESPEC_TO_TIMEVAL(&ts[0], &st.st_atim);
        TIMESPEC_TO_TIMEVAL(&ts[1], &st.st_mtim);
        err = utimes(file, ts);
        if (err < 0) {
            perror("archiver: chmod");
            return 1;
        }

        while (1) {
            int n = read(fd, buf, sizeof(buf));
            if (n == 0) {
                break;
            }
            if (n < 0) {
                perror("archiver: read content");
                return 1;
            }

            int new_n = write(tfd, buf, n);
            if (n != new_n) {
                perror("archiver: write content");
                return 1;
            }
        }

        close(fd);
        fd = open(archive_file, O_RDWR | O_TRUNC);

        int n = lseek(tfd, 0, SEEK_SET);
        if (n < 0) {
            perror("archiver: lseek");
            return 1;
        }

        while (1) {
            n = read(tfd, buf, sizeof(buf));
            if (n == 0) {
                break;
            }
            if (n < 0) {
                perror("archiver: read content");
                return 1;
            }

            int new_n = write(fd, buf, n);
            if (n != new_n) {
                perror("archiver: write content");
                return 1;
            }
        }

        close(tfd);
    }

    close(fd);
    return 0;
}
