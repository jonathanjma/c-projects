#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "linkedlist.h"
#include "hashtable.h"
#include "riscv.h"

/************** BEGIN HELPER FUNCTIONS ***************/
const int R_TYPE = 0;
const int I_TYPE = 1;
const int MEM_TYPE = 2;
const int U_TYPE = 3;
const int B_TYPE = 4;
const int UNKNOWN_TYPE = 5;

/**
 * Return the type of instruction for the given operation
 * Available options are R_TYPE, I_TYPE, MEM_TYPE, U_TYPE, B_TYPE, UNKNOWN_TYPE
 */
static int get_op_type(char *op)
{
    const char *r_type_op[] = {"add", "sub", "and", "slt", "sll", "sra"};
    const char *i_type_op[] = {"addi", "andi"};
    const char *mem_type_op[] = {"lw", "lb", "sw", "sb"};
    const char *u_type_op[] = {"lui"};
    const char *b_type_op[] = {"beq"};
    for (int i = 0; i < (int)(sizeof(r_type_op) / sizeof(char *)); i++)
    {
        if (strcmp(r_type_op[i], op) == 0)
        {
            return R_TYPE;
        }
    }
    for (int i = 0; i < (int)(sizeof(i_type_op) / sizeof(char *)); i++)
    {
        if (strcmp(i_type_op[i], op) == 0)
        {
            return I_TYPE;
        }
    }
    for (int i = 0; i < (int)(sizeof(mem_type_op) / sizeof(char *)); i++)
    {
        if (strcmp(mem_type_op[i], op) == 0)
        {
            return MEM_TYPE;
        }
    }
    for (int i = 0; i < (int)(sizeof(u_type_op) / sizeof(char *)); i++)
    {
        if (strcmp(u_type_op[i], op) == 0)
        {
            return U_TYPE;
        }
    }
    for (int i = 0; i < (int)(sizeof(b_type_op) / sizeof(char *)); i++)
    {
        if (strcmp(b_type_op[i], op) == 0)
        {
            return B_TYPE;
        }
    }
    return UNKNOWN_TYPE;
}
/*************** END HELPER FUNCTIONS ****************/

registers_t *registers;
char **program;
int no_of_instructions;

#define BUFFER_SIZE 256
char buf[BUFFER_SIZE];
int pc;
hashtable_t *memory;

void init(registers_t *starting_registers, char **input_program, int given_no_of_instructions)
{
    registers = starting_registers;
    program = input_program;
    no_of_instructions = given_no_of_instructions;

    pc = 0;
    memory = ht_init(2000);
}

void end()
{
    // Free everything from memory
    free(registers);
    for (int i = 0; i < no_of_instructions; i++)
    {
        free(program[i]);
    }
    free(program);
    ht_free(memory);
}

/**
 * Copy of the C standard library isspace function
 */
int isspace(int c)
{
    return c == '\t' || c == '\n' ||
           c == '\v' || c == '\f' || c == '\r' || c == ' ';
}

/**
 * Given a string, trim any leading or trailing space surrounding it
 */
char *trim_spaces(char *s)
{
    while (isspace(*s)) // trim leading space
        s++;

    char *end = s + strlen(s) - 1;
    while (end > s && isspace(*end)) // trim trailing space
        end--;
    end[1] = '\0';

    return s;
}

/**
 * Sign extend an x bit immediate value to 32 bits
 */
int sign_extend(int imm_v, int imm_num_bits)
{
    // only want the x LSBs: << by 32-x to remove extra bits and
    // >> by 32-x to sign extend to 32 bits (since >> is arithmetic shift on ints)
    return (imm_v << (32 - imm_num_bits)) >> (32 - imm_num_bits);
}

/**
 * Given a hex or decimal string representing an x bit immediate value,
 * convert it to an int and sign extend it to 32 bits.
 */
int process_imm(char *imm, int imm_num_bits)
{
    int imm_v = strtol(imm, NULL, 0);
    return sign_extend(imm_v, imm_num_bits);
}

