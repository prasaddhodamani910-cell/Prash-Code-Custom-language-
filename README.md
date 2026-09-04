# Prash code Language Compiler

This is a complete toy programming language compiler that generates native AArch64 (ARM64) assembly for Linux, specifically targeted at Android/Termux without relying on `libc`. The compiled binary interacts directly with the Linux kernel using syscalls.

## Language Features

- **Data Types**: 64-bit signed integers, booleans (true/false)
- **Arithmetic**: `+`, `-`, `*`, `/`, `%`
- **Comparison**: `==`, `!=`, `<`, `>`, `<=`, `>=`
- **Logic**: `&&`, `||`, `!`
- **Control Flow**: `if/else`, `while` loops
- **Functions**: Declarations with parameters and `return`, including recursion
- **Scope**: Block scope with variable shadowing
- **Output**: Built-in `print(expr)` function

### Examples

**Fibonacci:**
def fib(n):
    if n <= 1:
        return n
    return fib(n - 1) + fib(n - 2)

def main():
    print(fib(10))
    return 0
```

## Build Instructions

To build the compiler, simply run:
```sh
make
```
This will produce the `prashc` binary in the `build/` directory.

To build and run all tests, run:
```sh
make test
```

## Usage

```sh
# Compile to a native binary
./build/prashc examples/hello.pd -o hello
./hello

# Only generate assembly (outputs to .s file)
./build/prashc --emit-asm examples/fibonacci.pd

# Dump the Abstract Syntax Tree
./build/prashc --dump-ast examples/factorial.pd

# Dump lexical tokens
./build/prashc --dump-tokens examples/fizzbuzz.pd
```

## Architecture

The compiler consists of the following phases:

```
Source Code
    |
    v
[ Lexer ] -----> Tokens
    |
    v
[ Parser ] ----> AST (Abstract Syntax Tree)
    |
    v
[ Sema ] ------> Checked AST (Scope/Types)
    |
    v
[ CodeGen ] ---> AArch64 Assembly (.s)
    |
    v
[ Clang ] -----> Object File (.o) -> Executable
```

### AArch64 Code Generation
The code generator employs a simple stack-based compilation strategy. Expressions are evaluated by pushing their results onto the data stack. Operators pop their operands from the stack, perform the operation, and push the result back. Local variables are allocated on the stack frame relative to the frame pointer (`fp`).

A small runtime function, `_print_int`, is emitted in the assembly. It converts the 64-bit signed integer to a string and outputs it to `stdout` via the Linux `write` syscall (`svc #0` with `x8=64`). The program terminates using the `exit` syscall (`x8=93`).

## Project Structure

- `src/` - Compiler source code
  - `lexer.h/cpp` - Tokenizer
  - `parser.h/cpp` - Recursive descent parser
  - `ast.h` - AST nodes and pretty-printer
  - `sema.h/cpp` - Semantic analysis (scope, variables, functions)
  - `codegen.h/cpp` - AArch64 Assembly generator
  - `error.h/cpp` - Error reporting and formatting
  - `main.cpp` - Compiler driver
- `examples/` - Example Prash code programs
- `Makefile` - Build scripts

## Future Extensions

- Add support for string literals and character arrays
- Implement a register allocator instead of relying on the stack
- Add type checking and error recovery improvements
- Implement more robust optimizations (constant folding, dead code elimination)
