#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <getopt.h>
#include <pwd.h>
#include <grp.h>

struct dirfile {
    char* name;
    char* link;
    struct stat st;
};

char* get_current_working_dir() {
    char dir_path[PATH_MAX];
    memset(dir_path, 0, PATH_MAX);
    getcwd(dir_path, sizeof(dir_path) / sizeof(*dir_path));
    return strdup(dir_path);
}

int is_file_hidden(char* name) {
    return name[0] == '.';
}

int is_opt(char* name) {
    return name[0] == '-';
}

size_t get_directory_files(char* dir_path, struct dirfile** dir_files) {
    assert(*dir_files == NULL);

    size_t size = 0;
    size_t capacity = 1;
    struct dirfile* files = realloc(NULL, sizeof(*files) * capacity);

    char buf[PATH_MAX];
    char link_buf[PATH_MAX];

    DIR* dirfd = opendir(dir_path);
    if (dirfd == NULL) {
        return 0;
    }

    while (1) {
        struct dirent* de = readdir(dirfd);
        if (de == NULL) {
            break;
        }

        if (size == capacity) {
            capacity *= 2;
            files = realloc(files, sizeof(*files) * (capacity + 1));
        }

        snprintf(buf, PATH_MAX, "%s/%s", dir_path, de->d_name);
        files[size].name = strdup(de->d_name);
        lstat(buf, &files[size].st);
        if (de->d_type == DT_LNK) {
            size_t n = readlink(buf, link_buf, sizeof(link_buf) / sizeof(*link_buf));
            link_buf[n] = 0;
            files[size].link = strdup(link_buf);
        } else {
            files[size].link = NULL;
        }
        size++;
    }


    for (size_t i = size - 1; i > 0; i--) {
        size_t max_pos = i;
        for (size_t j = 0; j < i; j++) {
            char* s1 = files[j].name;
            char* s2 = files[max_pos].name;

            if (is_file_hidden(s1)) {
                s1++;
            }
            if (is_file_hidden(s2)) {
                s2++;
            }

            if (strcasecmp(s1, s2) > 0) {
                max_pos = j;
            }
        }

        if (max_pos != i) {
            struct dirfile de = files[max_pos];
            files[max_pos] = files[i];
            files[i] = de;
        }
    }

    *dir_files = files;
    return size;
}


size_t filter_hidden_files(struct dirfile* dir_files, size_t size) {
    size_t w_ind = 0;
    for (size_t i = 0; i < size; i++) {
        if (is_file_hidden(dir_files[i].name)) {
            continue;
        }

        if (w_ind != i) {
            dir_files[w_ind] = dir_files[i];
        }

        w_ind++;
    }

    return w_ind;
}

char* format_file_mode(mode_t mode) {
    const int MODE_STR_LEN = 11; // drwxrwxrwx
    char* mode_str = malloc(sizeof(*mode_str) * MODE_STR_LEN);

    for (size_t i = 0; i < MODE_STR_LEN; i++) {
        mode_str[i] = '-';
    }
    mode_str[MODE_STR_LEN - 1] = 0;

    //if ((mode & S_IFREG) != 0) {
    //    mode_str[0] = '-';
    //} else
    if ((mode & S_IFMT) == S_IFDIR) {
        mode_str[0] = 'd';
    } else if ((mode & S_IFMT) == S_IFLNK) {
        mode_str[0] = 'l';
    }

    if ((mode & S_IRUSR) != 0) {
        mode_str[1] = 'r';
    }
    if ((mode & S_IWUSR) != 0) {
        mode_str[2] = 'w';
    }
    if ((mode & S_IXUSR) != 0) {
        mode_str[3] = 'x';
    }

    if ((mode & S_IRGRP) != 0) {
        mode_str[4] = 'r';
    }
    if ((mode & S_IWGRP) != 0) {
        mode_str[5] = 'w';
    }
    if ((mode & S_IXGRP) != 0) {
        mode_str[6] = 'x';
    }

    if ((mode & S_IROTH) != 0) {
        mode_str[7] = 'r';
    }
    if ((mode & S_IWOTH) != 0) {
        mode_str[8] = 'w';
    }
    if ((mode & S_IXOTH) != 0) {
        mode_str[9] = 'x';
    }

    return mode_str;
}

char* get_user_name(unsigned uid) {
    struct passwd* psw;
    psw = getpwuid(uid);
    if (!psw) {
        char* res = NULL;
        asprintf(&res, "%u", uid);
        return res;
    }
    return strdup(psw->pw_name);
}

char* get_group_name(unsigned gid) {
    struct group* grp;
    grp = getgrgid(gid);
    if (!grp) {
        char* res = NULL;
        asprintf(&res, "%u", gid);
        return res;
    }
    return strdup(grp->gr_name);
}

