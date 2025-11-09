/**
 * @file exception.hpp
 *
 * @brief Header file for defining thrown exceptions by the compiler
 */

#include <cstdint>
#include <string>

/**
 * @brief Subsystem from which the exception throwed
 */
enum SubsystemType {
    SUB_LEXER,                                  /**< Lexer subsystem */
    SUB_PARSER,                                 /**< Parser subsystem */
    SUB_CODEGEN                                 /**< Code generator subsystem */
};

/**
 * @brief Function for throwing exception
 *
 * This function throwing exception based passed arguments
 *
 * @param type Subsystem from which the exception throwned
 * @param msg Exception message
 * @param line Line where exception throwed
 * @param file_name File where exception throwed
 */
void throw_excpetion(SubsystemType type, std::string msg, uint32_t line, std::string file_name);