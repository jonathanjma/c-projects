/**
 * The interface for a register file containing 32 individual registers
 */
struct registers
{
    int r[32];
};
typedef struct registers registers_t;

/**
 * Initializes the internal state with the given set of register values,
 * pointer to program, and number of instructions.
 * This method is called ONCE at the very beginning of the interpreter.
 */
void init(registers_t *starting_registers, char **input_program, int given_no_of_instructions);

/**
 * Free every memory previously allocated.
 */
void end();

/**
 * Evaluates the given instruction. This method is NOT called by the 
 * interpreter.
 */
void step(char *instruction);

/**
 * Evaluates the program.
 * This method is called ONCE after the interpreter reads in the
 * whole program.
 */
void evaluate_program();