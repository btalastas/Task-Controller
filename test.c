#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int checkForBuiltInInstructions(char *instruction)
{

    if (strcmp(instruction, "help") == 0)
    {
        // Help instruction
        return 1;
    }
    else if (strcmp(instruction, "quit") == 0)
    {
        // Quit instruction
        return 2;
    }
    else if (strcmp(instruction, "list") == 0)
    {
        // List instruction
        return 3;
    }
    else if (strcmp(instruction, "purge") == 0)
    {
        // Purge instruction
        return 4;
    }
    else if (strcmp(instruction, "exec") == 0)
    {
        // Exec instruction
        return 5;
    }
    else if (strcmp(instruction, "bg") == 0)
    {
        // Bg instruction
        return 6;
    }
    else if (strcmp(instruction, "pipe") == 0)
    {
        // Pipe instruction
        return 7;
    }
    else if (strcmp(instruction, "kill") == 0)
    {
        // Kill instruction
        return 8;
    }
    else if (strcmp(instruction, "suspend") == 0)
    {
        // Suspend instruction
        return 9;
    }
    else if (strcmp(instruction, "resume") == 0)
    {
        // Resume instruction
        return 10;
    }

    // Function reaches this point if instruction does not match any built-in instruction
    return 0;
}

int main(int argc, char const *argv[])
{
    char *instruction = "list";
    char *buf = malloc(sizeof(char *));

        printf("buf: %s\n", buf);
    char *path0 = "./", *path1 = "/usr/bin/";
    char *instructArgv = "ls";
    printf("path0: %s\tpath1: %s\n", path0, path1);
    buf = strncat(buf, path0, strlen(path0) + 1);
    // buf += '\0';
    printf("buf: %s\n", buf);
    buf = strncat(buf, instructArgv, strlen(instructArgv) + 1);
    // buf += '\0';
    printf("buf: %s\n", buf);
    memset(buf, '\0', sizeof(char));
    buf = strncat(buf, path1, strlen(path1) + 1);
    // buf += '\0';
    printf("buf: %s\n", buf);
    buf = strncat(buf, instructArgv, strlen(instructArgv) + 1);
    // buf += '\0';

    printf("buf: %s\n", buf);
    printf("instruction number: %d\npath0: %s\npath1: %s\n", checkForBuiltInInstructions(instruction), path0, path1);
    return 0;
}
