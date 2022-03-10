#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

void commander(char *arg, char **parsed);
int parser(char *input, char** output);
int runBuiltin(char **input, int input_file);
int tryPath(char **parsed);
void editPath(char **parsed);
int find_redirection(char **parsed);
int handle_redirection(char **parsed, int index);

char usable_path[100];
char *path[50] = { NULL };
char *output;

int main(int argc, char *argv[]) {
    int currentLine = 1;
    char *parsed[150] = { NULL };
    path[0] = "/bin"; //initialize first path to bin

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
    int index_p = find_redirection(parsed);
    int input_file = 0;

    if (index_p > 0) {
        //redirection shenanigans
        if (handle_redirection(parsed, index_p) == 1) {
            //we have a redirection and need to use the output file
            output = parsed[index_p + 1];
            input_file = open(output, O_RDWR);
        }
    }
    
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
        editPath(parsed);
    } else if ( strncmp(parsed[0], "pwd", 3) == 0 ) {
        char directory[400];
        getcwd(directory, sizeof(directory));
        printf("%s\n", directory);
    } else if ( strncmp(parsed[0], "ls", 2) == 0 ) {
        if (tryPath(parsed) == 1) {
            runBuiltin(parsed, input_file);
        } else {
            fprintf(stderr, "An error has occurred\n");
        }
    } else {
        if (tryPath(parsed)) {
            runBuiltin(parsed, input_file);
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

int runBuiltin(char **input, int input_file) {
    int rc = fork();

    if (rc < 0) {
        //fail to fork
        return 0;
    } else if (rc == 0) {
        //in child
        if (input_file > 0) {
            dup2(input_file, STDIN_FILENO);//this is running ls > and breaking stuff I think
        } 
        execv(usable_path, input);
        exit(-1);
    } else {
        //in parent
        //rc = child's id
        wait(NULL);
        return 0;
    }
    return 1;
}

int tryPath(char **parsed) {
    char test_path[500];

    //copy path and parsed to each item in test path
    int j = 0;
    while (path[j] != NULL) {
        strcpy(test_path, path[j]);
        strcat(test_path, "/");
        strcat(test_path, parsed[0]);

        if ( access(test_path, X_OK) == 0 ) {
            strcpy(usable_path, test_path);
            return 1;
        }
        j++;
    }
    
    strcpy(usable_path, "\0");
    return 0;
}

void editPath(char **parsed) {
    //change path to whatever we got in parsed
    //erase path
    char *new_path[50] = { NULL };

    int i = 0;
    while (path[i] != NULL) {
        path[i] = new_path[i];
        i++;
    }

    //check if were given a / or not 
    if (strncmp(parsed[0], "/", 1) == 0) {
        //set the path directly
        int j = 1;
        while (parsed[j] != NULL) {
            path[j - 1] = "/";
            strcat(path[j - 1], parsed[j]); //this might break who knows
            j++;
        }
    } else {
        //add current directory/whatever
        char this_dir[256];

        getcwd(this_dir, sizeof(this_dir));

        int j = 1;
        while (parsed[j] != NULL) {
            path[j - 1] = this_dir;
            strcat(path[j - 1], "/");
            strcat(path[j - 1], parsed[j]); //this might break who knows
            j++;
        }
    }
}

/**
 * Return the index of the pipe, otherwise return 0
 */
int find_redirection(char **parsed) {
    int i = 1;
    while (parsed[i] != NULL) {
        if (strncmp(parsed[i], ">", 1) == 0) {
            return i;
        }
        i++;
    }
    return 0;
}

int handle_redirection(char **parsed, int index) {
    //first need to figure out what the output file is so whatever is after parsed[index]
    if (parsed[index + 1] == NULL) {
        //no output file given
        fprintf(stderr, "An error has occurred\n");
    } else if (parsed[index + 2] != NULL) {
        //extra output file given
        fprintf(stderr, "An error has occurred\n");
    } else {
        //we have a theoretical output file
        return 1;
    }
    return 0;
}