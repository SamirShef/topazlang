/**
 * @file main.cpp
 *
 * @brief Compiler entry point
 */

#include "../include/lexer/lexer.hpp"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <cstdlib>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: topazc \"path/to/src.tp\"\n";
        exit(1);
    }
    
    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Error openning file: does not exist!\n";
        exit(1);
    }
    
    std::filesystem::path file_path = std::filesystem::absolute(argv[1]);
    std::string content = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    Lexer lexer(content, file_path.string());
    std::vector<Token> tokens = lexer.tokenize();
    for (const Token& token : tokens) {
        std::cout << (int)token.type << " : '" << token.value << "' (" << token.line << ':' << token.column << ")\n";
    }
    return 0;
}