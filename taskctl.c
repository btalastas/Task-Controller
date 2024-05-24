/* This is the only file you should update and submit. */

/* Fill in your Name and GNumber in the following two comment fields
 * Name: Bjorn Talastas
 * GNumber: 01335804
 */

#include <sys/wait.h>
#include "taskctl.h"
#include "parse.h"
#include "util.h"

/*
 * Data structure to hold instructions and point to another instruction.
 */
typedef struct instruction_struct_linked
{
    Instruction *instruct;
    char *task;
    char *inFile;
    char *outFile;
    char *cmd;
    char **argv;
    int state;
    int instructionNumber;
    int exitCode;
    int type;
    pid_t pid;
    struct instruction_struct_linked *next;
} instruction_link_t;
/*
 * Linked list structure of Instructions.
 */
typedef struct instruction_struct_list
{
    int instructionCounter;
    instruction_link_t *head;

} instruction_list_t;

/* Constants */
#define DEBUG 1

// global list structure to hold instructions
instruction_list_t *INSTRUCTION_LIST;

// uncomment if you want to use any of these:

//#define NUM_PATHS 2
//#define NUM_INSTRUCTIONS 10

static const char *task_path[] = {"./", "/usr/bin/", NULL};
// static const char *instructions[] = {"quit", "help", "list", "purge", "exec", "bg", "kill", "suspend", "resume", "pipe", NULL};

/*
 * Funcation that searches through instruction list and checks to see if given task num is inside our instruction list
 * Returns 1 if task num is found, otherwise 0.
 */
instruction_link_t *searchInstructionList(int taskNum)
{
    instruction_link_t *currentInstruction = INSTRUCTION_LIST->head;

    while (currentInstruction)
    {
        if (currentInstruction->instructionNumber == taskNum)
        {

            return currentInstruction;
        }
        currentInstruction = currentInstruction->next;
    }

    return currentInstruction;
}

/*
 * Function that blocks SIGCHLD signal.
 */
void blockSIGCHLD()
{
    sigset_t mask, prevMask;
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &prevMask);
}

/*
 * Function that unblocks blocked SIGCHLD signal.
 */
void unblockSIGCHLD()
{
    sigset_t mask, prevMask;
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_UNBLOCK, &mask, &prevMask);
}
/*
 * Removes selected task from instruction list.
 */
void removeInstruction(int taskNum)
{
    instruction_link_t *currentInstruction = INSTRUCTION_LIST->head, *prevInstruction = NULL;
    int found = 0;

    if (taskNum == currentInstruction->instructionNumber) // Remove from head
    {

        INSTRUCTION_LIST->head = INSTRUCTION_LIST->head->next;
    }
    else
    {
        while (currentInstruction && !found)
        {
            if (currentInstruction->instructionNumber == taskNum)
            {
                found = 1;
            }
            else
            {
                prevInstruction = currentInstruction;
                currentInstruction = currentInstruction->next;
            }
        }

        if (!currentInstruction->next)
        {

            prevInstruction->next = NULL;
        }
        else
        {

            prevInstruction->next = currentInstruction->next;
            currentInstruction->next = NULL;
        }
    }
    INSTRUCTION_LIST->instructionCounter--;
}

/*
 * Creates new instruction_link_t instruction and adds it into instruction list.
 */
