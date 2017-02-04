For this project, there is a Makefile which will handle all of the compilation. Browse to the folder that contains the Makefile and run one of these commands:

* `make`: Build a single executable `pl0` (optimizations on, debug and asserts off)
* `make debug`: Build the executable in debug configuration (optimizations off, debug and asserts on)
* `make debug+`: Build the executable in debug configuration and using clang's Address Sanitizer (eustis doesn't have clang so this won't work there)
* `make graph`: Build the executable and run it on `input.txt` to produce the Graphviz input files, then run those through Graphviz to produce the rendered pdfs (eustis doesn't have Graphviz so this won't work there)
* `make run`: Build the executable in debug configuration and run it to produce all output files
* `make archive`: Build the executable in debug configuration and run it on the input file to produce all of their output files, then create the zip containing all of the source code, test cases, input and output files, Makefile, .gitignore, README.md, and other documentation
* `make clean`: Remove all built products (including the executable in the top-level directory and the zip)

If you simply run command `make` an executable `pl0` will be produced. 

After the PL/0 toolchain is built, simply run command `./pl0` to process `input.txt` and produce all the output files.

`pl0` also allows for command line switches. These switches will duplicate their corresponding output to the terminal:

* `-t` The token list
* `-s` The symbol table
* `-m` The machine code
* `-a` The disassembled code
* `-v` The virtual machine execution stack trace

Beyond these flags, the compiler driver accepts the `-c` option to compile the source without running it and a `-r` option to run the code in `mcode.txt` without compiling any code. Additionally, the VM has a debugger built in, so running `./pl0 -d` will compile the source and run it in the VM in a debug mode.

This compiler supports calling procedures with parameters and returning a functional value. Arrays are currently not supported.
