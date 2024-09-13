#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "riscv.h"

const char *COMMENT_START = "## start";
const char *COMMENT_CYCLES = "## cycles";
const int BUFFER_SIZE = 256;
const int DEFAULT_NO_OF_INSTS = 50;
int DEBUG = 0;

/**
 * Prints the values of the 32 registers to stderr, one per line.
 * Since the values are printed to stderr, use:
 *
 *     printf("r0 = %d\n", registers->r[0]);
 *     // above is equivalent to fprintf(stdout, "r0 = %d\n", registers->r[0])
 *
 * for debugging purposes, since printf outputs to stdout.
 */
void print_registers(registers_t *registers)
{
    for (int i = 0; i < 32; i++)
    {
        fprintf(stderr, "r[%d] = 0x%x\n", i, registers->r[i]);
    }
}

/**
 * Converts all the characters to lowercase in the given string
 */
void strlower(char *s)
{
    // If the string is not null-terminated, the length is bound by BUFFER_SIZE
    int len = strnlen(s, BUFFER_SIZE);
    for (int i = 0; i < len; i++)
    {
        s[i] = tolower(s[i]);
    }
}

/**
 * Return a pointer to the first non-space character in the string
 */
char *ltrim_spaces(char *s)
{
    while (isspace(*s))
        s++;
    return s;
}

/**
 * Handles the start comment, which initializes a register to a value.
 *
 *     ## start[<r>] = <v>
 *
 * sets the register <r> to the value <v>
 */
void handle_start(char *s, registers_t *registers)
{
    char *start = strstr(s, COMMENT_START);
    strsep(&start, "[");
    int r = atoi(strsep(&start, "]"));
    strsep(&start, "=");
    int v = (int)strtol(start, NULL, 0);
    if (r > 0 && r < 32)
    {
        registers->r[r] = v;
    }
}

char **handle_init(int no_of_instructions, registers_t *registers)
{
    // Allocate memory for `no_of_instructions` instructions
    char **program = (char **)calloc(no_of_instructions, sizeof(char *));
    for (int i = 0; i < no_of_instructions; i++)
    {
        program[i] = (char *)calloc(BUFFER_SIZE, sizeof(char));
        if (program[i] == NULL)
            fprintf(stderr, "Cannot allocate space for program\n");
    }
    // Call init() code with the allocated registers, program pointer, and program size
    init(registers, program, no_of_instructions);
    return program;
}

int main(int argc, char *argv[])
{
    // Allocate memory for 32 registers and return a pointer to the memory
    registers_t *registers = (registers_t *)calloc(1, sizeof(registers_t));
    char **program = NULL;
    // Set number of instructions
    int no_of_instructions = -1;
    // Use a temporary buffer to read a string into
    char buffer[BUFFER_SIZE];
    // Keep track of the current instruction number
    int inst_no = -1;
    // Read from stdin until an EOF (sent using Ctrl+D)
    while (fgets(buffer, BUFFER_SIZE, stdin) != NULL)
    {
        // Convert the newline at the end of a line to a null-terminator
        buffer[strcspn(buffer, "\r\n")] = 0;
        char *cycles = strstr(buffer, COMMENT_CYCLES);
        if (cycles)
        {
            cycles = ltrim_spaces(cycles);
            strsep(&cycles, "=");
            no_of_instructions = (int)strtol(cycles, NULL, 0);
            program = handle_init(no_of_instructions, registers);
            continue;
        }
        // Handles the comment which initializes a register to a value
        char *start = strstr(buffer, COMMENT_START);
        if (start)
        {
            handle_start(buffer, registers);
            continue;
        }
        // Find the first occurrence of a comment, and terminate string there
        char *comment = strchr(buffer, '#');
        if (comment)
        {
            *comment = '\0';
        }
        // Normalize our input by converting the string to lowercase
        strlower(buffer);

        char *instruction = ltrim_spaces(buffer);
        if (strlen(instruction) == 0)
        {
            continue;
        }
        else
        {
            if (no_of_instructions == -1)
            {
                no_of_instructions = DEFAULT_NO_OF_INSTS;
                // printf("Using default number of cycles: %d\n", DEFAULT_NO_OF_INSTS);
                program = handle_init(no_of_instructions, registers);
            }
            inst_no++;
            if (inst_no < no_of_instructions)
            {
                strcpy(program[inst_no], instruction);
            }
            else
            {
                fprintf(stderr, "Number of cycles beyond expectation.\n");
                end();
                return 1;
            }
        }
    }
    // After entire program is read from stdin, call evaluate_program() code
    evaluate_program();

    // Print the register values
    print_registers(registers);
    end();
}
