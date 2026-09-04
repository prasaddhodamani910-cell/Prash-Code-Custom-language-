#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>

#include "error.h"
#include "lexer.h"
#include "parser.h"
#include "sema.h"
#include "codegen.h"

using namespace toy;

void printUsage() {
    std::cerr << "Usage: prashc [options] <input.pd>\n"
              << "Options:\n"
              << "  --emit-asm     Only emit assembly (.s file)\n"
              << "  --dump-ast     Parse and dump the AST\n"
              << "  --dump-tokens  Lex and dump tokens\n"
              << "  -o <file>      Specify output executable name (default: a.out)\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string inputFile;
    std::string outputFile = "a.out";
    bool emitAsm = false;
    bool dumpAst = false;
    bool dumpTokens = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--emit-asm") {
            emitAsm = true;
        } else if (arg == "--dump-ast") {
            dumpAst = true;
        } else if (arg == "--dump-tokens") {
            dumpTokens = true;
        } else if (arg == "-o") {
            if (i + 1 < argc) {
                outputFile = argv[++i];
            } else {
                std::cerr << "Error: Missing argument for -o\n";
                return 1;
            }
        } else if (arg[0] != '-') {
            inputFile = arg;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage();
            return 1;
        }
    }

    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified.\n";
        printUsage();
        return 1;
    }

    std::ifstream inFile(inputFile);
    if (!inFile) {
        std::cerr << "Error: Could not open file " << inputFile << "\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << inFile.rdbuf();
    std::string source = buffer.str();

    ErrorReporter reporter(source, inputFile);

    Lexer lexer(source, reporter);
    auto tokens = lexer.tokenize();

    if (reporter.hasErrors()) return 1;

    if (dumpTokens) {
        for (const auto& t : tokens) {
            std::cout << tokenTypeToString(t.type) << " '" << t.text << "' at line " << t.loc.line << ":" << t.loc.column << "\n";
        }
        return 0;
    }

    Parser parser(tokens, reporter);
    auto ast = parser.parse();

    if (reporter.hasErrors()) return 1;

    if (dumpAst) {
        ast->dump();
        return 0;
    }

    Sema sema(reporter);
    sema.analyze(ast.get());

    if (reporter.hasErrors()) return 1;

    std::string asmFile = inputFile + ".s";
    {
        CodeGen codegen(asmFile);
        codegen.generate(ast.get());
    } // CodeGen destructor flushes & closes the .s file here

    if (emitAsm) {
        return 0;
    }

    // Assemble and link
    std::string assembleCmd = "clang -c " + asmFile + " -o " + inputFile + ".o";
    std::string linkCmd = "ld.lld -e _start -o " + outputFile + " " + inputFile + ".o";
    
    if (std::system(assembleCmd.c_str()) != 0) {
        std::cerr << "Error assembling\n";
        return 1;
    }
    if (std::system(linkCmd.c_str()) != 0) {
        std::cerr << "Error linking\n";
        return 1;
    }
    
    // Clean up temp files
    std::remove(asmFile.c_str());
    std::remove((inputFile + ".o").c_str());

    return 0;
}
