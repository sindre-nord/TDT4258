#include <stdio.h>
#include <dirent.h>
#include <fnmatch.h>

/* 
After some back and forth this seems to be the most elegant way to look into a directory
and look for a specific file 
*/

int main(int argc, char *argv[]) {
    DIR *dir;
    struct dirent *entry;
    const char *directory_path = "/dev"; // default to /dev directory
    const char *pattern = "tty.*"; // The pattern to match
    if (argc > 1) {
        directory_path = argv[1];
    }

    dir = opendir(directory_path);
    if (!dir) {
        perror("Unable to open directory");
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (fnmatch(pattern, entry->d_name, 0) == 0) {
            printf("%s\n", entry->d_name);
        }
    }

    closedir(dir);
    return 0;
}