char* format_date(time_t mtime) {
    char date_buf[100];
    strftime(date_buf, sizeof(date_buf) / sizeof(*date_buf), "%b %d %H:%M", localtime(&mtime));
    return strdup(date_buf);
}

char* format_name(char* name, char* links_to, mode_t mode) {
    char* start = "\x1B[0m";

    if ((mode & S_IFMT) == S_IFDIR) {
        start = "\x1B[94m";
    } else if ((mode & S_IFMT) == S_IFLNK) {
        start = "\x1B[96m";
    } else if (((mode & S_IXUSR) != 0) || ((mode & S_IXGRP) != 0) || ((mode & S_IXOTH) != 0)) {
        start = "\x1B[92m";
    }

    char buf[1024];

    if ((mode & S_IFMT) == S_IFLNK && links_to) {
        snprintf(buf, sizeof(buf) / sizeof(*buf), "%s%s\x1B[0m -> %s", start, name, links_to);
    } else {
        snprintf(buf, sizeof(buf) / sizeof(*buf), "%s%s\x1B[0m", start, name);
    }

    return strdup(buf);
}

char* format_ulong(unsigned long x) {
    char buf[100];
    snprintf(buf, sizeof(buf) / sizeof(*buf), "%lu", x);
    return strdup(buf);
}

int main(int argc, char* argv[]) {
    int all_files = 0;
    int list_view = 0;

    int c;
    while ((c = getopt(argc, argv, "la")) != -1) {
        switch (c) {
            case 'a':
                all_files = 1;
                break;
            case 'l':
                list_view = 1;
                break;
            case '?':
                printf("Error %d", optopt);
                break;
        }
    }

    char* dir_path = NULL;

    for (size_t i = 1; i < argc; i++) {
        if (is_opt(argv[i])) {
            continue;
        }
        dir_path = argv[i];
        break;
    }

    int cleanup_dir_path = 0;
    if (!dir_path) {
        dir_path = get_current_working_dir();
        cleanup_dir_path = 1;
    }

    struct dirfile* dir_files = NULL;
    size_t dir_files_cnt = get_directory_files(dir_path, &dir_files);

    if (cleanup_dir_path) {
        free(dir_path);
    }

    if (!all_files) {
        dir_files_cnt = filter_hidden_files(dir_files, dir_files_cnt);
    }

    if (!list_view) {
        for (size_t i = 0; i < dir_files_cnt; i++) {
            char* name = format_name(dir_files[i].name, NULL, dir_files[i].st.st_mode);
            printf("%s\t", name);
            free(name);
        }

        printf("\n");
        free(dir_files);
        return 0;
    }

    char** users = calloc(dir_files_cnt, sizeof(*users));
    char** groups = calloc(dir_files_cnt, sizeof(*groups));

    size_t total_size = 0;
    size_t max_user_size = 0, max_group_size = 0, max_size_size = 0, max_nlinks_size = 0;
    for (size_t i = 0; i < dir_files_cnt; i++) {
        total_size += (dir_files[i].st.st_blocks / 2); // Block - 512b, output - 1024b
        char* user_str = get_user_name(dir_files[i].st.st_uid);
        char* group_str = get_group_name(dir_files[i].st.st_gid);

        char* size_str = format_ulong(dir_files[i].st.st_size);
        char* nlinks_str = format_ulong(dir_files[i].st.st_nlink);

        max_user_size = (max_user_size < strlen(user_str) ? strlen(user_str) : max_user_size);
        max_group_size = (max_group_size < strlen(group_str) ? strlen(group_str) : max_group_size);
        max_size_size = (max_size_size < strlen(size_str) ? strlen(size_str) : max_size_size);
        max_nlinks_size = (max_nlinks_size < strlen(nlinks_str) ? strlen(nlinks_str) : max_nlinks_size);

        users[i] = user_str;
        groups[i] = group_str;

        free(nlinks_str);
        free(size_str);
    }

    char format_buf[100];
    snprintf(format_buf, 
             sizeof(format_buf) / sizeof(*format_buf),
             "%%s %%%luld %%%lus %%%lus %%%lud %%s %%s\n", 
             max_nlinks_size,
             max_user_size,
             max_group_size, 
             max_size_size
             );

    printf("total %lu\n", total_size);
    for (size_t i = 0; i < dir_files_cnt; i++) {
        char* mode_str = format_file_mode(dir_files[i].st.st_mode);
        char* user_str = users[i];
        char* group_str = groups[i];
        char* date_str = format_date(dir_files[i].st.st_mtime);
        char* name = format_name(dir_files[i].name, dir_files[i].link, dir_files[i].st.st_mode);

        printf(format_buf, 
               mode_str,
               dir_files[i].st.st_nlink,
               user_str,
               group_str,
               dir_files[i].st.st_size,
               date_str,
               name
               );

        free(name);
        free(date_str);
        free(user_str);
        free(group_str);
        free(mode_str);
    }

    free(groups);
    free(users);
    free(dir_files);
    return 0;
}