void addToInstructionList(instruction_list_t *instructionList, Instruction *instruction, char **argv, char *cmd)
{
    int taskNum = 1;
    instruction_link_t *currentInstruction = malloc(sizeof(instruction_link_t));
    instruction_link_t *currentHead = instructionList->head;
    instruction_link_t *prevInstruction = NULL;
    currentInstruction->instruct = instruction;
    currentInstruction->task = string_copy(instruction->instruct);
    currentInstruction->state = LOG_STATE_READY;
    currentInstruction->instructionNumber = taskNum;
    currentInstruction->argv = clone_argv(argv);
    currentInstruction->cmd = string_copy(cmd);
    currentInstruction->exitCode = 0;
    currentInstruction->pid = 0;
    currentInstruction->type = 0;
    if (!instructionList->head) // instruction list is empty
    {

        instructionList->head = currentInstruction;
        currentInstruction->next = NULL;
    }
    else // instruction list is not empty
    {

        if (taskNum < currentHead->instructionNumber) // new instruction number is less than first instruction in list
        {
            // Current instruction is new head
            currentInstruction->next = currentHead;
            instructionList->head = currentInstruction;
            instructionList->instructionCounter++;
        }
        else // more than 1 instruction in instruction list
        {
            while (currentHead && taskNum <= currentHead->instructionNumber)
            {
                taskNum++;
                prevInstruction = currentHead;
                currentHead = currentHead->next;
            }
            currentInstruction->instructionNumber = taskNum;
            if (!currentHead) // Add to end of instruction list
            {

                prevInstruction->next = currentInstruction;
            }
            else
            {
                if (prevInstruction)
                {
                    prevInstruction->next = currentInstruction;
                    currentInstruction->next = currentHead;
                }
                else
                {
                    currentHead->next = currentInstruction;
                }
            }
        }
    }
    instructionList->instructionCounter++;
    log_kitc_task_init(currentInstruction->instructionNumber, currentInstruction->cmd);
}
/*
 * Function to check if first word user inputs is a built-in instruction.
 * Returns int value corresponding to instruction number.
 */
