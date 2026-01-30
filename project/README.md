# Simple Shell (mysh)

## Overview
This project implements a simple Unix-like shell (`mysh`) written in C.  
The shell supports both **interactive mode** and **batch mode** (input redirected from a file).

The implementation includes:
- Built-in commands (e.g., `help`, `quit`, `set`, `echo`, `my_ls`, etc.)
- Variable expansion using `$var`
- Batch execution via input redirection

---

## Starter Code Usage
**This project uses the starter code provided by the OS course staff.**

The following files and structure were initially provided and then extended:
- `shell.c / shell.h`
- `interpreter.c`
- `shellmemory.c / shellmemory.h`
- Main program structure and command dispatch logic

All additional logic was written on top of the provided starter code.

--- 

## Compiling
To compile or recompile the code, run the following commands:
```
cd src
make clean
make mysh
```

---

## Test Cases
All test cases are located in the `test-cases/` directory.
The executable file `mysh` has been copied over from `src/` into `test-cases/` for simplicity.
To run a test, use the following command:
```
cd test-cases
./mysh < [test_name].txt
```

An automated Python test script was used to run all test cases in batch mode and compare outputs using diff.

Tests can be run with 
```bash
python3 ../run_tests.py # important! make sure to be inside inputs/ to run the testing 
```
and new tests can be added by appending entries to the TESTS list as ```(input_file, expected_file, [required_files])```, where required files are copied into the test working directory before execution.



