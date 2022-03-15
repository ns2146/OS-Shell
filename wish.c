#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

void commander(char *arg, char **parsed, char **path, int path_size);
int parser(char *input, char** output);
int runBuiltin(char **input);
int tryPath(char **parsed, char **path, int path_size);
void editPath(char **parsed, char **path, int path_size);
int find_redirection(char **parsed);
int handle_redirection(char **parsed, int index);
int find_redirection_no_spaces(char *string);

char usable_path[100];
char *output;
FILE *destination = NULL;

int main(int argc, char *argv[]) {
    char **path;
    int path_size = 0;
    int currentLine = 1;
    char **parsed = malloc(100 * sizeof(char*));
    path = malloc(100 * sizeof(char*));
    path[0] = "/bin";
    path_size = 1;
    

    if (argc == 1) {
        //INTERACTIVE MODE
        while(1) {
            char str[100];
            printf("wish>");
            fgets(str, 100, stdin);

            if (strncmp(str, "\n", 1) != 0) {
                commander(str, parsed, path, path_size);
            } 
            memset(parsed, 0, sizeof(parsed));
        }
    } else if (argc == 2) {
        //BATCH MODE
        char str[100];
        FILE *fp;

        if ( (fp = fopen(argv[1], "r")) == NULL ) {
            fprintf(stderr, "An error has occurred\n");
            return 1;
        }

        while (fgets(str, 100, fp) != NULL) {
            //printf("Line%d: %s", currentLine, str);
            commander(str, parsed, path, path_size);
            currentLine++;
        }
        memset(parsed, 0, sizeof(parsed));
        
        fclose(fp);
    } else {
        fprintf(stderr, "An error has occurred\n");
        return 1;
    }
}

void commander (char *arg, char **parsed, char **path, int path_size) {
    int length = parser(arg, parsed);

    if (length == 0) {
        return;
    }

    int index_p = find_redirection(parsed);

    if (index_p > 0) {
        //redirection shenanigans
        int handle = handle_redirection(parsed, index_p);
        if (handle == -1) {
            return;
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
        editPath(parsed, path, path_size);
    } else if ( strncmp(parsed[0], "pwd", 3) == 0 ) {
        char directory[400];
        getcwd(directory, sizeof(directory));
        printf("%s\n", directory);
    } else if ( strncmp(parsed[0], "ls", 2) == 0 ) {
        if (tryPath(parsed, path, path_size) == 1) {
            runBuiltin(parsed);
        } else {
            fprintf(stderr, "An error has occurred\n");
        }
    } else if ( strncmp(parsed[0], "cat", 3) == 0 ) {
        if (tryPath(parsed, path, path_size) == 1) {
            runBuiltin(parsed);
        } else {
            fprintf(stderr, "An error has occurred\n");
        }
    } else {
        if (tryPath(parsed, path, path_size)) {
            runBuiltin(parsed);
        } else {
            fprintf(stderr, "An error has occurred\n");
        }
    }
}

//returns length of output array
int parser(char *input, char **output) {

    char *found;
    char *found_redirect;

    int i = 0;
    while ( (found = strsep(&input, " \n")) != NULL ) {
        if (strlen(found) > 0) {

            int index = find_redirection_no_spaces(found);
            if (index != -1) {
                //we have a redirection at index
                found_redirect = strsep(&found, ">");
                char *dest = strsep(&found, ">");
                
                output[i] = found_redirect;
                
                i++;
                char *temp = ">";
                output[i] = temp;
                
                i++;
                output[i] = dest;
            } else {
                output[i] = found;
            }
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
        if (destination) {
            int destination_file_no = fileno(destination);
            dup2(destination_file_no, 1);
            dup2(destination_file_no, 2);
        } 
        execv(usable_path, input);
        exit(-1);
    } else {
        //in parent
        //rc = child's id
        //need to close parent
        wait(NULL);

        if (destination) {
            fclose(destination);
            destination = NULL;
        } 

        return 0;
    }
    return 1;
}

int tryPath(char **parsed, char **path, int path_size) {
    char test_path[100];

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

void editPath(char **parsed, char **path, int path_size) {
    //use memset to clear path
    memset(path, 0, path_size);
    path_size = 0;

    int i = 1;
    while (parsed[i] != NULL) {
        path[i - 1] = malloc((strlen(parsed[i]) + 1) * sizeof(char));
        path[i - 1] = parsed[i];
        path_size++;
        i++;
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
        return -1;
    } else if (parsed[index + 2] != NULL) {
        //extra output file given
        fprintf(stderr, "An error has occurred\n");
        return -1;
    } else {
        //we have a theoretical output file

        if ((destination = fopen(parsed[index + 1], "w")) == NULL) {
            fprintf(stderr, "An error has occurred");
            return 0;
        }

        memset(&parsed[index], 0, 2);
        parsed[index] = NULL;
        
        return 1;
    }
    return 0;
}

int find_redirection_no_spaces(char *string) {
    int length = strlen(string);

    if (length == 1) {
        return -1;
    }

    for (int i = 0; i < length; i++) {
        if (string[i] == '>') {
            return i;
        }
    }
    return -1;
}