/*************************************************************************
	> File Name: ls-3.c
	> Author: yufangze
	> Mail: 205552151@qq.com
	> Created Time: 2019年05月11日 星期六 14时46分25秒
 ************************************************************************/
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <grp.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <math.h>

#define LEN 512
//0: ls
//1: ls -a
//2: ls -l
//3: ls -al
int ls_opt = 0;
void do_ls(char *);
void do_stat(char *);
void show_file_info(char *, struct stat *);
int mode_to_letters(mode_t, char *);
void size_window(char file_name[][LEN], int cnt, int *row, int *col);
void show_files(char file_name[][LEN], int cnt, int row, int col);
char *uid_to_name(uid_t);
char *gid_to_name(gid_t);
char work_dir[LEN];

int main(int argc, char *argv[]) {
    int opt;
    getcwd(work_dir, LEN);
    while ((opt = getopt(argc, argv, "al")) != -1) {
        switch (opt) {
            case 'a':
                //printf("a\n");
                ls_opt++;
                break;
            case 'l':
                //printf("l\n");
                ls_opt += 2;
                break;
            default:
                exit(1);
        }
    }
    argc -= (optind - 1);
    argv += (optind - 1);
    //printf("argv[%d] = %s\n", argc - 1, *(argv + 1));
    if (argc == 1) {
        do_ls(".");
    } else {
        while (--argc) {
            do_ls(*(++argv));
            //printf("%s\n", *(argv + 1));
        }
    }
    return 0;
}

int cmp_name(const void* _a, const void* _b) {
    char *a = (char *)_a;
    char *b = (char *)_b;
    return strcmp(a, b);
}

void do_ls(char *dirname) {
    printf("%s:\n", dirname);
    chdir(work_dir);
    DIR *dir_ptr = NULL;
    struct dirent *direntp;
    char file_name[1024][LEN] = {0};
    int row, col;
    if ((dir_ptr = opendir(dirname)) == NULL) {
        perror("opendir");
    } else {
        int cnt = 0;
        while ((direntp = readdir(dir_ptr)) != NULL) {
            strcpy(file_name[cnt++], direntp->d_name);
            //printf("%s\n", direntp->d_name);
        }
        qsort(file_name, cnt, LEN, cmp_name);
        /*for (int i = 0; i < cnt; i++) {
            printf("file_name[%d] = %s\n", i, file_name[i]);
        }*/
        if (ls_opt >= 2) {
            chdir(dirname);
            for (int i = 0; i < cnt; i++) {
                do_stat(file_name[i]);
            }
        } else if (ls_opt == 0) {
            size_window(file_name, cnt, &row, &col);
            show_files(file_name, cnt, row, col);
        } 
    }
}

void size_window(char file_name[][LEN], int cnt, int *row, int *col) {
    struct winsize size;
    int len[cnt];
    int max = 0;
    int all = 0;
    memset(len, 0, sizeof(int) * cnt);
    if (isatty(STDOUT_FILENO) == 0) {
        exit(1);
    }
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) < 0) {
        perror("ioctl");
        exit(1);
    }
    printf("size_row = %d, size_col = %d\n", size.ws_row, size.ws_col);
    for (int i = 0; i < cnt; i++) {
        len[i] = strlen(file_name[i]);
        if (max < len[i]) max = len[i];
        all += len[i] + 2;
    }
    if (max + 2> size.ws_col) {
        *row = cnt;
        *col = 1;
        return ;
    }
    if (all < size.ws_col) {
        *row = 1;
        *col = cnt;
        return ;
    }
    int try_begin = 0;
    for (int i = 0, tmp = 0; i < cnt; i++) {
        tmp += (len[i] + 2);
        if (tmp > size.ws_col) {
            try_begin = i;
            break;
        }
    }
    for (int i = try_begin; ; i--) {
        int *wide = (int *)malloc(sizeof(int) * i);
        memset(wide, 0, sizeof(int) * i);
        *row = (int)ceil(cnt / i);
        int try_sum = 0;
        for (int x = 0; x < i; x++) {
            for (int y = x * (*row); y < (x + 1) * *row && y < cnt; y++) {
                if (wide[x] < len[y]) wide[x] = len[y];
            }
            try_sum += (wide[x] + 2);
        }
        if (try_sum > size.ws_col) continue;
        if (try_sum <= size.ws_col) {
            *col = i;
            break;
        }
    }
    printf("window size: %d * %d\n", *row, *col);
}

