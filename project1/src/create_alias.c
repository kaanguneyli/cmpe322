#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

struct index_and_line
{
    int index;
    char *string;
};

/*
    Takes a filename and the arguments array as argument
    Writes the alias name and the content of the alias to the file
    In other words, it writes everything but the string in the indexes 0 and 2
    Returns the line that it has written to and also the content
    Able to realias too
*/
struct index_and_line createAlias(char* filename, char *args[]) {
    int size = strlen(args[1]) + 2;
    int wordcount = 0;
    for (int a=3; args[a] != NULL; a++) {
        size += strlen(args[a]);
        wordcount++;
    } 
    char *append_line = (char *) malloc(size);
    strcpy(append_line, args[1]);
    strcat(append_line, " ");
    for (int a=3; a<wordcount+3; a++) {
        strcat(append_line, args[a]);
        strcat(append_line, " ");
    }
    strcat(append_line, "\n");

    FILE  *file = fopen(filename, "a+");
    FILE  *temp = fopen("temp_file.txt", "a+"); // creates a temp file for realiasing
    char  *line;
    size_t len = 0;
    bool   isOld = false;
    int    line_to_change = 0;

    while (getline(&line, &len, file) != -1) {
        /* write the content of original file to the temp file
           if there is realiasing do not take the info from file, take it from the variable here */
        char *line_copy = strdup(line);
        char *name = strtok(line, " ");
        if (strcmp(name, args[1]) == 0) {
            isOld = true;
            fprintf(temp, "%s", append_line);
            free(line_copy);
        } else {
            fprintf(temp, "%s", line_copy);
        }
        line_to_change++;
    }
    free(line);

    if (! isOld) { // if not realiasing append to the file
        fclose(temp);
        fprintf(file, "%s", append_line);
        fclose(file);
    } else {     // if realias remove the original file and rename the temp
        fclose(file);
        fclose(temp);
        remove(filename);
        rename("temp_file.txt", filename);
    }
    remove("temp_file.txt");
    struct index_and_line ret;
    ret.index = line_to_change; 
    ret.string = strdup(append_line);
    free(append_line);
    return ret;
}