void step(char *instruction)
{
    // Extracts and returns the substring before the first space character,
    // by replacing the space character with a null-terminator.
    // `instruction` is MODIFIED IN PLACE to point to the next character
    // after the space. See `man strsep` for how this library function works.
    char *op = strsep(&instruction, " ");
    int op_type = get_op_type(op); // type of instruction

    if (op_type == R_TYPE)
    {
        char *rd = trim_spaces(strsep(&instruction, ","));
        char *r1 = trim_spaces(strsep(&instruction, ","));
        char *r2 = trim_spaces(instruction);
        printf("%s/%s/%s/%s\n", op, rd, r1, r2);

        int rd_r = atoi(rd + 1); // offset by 1 to remove x
        int r1_r = atoi(r1 + 1);
        int r2_r = atoi(r2 + 1);

        // disallow writing to x0
        if (rd_r == 0)
        {
            return;
        }

        if (strcmp("add", op) == 0)
        {
            registers->r[rd_r] = registers->r[r1_r] + registers->r[r2_r];
        }
        else if (strcmp("sub", op) == 0)
        {
            registers->r[rd_r] = registers->r[r1_r] - registers->r[r2_r];
        }
        else if (strcmp("and", op) == 0)
        {
            registers->r[rd_r] = registers->r[r1_r] & registers->r[r2_r];
        }
        else if (strcmp("slt", op) == 0)
        {
            registers->r[rd_r] = (registers->r[r1_r] < registers->r[r2_r]) ? 1 : 0;
        }
        else if (strcmp("sll", op) == 0)
        {
            // only use 5 LSBs for shift amount
            registers->r[rd_r] = registers->r[r1_r] << (registers->r[r2_r] & 0x0000001F);
        }
        else if (strcmp("sra", op) == 0)
        {
            // >> is arithmetic shift by default since ints are signed
            registers->r[rd_r] = registers->r[r1_r] >> (registers->r[r2_r] & 0x0000001F);
        }
    }
    else if (op_type == I_TYPE)
    {
        char *rd = trim_spaces(strsep(&instruction, ","));
        char *r1 = trim_spaces(strsep(&instruction, ","));
        char *imm = trim_spaces(instruction);
        printf("%s/%s/%s/%s\n", op, rd, r1, imm);

        int rd_r = atoi(rd + 1);
        int r1_r = atoi(r1 + 1);
        int imm_v = process_imm(imm, 12);

        if (rd_r == 0)
        {
            return;
        }

        if (strcmp("addi", op) == 0)
        {
            registers->r[rd_r] = registers->r[r1_r] + imm_v;
        }
        else if (strcmp("andi", op) == 0)
        {
            registers->r[rd_r] = registers->r[r1_r] & imm_v;
        }
    }
    else if (op_type == MEM_TYPE)
    {
        // target is rd for loads and r2 for stores
        char *target = trim_spaces(strsep(&instruction, ","));
        char *imm = trim_spaces(strsep(&instruction, "("));
        char *r1 = trim_spaces(strsep(&instruction, ")"));
        printf("%s/%s/%s/%s\n", op, target, imm, r1);

        int target_r = atoi(target + 1);
        int imm_v = process_imm(imm, 12);
        int r1_r = atoi(r1 + 1);

        int addr = imm_v + registers->r[r1_r];

        // disallow writing to x0 for loads
        if (op[0] == 'l' && target_r == 0)
        {
            return;
        }

        if (strcmp("lw", op) == 0)
        {
            // shift each byte to correct position and assemble int
            int byte1 = ht_get(memory, addr);
            int byte2 = ht_get(memory, addr + 1) << 8;
            int byte3 = ht_get(memory, addr + 2) << 16;
            int byte4 = ht_get(memory, addr + 3) << 24;
            registers->r[target_r] = byte1 + byte2 + byte3 + byte4;
        }
        else if (strcmp("lb", op) == 0)
        {
            registers->r[target_r] = sign_extend(ht_get(memory, addr), 8);
        }
        else if (strcmp("sw", op) == 0)
        {
            // shift each byte to LSB position and mask away the other bytes
            ht_add(memory, addr, registers->r[target_r] & 0x000000FF);
            ht_add(memory, addr + 1, (registers->r[target_r] >> 8) & 0x000000FF);
            ht_add(memory, addr + 2, (registers->r[target_r] >> 16) & 0x000000FF);
            ht_add(memory, addr + 3, (registers->r[target_r] >> 24) & 0x000000FF);
        }
        else if (strcmp("sb", op) == 0)
        {
            ht_add(memory, addr, registers->r[target_r] & 0x000000FF);
        }
    }
    else if (op_type == U_TYPE)
    {
        char *rd = trim_spaces(strsep(&instruction, ","));
        char *imm = trim_spaces(instruction);
        printf("%s/%s/%s\n", op, rd, imm);

        int rd_r = atoi(rd + 1);
        int imm_v = process_imm(imm, 20);

        if (rd_r == 0)
        {
            return;
        }

        if (strcmp("lui", op) == 0)
        {
            registers->r[rd_r] = imm_v << 12;
        }
    }
    else if (op_type == B_TYPE)
    {
        char *r1 = trim_spaces(strsep(&instruction, ","));
        char *r2 = trim_spaces(strsep(&instruction, ","));
        char *imm = trim_spaces(instruction);
        printf("%s/%s/%s/%s\n", op, r1, r2, imm);

        int r1_r = atoi(r1 + 1);
        int r2_r = atoi(r2 + 1);
        int imm_v = process_imm(imm, 13);

        if (strcmp("beq", op) == 0 && registers->r[r1_r] == registers->r[r2_r])
        {
            // assume that all offsets are valid
            pc += imm_v - 4;
            printf("branch\n");
        }
    }
}

void evaluate_program()
{
    // Logic for evaluating the program
    while (pc / 4 < no_of_instructions)
    {
        // copy instructions so strsep does not modify original program
        strncpy(buf, program[pc / 4], BUFFER_SIZE);
        step(buf);
        pc += 4;
    }
}
