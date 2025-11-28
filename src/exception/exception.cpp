/**
 * @file exception.cpp
 *
 * @brief exception.hpp implementation
 */

#include "../../include/exception/exception.hpp"
#include <iostream>

/**
 * @brief Function for converting passed subsystem type into string
 *
 * @param type Subsystem type
 *
 * @return Converted type into string
 */
std::string convert_subsystem_type_to_string(SubsystemType type) {
    switch (type) {
        case SUB_LEXER:
            return "lexer";
        case SUB_PARSER:
            return "parser";
        case SUB_SEMANTIC:    
            return "semantic";
        case SUB_CODEGEN:
            return "codegen";
    }
}

void throw_exception(SubsystemType type, std::string msg, uint32_t line, std::string file_name) {
    std::cerr << "\033[31mSubsystem \033[0m'" << convert_subsystem_type_to_string(type) << "'\033[31m handled the error\n";
    std::cerr << "Compilation error at:\033[0m " << file_name << ':' << line << "\n\033[31m" << msg << "\033[0m\n";
    exit(1);
}