void show_files(char file_name[][LEN], int cnt, int row, int col) {
    int wide_file[cnt];
    memset(wide_file, 0, sizeof(int) * cnt);
    for (int i = 0; i < col; i++) {
        for (int j = (i * row); j < (i + 1) * row && j < cnt; j++) {
            if (wide_file[i] < strlen(file_name[j])) wide_file[i] = strlen(file_name[j]);

        }
    }
    for (int i = 0; i < row; i++) {
        for (int j = i; j < i + (row * col) && j < cnt; j += row) {
            int tmp = j / row;
            printf("%-*s", wide_file[tmp] + 2, file_name[j]);
        }
        printf("\n");
    }
}

void do_stat(char *filename) {
    struct stat info;
    if (stat(filename, &info) == -1) {
        perror(filename);
    } else {
        show_file_info(filename, &info);
    }
}

void show_file_info(char *filename, struct stat *info) {
    char modestr[11] = "----------";
    char *uid_to_name(), *gid_to_name(), *ctime();
    int flag = 0;
    flag = mode_to_letters(info->st_mode, modestr);
    printf("%s ", modestr);
    printf("%4ld ", info->st_nlink);
    printf("%8s ", uid_to_name(info->st_uid));
    printf("%8s ", gid_to_name(info->st_gid));
    printf("%8ld ", (long)info->st_size);
    printf("%.12s ", 4 + ctime(&info->st_mtime));
    if (!flag) {
        printf("%s\n", filename);
    } else if (flag == 1) {
        printf("\e[34;1m%s\e[0m\n", filename);
    } else {
        printf("\e[32;1m%s\e[0m\n", filename);
    }
}

//flag == 1, dir
//flag == 2, 'x'
int mode_to_letters(mode_t mode, char *str) {
    int flag = 0;
    if (S_ISDIR(mode)) flag = 1, str[0] = 'd';
    if (S_ISCHR(mode)) str[0] = 'c';
    if (S_ISBLK(mode)) str[0] = 'b';
    if (S_ISSOCK(mode)) str[0] = 's';
    if (S_ISFIFO(mode)) str[0] = 'p';
    if (S_ISLNK(mode)) str[0] = 'l';
    if (S_ISREG(mode)) str[0] = '-';

    if (mode & S_IRUSR) str[1] = 'r';
    if (mode & S_IWUSR) str[2] = 'w';
    if (mode & S_IXUSR) {
        str[3] = 'x';
        if (flag == 0) flag = 2;
    } 

    if (mode & S_IRGRP) str[4] = 'r';
    if (mode & S_IWGRP) str[5] = 'w';
    if (mode & S_IXGRP) {
        str[6] = 'x';
        if (flag == 0) flag = 2;
    }
    
    if (mode & S_IROTH) str[7] = 'r';
    if (mode & S_IWOTH) str[8] = 'w';
    if (mode & S_IXOTH) {
        str[9] = 'x';
        if (flag == 0) flag = 2;
    }
    return flag;
}

char *uid_to_name(uid_t uid) {
    struct passwd *pw_ptr;
    static char numstr[10] = {0};
    if ((pw_ptr = getpwuid(uid)) == NULL) {
        sprintf(numstr, "%d", uid);
        return numstr;
    } else {
        return pw_ptr->pw_name;
    }
}

char *gid_to_name(gid_t gid) {
    struct group *gr_ptr;
    static char numstr[10] = {0};
    if ((gr_ptr = getgrgid(gid)) == NULL) {
        sprintf(numstr, "%d", gid);
        return numstr;
    } else {
        return gr_ptr->gr_name;
    }
}