int checkForBuiltInInstructions(Instruction *instruct)
{
    char *instruction = instruct->instruct;
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

/*
 * Executes the kill instruction.
 * Sends a SIGINT signal to the desired instruction given a task num.
 */
void executeKillInstruction(Instruction *instruct)
{
    instruction_link_t *currentInstruction = searchInstructionList(instruct->num);

    if (!currentInstruction)
    {
        log_kitc_task_num_error(instruct->num);
    }
    else
    {
        if (currentInstruction->state == LOG_STATE_READY || currentInstruction->state == LOG_STATE_FINISHED || currentInstruction->state == LOG_STATE_KILLED)
        {
            log_kitc_status_error(currentInstruction->instructionNumber, currentInstruction->state);
        }
        else
        {
            kill(currentInstruction->pid, SIGINT);
            log_kitc_sig_sent(LOG_CMD_KILL, currentInstruction->instructionNumber, currentInstruction->pid);
        }
    }
}

/*
 * Executes the suspend instruction.
 * Sends a SIGSTP signal to the desired instruction given a task num.
 */
void executeSuspendInstruction(Instruction *instruct)
{
    instruction_link_t *currentInstruction = searchInstructionList(instruct->num);

    if (!currentInstruction)
    {
        log_kitc_task_num_error(instruct->num);
    }
    else
    {
        if (currentInstruction->state == LOG_STATE_READY || currentInstruction->state == LOG_STATE_FINISHED || currentInstruction->state == LOG_STATE_KILLED)
        {
            log_kitc_status_error(currentInstruction->instructionNumber, currentInstruction->state);
        }
        else
        {
            currentInstruction->state = LOG_STATE_SUSPENDED;
            kill(currentInstruction->pid, SIGTSTP);
            log_kitc_sig_sent(LOG_CMD_SUSPEND, currentInstruction->instructionNumber, currentInstruction->pid);
        }
    }
}

/*
 * Executes the resume instruction.
 * Sends a SIGCONT signal to the desired instruction given a task num
 */
void executeResumeInstruction(Instruction *instruct)
{
    instruction_link_t *currentInstruction = searchInstructionList(instruct->num);

    if (!currentInstruction)
    {
        log_kitc_task_num_error(instruct->num);
    }
    else
    {
        if (currentInstruction->state == LOG_STATE_READY || currentInstruction->state == LOG_STATE_FINISHED || currentInstruction->state == LOG_STATE_KILLED)
        {
            log_kitc_status_error(currentInstruction->instructionNumber, currentInstruction->state);
        }
        else
        {
            currentInstruction->type = LOG_FG;
            currentInstruction->state = LOG_STATE_RUNNING;
            kill(currentInstruction->pid, SIGCONT);
            log_kitc_sig_sent(LOG_CMD_RESUME, currentInstruction->instructionNumber, currentInstruction->pid);
        }
    }
}
/*
 * Iterates through instruction list to log each instruction
 */
void printLinkedInstructions()
{
    instruction_link_t *currentInstruction = INSTRUCTION_LIST->head;
    while (currentInstruction)
    {
        log_kitc_task_info(currentInstruction->instructionNumber, currentInstruction->state, currentInstruction->exitCode, currentInstruction->pid, currentInstruction->cmd);
        currentInstruction = currentInstruction->next;
    }
}

/*
 * Executes the list built-in instruction. Prints out number of tasks inside instruction list and task name.
 */
void executeListInstruction()
{
    log_kitc_num_tasks(INSTRUCTION_LIST->instructionCounter);
    // Iterate through linked list and log each instruction
    printLinkedInstructions(INSTRUCTION_LIST);
}

/*
 * Executes the purge instruction
 * Removes an instruction inside instruction list given a task num
 */
void executePurgeInstruction(Instruction *instruct)
{
    instruction_link_t *currentInstruction = searchInstructionList(instruct->num);

    if (!currentInstruction) // current instruction does not exist
    {
        log_kitc_task_num_error(instruct->num);
    }
    else
    {
        if (currentInstruction->state == LOG_STATE_RUNNING || currentInstruction->state == LOG_STATE_SUSPENDED) // current instruction is busy
        {
            // Log error but do not remove task
            log_kitc_status_error(currentInstruction->instructionNumber, currentInstruction->state);
        }
        else
        {
            // Log successful purge after removal
            removeInstruction(instruct->num);
            log_kitc_purge(currentInstruction->instructionNumber);
        }
    }
}

/*
 * Creates a new file descriptor for reading only.
 */
int openInfile(char *inFile)
{
    int newFd = open(inFile, O_RDONLY, 0644);
    return newFd;
}

/*
 * Creates a new file descriptor for writing only.
 */
int openOutFile(char *outFile)
{
    int newFd = open(outFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    return newFd;
}

/*
 * Executes the pipe instruction.
 * Task num 1 runs as a background process which redirects its output to the write end of the pipe.
 * Task num 2 runs as a foreground process which redirects its input to the read end of the pipe.
 * Task num 1 gets reaped by signal handler while task num 2 gets reaped by parent process.
 */
void executePipeInstruction(Instruction *instruct)
{
    if (instruct->num == instruct->num2)
    {
        log_kitc_pipe_error(instruct->num);
        return;
    }
    instruction_link_t *instructionOne = searchInstructionList(instruct->num);
    instruction_link_t *instructionTwo = searchInstructionList(instruct->num2);

    // Both task nums do not exist
    if (!instructionOne && !instructionTwo)
    {
        log_kitc_task_num_error(instruct->num);
        log_kitc_task_num_error(instruct->num2);
    }
    else if (!instructionOne) // task 1 does not exist
    {
        log_kitc_task_num_error(instruct->num);
    }
    else if (!instructionTwo) // task 2 does not exist
    {
        log_kitc_task_num_error(instruct->num2);
    }
    else // both task nums exist
    {
        blockSIGCHLD();
        instructionOne->type = LOG_BG;
        instructionTwo->type = LOG_FG;
        // both tasks are busy
        if ((instructionOne->state == LOG_STATE_RUNNING || instructionOne->state == LOG_STATE_SUSPENDED) && (instructionTwo->state == LOG_STATE_RUNNING || instructionTwo->state == LOG_STATE_SUSPENDED))
        {
            log_kitc_status_error(instructionOne->instructionNumber, instructionOne->state);
            log_kitc_status_error(instructionTwo->instructionNumber, instructionTwo->state);
        }
        // task 1 is busy
        else if (instructionOne->state == LOG_STATE_RUNNING || instructionOne->state == LOG_STATE_SUSPENDED)
        {
            log_kitc_status_error(instructionOne->instructionNumber, instructionOne->state);
        }
        // task 2 is busy
        else if (instructionTwo->state == LOG_STATE_RUNNING || instructionTwo->state == LOG_STATE_SUSPENDED)
        {
            log_kitc_status_error(instructionTwo->instructionNumber, instructionTwo->state);
        }
        // both tasks are not busy
        else
        {
            int child_status2;
            int pipe_array[2] = {0};
            int succ = pipe(pipe_array);
            if (succ == -1)
            {
                log_kitc_file_error(instruct->num, LOG_FILE_PIPE);
                return;
            }
            // in_pipe write end, out_pipe read end
            int in_pipe = pipe_array[1], out_pipe = pipe_array[0];
            char *buffer = malloc(sizeof(char *));
            buffer[0] = '\0';

            // Create foreground process
            pid_t taskTwo = fork();
            // true if child
            if (taskTwo == 0)
            {
                unblockSIGCHLD();
                close(in_pipe);
                dup2(out_pipe, STDIN_FILENO);
                // execute process
                buffer = strncat(buffer, task_path[0], strlen(task_path[0]) + 1);
                buffer = strncat(buffer, instructionTwo->task, strlen(instructionTwo->task) + 1);
                execv(buffer, instructionTwo->argv);
                memset(buffer, '\0', sizeof(char));
                buffer = strncat(buffer, task_path[1], strlen(task_path[1]) + 1);
                buffer = strncat(buffer, instructionTwo->task, strlen(instructionTwo->task) + 1);
                execv(buffer, instructionTwo->argv);
                log_kitc_exec_error(instructionTwo->cmd);
                exit(1);
            }
            else
            {
                // create background process
                pid_t taskOne = fork();
                if (taskOne == 0)
                {
                    unblockSIGCHLD();
                    setpgid(0, 0);
                    close(out_pipe);
                    dup2(in_pipe, STDOUT_FILENO);
                    buffer = strncat(buffer, task_path[0], strlen(task_path[0]) + 1);
                    buffer = strncat(buffer, instructionOne->task, strlen(instructionOne->task) + 1);
                    execv(buffer, instructionOne->argv);
                    memset(buffer, '\0', sizeof(char));
                    buffer = strncat(buffer, task_path[1], strlen(task_path[1]) + 1);
                    buffer = strncat(buffer, instructionOne->task, strlen(instructionOne->task) + 1);
                    execv(buffer, instructionOne->argv);
                    log_kitc_exec_error(instructionOne->cmd);
                    exit(1);
                }
                instructionOne->pid = taskOne;
                instructionTwo->pid = taskTwo;
                log_kitc_pipe(instructionOne->instructionNumber, instructionTwo->instructionNumber);
                log_kitc_status_change(instructionOne->instructionNumber, instructionOne->pid, instructionOne->type, instructionOne->cmd, LOG_START);
                log_kitc_status_change(instructionTwo->instructionNumber, instructionTwo->pid, instructionTwo->type, instructionTwo->cmd, LOG_START);
                close(in_pipe);
                close(out_pipe);
                waitpid(taskTwo, &child_status2, 0);
                instructionTwo->state = LOG_STATE_FINISHED;
                instructionTwo->exitCode = WEXITSTATUS(child_status2);
                unblockSIGCHLD();
            }
        }
    }
}

/*
 * Executes the background instruction.
 * Parent does not wait until instruction is finished. Signal handler reaps background instruction.
 */
void executeBgInstruction(Instruction *instruct)
{
    instruction_link_t *currentInstruction = searchInstructionList(instruct->num);

    // current Instruction does not exist
    if (!currentInstruction)
    {
        log_kitc_task_num_error(instruct->num);
    }
    else
    {
        // current instruction is busy
        if (currentInstruction->state == LOG_STATE_RUNNING || currentInstruction->state == LOG_STATE_SUSPENDED)
        {
            log_kitc_status_error(currentInstruction->instructionNumber, currentInstruction->state);
        }
        else
        {
            int fd;
            char *buffer = malloc(sizeof(char *));
            buffer[0] = '\0';
            blockSIGCHLD();
            pid_t child_pid = fork(); // create new child process
            currentInstruction->type = LOG_BG;
            if (child_pid == 0)
            {
                setpgid(0, 0);
                if (instruct->infile && !instruct->outfile)
                {

                    log_kitc_redir(currentInstruction->instructionNumber, LOG_REDIR_IN, instruct->infile);
                    // Open file
                    fd = openInfile(instruct->infile);
                    // redirect FD
                    dup2(fd, STDIN_FILENO);
                }
                else if (instruct->outfile && !instruct->infile)
                {

                    log_kitc_redir(currentInstruction->instructionNumber, LOG_REDIR_OUT, instruct->outfile);
                    // Open file
                    fd = openOutFile(instruct->outfile);
                    // redirect FD
                    dup2(fd, STDOUT_FILENO);
                }
                else if (instruct->infile && instruct->outfile)
                {

                    log_kitc_redir(currentInstruction->instructionNumber, LOG_REDIR_IN, instruct->infile);
                    log_kitc_redir(currentInstruction->instructionNumber, LOG_REDIR_OUT, instruct->outfile);
                    // open infile and redirect stdin fd
                    fd = openInfile(instruct->infile);
                    dup2(fd, STDIN_FILENO);

                    // open outfile and redirect stdout fd
                    fd = openOutFile(instruct->outfile);
                    dup2(fd, STDOUT_FILENO);
                }
                unblockSIGCHLD();
                buffer = strncat(buffer, task_path[0], strlen(task_path[0]) + 1);
                buffer = strncat(buffer, currentInstruction->task, strlen(currentInstruction->task) + 1);
                execv(buffer, currentInstruction->argv);
                memset(buffer, '\0', sizeof(char));
                buffer = strncat(buffer, task_path[1], strlen(task_path[1]) + 1);
                buffer = strncat(buffer, currentInstruction->task, strlen(currentInstruction->task) + 1);
                execv(buffer, currentInstruction->argv);
                log_kitc_exec_error(currentInstruction->cmd);
                exit(1);
            }
            else
            {
                currentInstruction->pid = child_pid;
                currentInstruction->state = LOG_STATE_RUNNING;
                log_kitc_status_change(currentInstruction->instructionNumber, child_pid, currentInstruction->type, currentInstruction->cmd, LOG_START);
                unblockSIGCHLD();
            }
        }
    }
}

/*
 * Executes the exec instruction.
 * Parent waits for instruction to finish to be reaped.
 */
void executeExecInstruction(Instruction *instruct)
{
    instruction_link_t *currentInstruction = searchInstructionList(instruct->num);

    // current Instruction does not exist
    if (!currentInstruction)
    {
        log_kitc_task_num_error(instruct->num);
    }
    else
    {
        // current instruction is busy
        if (currentInstruction->state == LOG_STATE_RUNNING || currentInstruction->state == LOG_STATE_SUSPENDED)
        {
            log_kitc_status_error(currentInstruction->instructionNumber, currentInstruction->state);
        }
        else
        {
            int child_status, fd;
            char *buffer = malloc(sizeof(char *));
            buffer[0] = '\0';
            blockSIGCHLD();
            pid_t child_pid = fork(); // create new child process
            currentInstruction->pid = child_pid;
            currentInstruction->type = LOG_FG;
            if (child_pid == 0) // only child process will execute
            {

                if (instruct->infile && !instruct->outfile)
                {

                    log_kitc_redir(currentInstruction->instructionNumber, LOG_REDIR_IN, instruct->infile);
                    // Open file
                    fd = openInfile(instruct->infile);
                    // redirect FD
                    dup2(fd, STDIN_FILENO);
                }
                else if (instruct->outfile && !instruct->infile)
                {

                    log_kitc_redir(currentInstruction->instructionNumber, LOG_REDIR_OUT, instruct->outfile);
                    // Open file
                    fd = openOutFile(instruct->outfile);
                    // redirect FD
                    dup2(fd, STDOUT_FILENO);
                }
                else if (instruct->infile && instruct->outfile)
                {

                    log_kitc_redir(currentInstruction->instructionNumber, LOG_REDIR_IN, instruct->infile);
                    log_kitc_redir(currentInstruction->instructionNumber, LOG_REDIR_OUT, instruct->outfile);
                    // open infile and redirect stdin fd
                    fd = openInfile(instruct->infile);
                    dup2(fd, STDIN_FILENO);

                    // open outfile and redirect stdout fd
                    fd = openOutFile(instruct->outfile);
                    dup2(fd, STDOUT_FILENO);
                }
                unblockSIGCHLD();
                buffer = strncat(buffer, task_path[0], strlen(task_path[0]) + 1);
                buffer = strncat(buffer, currentInstruction->task, strlen(currentInstruction->task) + 1);
                execv(buffer, currentInstruction->argv);
                memset(buffer, '\0', sizeof(char));
                buffer = strncat(buffer, task_path[1], strlen(task_path[1]) + 1);
                buffer = strncat(buffer, currentInstruction->task, strlen(currentInstruction->task) + 1);
                execv(buffer, currentInstruction->argv);
                log_kitc_exec_error(currentInstruction->cmd);
                exit(1);
            }
            else
            {
                log_kitc_status_change(currentInstruction->instructionNumber, child_pid, currentInstruction->type, currentInstruction->cmd, LOG_START);
                wait(&child_status);
                unblockSIGCHLD();
                free(buffer);
            }
        }
    }
}
/*
 * Executes built-in instruction based on int value.
 */
void selectBuiltInInstruction(int instruction, Instruction *instruct)
{
    switch (instruction)
    {
    case 1: // help instruction
        log_kitc_help();
        break;
    case 2: // quit instruction
        log_kitc_quit();
        exit(0);
        break;
    case 3: // list instruction
        executeListInstruction();
        break;
    case 4: // purge instruction
        executePurgeInstruction(instruct);
        break;
    case 5: // exec instruction
        executeExecInstruction(instruct);
        break;
    case 6: // bg instruction
        executeBgInstruction(instruct);
        break;
    case 7: // pipe instruction
        executePipeInstruction(instruct);
        break;
    case 8: // kill instruction
        executeKillInstruction(instruct);
        break;
    case 9: // suspend instruction
        executeSuspendInstruction(instruct);
        break;
    case 10: // resume instruction
        executeResumeInstruction(instruct);
        break;
    }
}

/*
 * Function that allocates and initializes memory for instruction list.
 * Returns pointer to a instruction_list_t struct.
 */
instruction_list_t *createInstructionList()
{
    instruction_list_t *newInstructionList;
    newInstructionList = malloc(sizeof(instruction_list_t *));
    newInstructionList->head = NULL;
    newInstructionList->instructionCounter = 0;
    return newInstructionList;
}

/*
 * Function that searches instruction for the given PID
 */
instruction_link_t *searchPIDinstructionList(pid_t pid)
{
    instruction_link_t *currentInstruction = INSTRUCTION_LIST->head;

    while (currentInstruction)
    {
        if (currentInstruction->pid == pid)
        {
            return currentInstruction;
        }
        currentInstruction = currentInstruction->next;
    }
    return currentInstruction;
}

/*
 * Handler function for SIGCHLD
 * Signal catches whenever a child process changes states - Terminated, Suspended, or Continued.
 * Handler reaps processes in a loop and exits if they're no child processes left to reap.
 */
void kitcHandleSIGCHLD()
{
    int status = 0;
    pid_t pid = 0;
    instruction_link_t *currentInstruction;

    do
    {
        pid = waitpid(-1, &status, WNOHANG | WUNTRACED);
        if (pid > 0)
        {
            if (WIFEXITED(status))
            {
                write(STDOUT_FILENO, "hi", 6);
                currentInstruction = searchPIDinstructionList(pid);
                currentInstruction->state = LOG_STATE_FINISHED;
                currentInstruction->exitCode = WEXITSTATUS(status);
                log_kitc_status_change(currentInstruction->instructionNumber, currentInstruction->pid, currentInstruction->type, currentInstruction->cmd, LOG_TERM);
            }
            else
            {
                if (WIFSIGNALED(status))
                {
                    currentInstruction = searchPIDinstructionList(pid);
                    currentInstruction->state = LOG_STATE_KILLED;
                    currentInstruction->exitCode = WEXITSTATUS(status);
                    log_kitc_status_change(currentInstruction->instructionNumber, currentInstruction->pid, currentInstruction->type, currentInstruction->cmd, LOG_TERM_SIG);
                }
                else if (WIFCONTINUED(status))
                {
                    currentInstruction = searchPIDinstructionList(pid);
                    currentInstruction->state = LOG_STATE_RUNNING;
                    log_kitc_status_change(currentInstruction->instructionNumber, currentInstruction->pid, currentInstruction->type, currentInstruction->cmd, LOG_RESUME);
                }
                else if (WIFSTOPPED(status))
                {
                    currentInstruction = searchPIDinstructionList(pid);
                    currentInstruction->state = LOG_STATE_SUSPENDED;
                    log_kitc_status_change(currentInstruction->instructionNumber, currentInstruction->pid, currentInstruction->type, currentInstruction->cmd, LOG_SUSPEND);
                    return;
                }
            }
        }
    } while (pid >= 0);
}

/* Handler function for SIGINT
 * Signal catches only for foreground processes.
 */
void kitcHandleSIGINT()
{
    log_kitc_ctrl_c();
}

/*
 * Handler function for SIGTSTP.
 * Signal catches only for foreground processes.
 */
void kitcHandleSIGTSTP()
{
    log_kitc_ctrl_z();
}

/*
 * User defined signal handler.
 * Catches SIGINT, SIGTSTP, SIGCONT, and SIGCHLD signals
 */
void kitcSignalHandler(int sig)
{
    // Signal Handler function
    // Need to handle SIGINT, SIGSTP, SIGCONT, SIGINT, SIGCHLD
    switch (sig)
    {
    case SIGINT:
        kitcHandleSIGINT();
        break;
    case SIGCHLD:
        kitcHandleSIGCHLD();
        break;
    case SIGTSTP:
        kitcHandleSIGTSTP();
        break;
    default:
        break;
    }
}

/* The entry of your task controller program */
int main()
{
    char cmdline[MAXLINE]; /* Command line */
    char *cmd = NULL;
    int instructionNumber = 0;
    struct sigaction sa = {0};
    sa.sa_handler = kitcSignalHandler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);
    sigaction(SIGCHLD, &sa, NULL);
    INSTRUCTION_LIST = createInstructionList();
    /* Intial Prompt and Welcome */
    log_kitc_intro();
    log_kitc_help();

    /* Shell looping here to accept user command and execute */
    while (1)
    {
        char *argv[MAXARGS + 1]; /* Argument list */
        Instruction inst;        /* Instruction structure: check parse.h */

        /* Print prompt */
        log_kitc_prompt();

        /* Read a line */
        // note: fgets will keep the ending '\n'
        errno = 0;
        if (fgets(cmdline, MAXLINE, stdin) == NULL)
        {
            if (errno == EINTR)
            {
                continue;
            }
            exit(-1);
        }

        if (feof(stdin))
        { /* ctrl-d will exit text processor */
            exit(0);
        }

        /* Parse command line */
        if (strlen(cmdline) == 1) /* empty cmd line will be ignored */
            continue;

        cmdline[strlen(cmdline) - 1] = '\0'; /* remove trailing '\n' */

        cmd = malloc(strlen(cmdline) + 1); /* duplicate the command line */
        snprintf(cmd, strlen(cmdline) + 1, "%s", cmdline);

        /* Bail if command is only whitespace */
        if (!is_whitespace(cmd))
        {
            initialize_command(&inst, argv); /* initialize arg lists and instruction */
            parse(cmd, &inst, argv);         /* call provided parse() */

            if (DEBUG)
            { /* display parse result, redefine DEBUG to turn it off */
                debug_print_parse(cmd, &inst, argv, "main (after parse)");
            }

            /* After parsing: your code to continue from here */
            /*================================================*/
            instructionNumber = checkForBuiltInInstructions(&inst);

            if (instructionNumber > 0)
            {

                // Built-in instruction
                selectBuiltInInstruction(instructionNumber, &inst);
            }
            else
            {
                // Not built-in instruction
                // Add instruction to instruction list
                addToInstructionList(INSTRUCTION_LIST, &inst, argv, cmd);
            }
        } // end if(!is_whitespace(cmd))

        free(cmd);
        cmd = NULL;
        free_command(&inst, argv);
    } // end while(1)

    return 0;
} // end main()
