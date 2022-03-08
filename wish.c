#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>

void commander(char *arg, char **parsed);
int parser(char *input, char** output);
int runBuiltin(char **input);
int runPathSetup(char **parsed);
char usable_path[150];
char *path = "/bin";

int main(int argc, char *argv[]) {
    int currentLine = 1;
    char *parsed[150] = { NULL };

    if (argc == 1) {
        //INTERACTIVE MODE
        while(1) {
            char str[100];
            printf("wish>");
            fgets(str, 100, stdin);

            commander(str, parsed);
        }
    } else {
        //BATCH MODE
        char str[100];
        FILE *fp;

        fp = fopen(argv[1], "r");

        while (fgets(str, 100, fp) != NULL) {
            //printf("Line%d: %s", currentLine, str);
            commander(str, parsed);
            currentLine++;
        }
        
        fclose(fp);
    }
}

void commander (char *arg, char **parsed) {
    int length = parser(arg, parsed);

    int i = 0;

    if ( strncmp(parsed[0], "cd", 2) == 0 ) {
        if (length != 2) {
            fprintf(stderr, "An error has occurred\n");
        } else {
            chdir(parsed[1]);
        }
    } else if ( strncmp(parsed[0], "exit", 4) == 0 ) {
        if ( length != 1 ) {
            fprintf(stderr, "An error has occurred\n");
        }
        exit(0);
    } else if ( strncmp(parsed[0], "path", 4 ) == 0 ) {
        if ( length == 1 ) {
            path = "";
        } else {
            path = parsed[1];
            strcat(path, " ");
            int i = 2;
            while (parsed[i] != NULL) {
                strcat(path, parsed[i]);
                strcat(path, " ");
            }
        }
        runPathSetup(parsed);
    } else if ( strncmp(parsed[0], "pwd", 3) == 0 ) {
        char directory[400];
        getcwd(directory, sizeof(directory));
        printf("%s\n", directory);
    } else if ( strncmp(parsed[0], "ls", 2) == 0 ) {
        int checker = runPathSetup(parsed);
        if (checker) {
            runBuiltin(parsed);
        }
    } else {
        int checker = runPathSetup(parsed);
        if (checker) {
            runBuiltin(parsed);
        } else {
            fprintf(stderr, "An error has occurred\n");
        }
    }
}

//returns length of output array
int parser(char *input, char **output) {
    char *found;

    int i = 0;
    while ( (found = strsep(&input, " \n")) != NULL ) {
        if (strlen(found) > 0) {
            output[i] = found;
            i++;
        }
    }
    output[i] = NULL;
    return i;
}

int runBuiltin(char **input) {
    int rc = fork();

    if (rc < 0) {
        //fail to fork
        return 0;
    } else if (rc == 0) {
        //in child
        execv(usable_path, input);
        exit(-1);
    } else {
        //in parent
        //rc = child's id
        return 0;
    }
    return 1;
}

int runPathSetup(char **parsed) {
    /**
    For example, when the user types `ls`,
    and path is set to include both `/bin` and `/usr/bin`, try `access("/bin/ls",
    X_OK)`. If that fails, try "/usr/bin/ls". If that fails too, it is an error.
    */
    char test_path[150];
    strcpy(test_path, path);
    strcat(test_path, "/");
    strcat(test_path, parsed[0]);

    if ( access(test_path, X_OK) == 0 ) {
        strcpy(usable_path, test_path);
        return 1;
    } else {
        strcpy(usable_path, "\0");
        return 0;
    }
}