For this project, there is a Makefile which will handle all of the compilation. Browse to the folder that contains the Makefile and run one of these commands:

* `make`: Build a single executable `pl0` (optimizations on, debug and asserts off)
* `make debug`: Build the executable in debug configuration (optimizations off, debug and asserts on)
* `make debug+`: Build the executable in debug configuration and using clang's Address Sanitizer (eustis doesn't have clang so this won't work there) (run `make clean` before changing between a normal build and a debug+ build)
* `make graph`: Build the executable and run it on `input.txt` to produce the Graphviz input files, then run those through Graphviz to produce the rendered pdfs (eustis doesn't have Graphviz so this won't work there)
* `make run`: Build the executable in debug configuration and run it to produce all output files
* `make archive`: Build the executable in debug configuration and run it on the input file to produce all of their output files, then create the zip containing all of the source code, test cases, input and output files, Makefile, .gitignore, README.md, and other documentation
* `make clean`: Remove all built products (including the executable in the top-level directory and the zip)

Additionally, the Makefile understands the following variables:

* `make ... VERBOSE=1`: Echo all commands before running them
* `make ... WITH_BISON=1`: Build with support for the Bison generated parser.
* `make ... WITH_LLVM=1`: Build with support for the LLVM code generator.

If you simply run command `make` an executable `pl0` will be produced. 

After the PL/0 toolchain is built, simply run command `./pl0` to process `input.txt` and produce all the output files.

`pl0` also allows for command line switches. Run `./pl0 --help` to see a list of the available options. Example output:

```
Usage: ./pl0 [-acdhlmnprsv]
Options:
    -h, --help               Display this help message
    -l, --tee-token-list     Duplicate token list to stdout
    -s, --tee-symbol-table   Duplicate symbol table to stdout
    -a, --tee-disassembly    Duplicate disassembly to stdout
    -v, --tee-program-trace  Duplicate program trace to stdout
    -m, --tee-machine-code   Duplicate machine code to stdout
    -p, --markdown           Pretty print output as Markdown
    -c, --compile-only       Compile only, do not run
    -r, --run-only           Run only, do not compile
    -d, --debug              Run program in the PM/0 debugger
    -n, --no-stacktrace      Don't write stacktrace while running (MUCH FASTER!)
        --parser=rdp         Use the recursive descent parser (default)
        --parser=bison       Use the Bison-generated parser
        --codegen=pm0        Use the PM/0 code generator (default)
        --codegen=llvm       Use the LLVM code generator
```

This compiler supports calling procedures with parameters and returning a functional value. Arrays are currently not supported.
