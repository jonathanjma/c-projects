# RISC-V Interpreter

Interpreter for 32 bit RISC-V assembly programs. Register and RAM instructions are simulated
by using hash table and linked lists. To run the interpreter on an assembly file, simply
paste the file into `stdin` after running `make all`. The interpreter also supports custom
directives such as `## start[<register>] = <hex value>` to set the starting value of a
register and `## cycles = <max cycles>` to limit the number of cycles executed. Multiple
test assembly files are provided- `gcd.txt` finds the GCD of 2 numbers while `test1.txt, test2.txt,
and test3.txt` check common and edge cases for various instructions.
