/**
 * @file token.hpp
 *
 * @brief Header file for defining the token
 */

#pragma once
#include <cstdint>
#include <string>

/**
 * @brief All tokens types
 */
typedef enum : uint8_t {
    TOK_CHAR,                               /**< 'char' type keyword */
    TOK_SHORT,                              /**< 'short' type keyword */
    TOK_INT,                                /**< 'int' type keyword */
    TOK_LONG,                               /**< 'long' type keyword */
    TOK_FLOAT,                              /**< 'float' type keyword */
    TOK_DOUBLE,                             /**< 'double' type keyword */
    TOK_BOOL,                               /**< 'bool' type keyword */
    
    TOK_LET,                                /**< 'let' keyword for variable definition */
    TOK_FUN,                                /**< 'fun' keyword for function definition */
    TOK_IF,                                 /**< 'if' keyword for conditionally expression operator */
    TOK_ELSE,                               /**< 'else' keyword for else branch in conditionally expression operator */
    TOK_FOR,                                /**< 'for' keyword for `for` cycle definition */
    TOK_WHILE,                              /**< 'while' keyword for `while` cycle definition */
    TOK_CONST,                              /**< 'const' keyword */

    TOK_ID,                                 /**< Identifier */
    TOK_SHORT_LIT,                          /**< Integer (16 bits) literal */
    TOK_INT_LIT,                            /**< Integer (32 bits) literal */
    TOK_LONG_LIT,                           /**< Integer (64 bits) literal */
    TOK_FLOAT_LIT,                          /**< Floating point (32 bits) literal */
    TOK_DOUBLE_LIT,                         /**< Floating point (64 bits) literal */
    TOK_BOOLEAN_LIT,                        /**< Boolean literal */
    TOK_STRING_LIT,                         /**< String literal */
    TOK_CHARACTER_LIT,                      /**< Character literal */

    TOK_OP_PLUS,                            /**< '+' operator */
    TOK_OP_PLUS_EQ,                         /**< '+=' operator */
    TOK_OP_MINUS,                           /**< '-' operator */
    TOK_OP_MINUS_EQ,                        /**< '-=' operator */
    TOK_OP_MULT,                            /**< '*' operator */
    TOK_OP_MULT_EQ,                         /**< '*=' operator */
    TOK_OP_DIV,                             /**< '/' operator */
    TOK_OP_DIV_EQ,                          /**< '/=' operator */
    TOK_OP_MODULO,                          /**< '%' operator */
    TOK_OP_MODULO_EQ,                       /**< '%=' operator */
    TOK_OP_EQ,                              /**< '=' operator */
    TOK_OP_EQ_EQ,                           /**< '==' operator */
    TOK_OP_NOT_EQ_EQ,                       /**< '!=' operator */
    TOK_OP_GT,                              /**< '>' operator */
    TOK_OP_GT_EQ,                           /**< '>=' operator */
    TOK_OP_LS,                              /**< '<' operator */
    TOK_OP_LS_EQ,                           /**< '<=' operator */
    TOK_OP_L_NOT,                           /**< '!' (logical negative) operator */
    TOK_OP_L_AND,                           /**< '&&' (logical and) operator */
    TOK_OP_L_OR,                            /**< '||' (logical or) operator */
    TOK_OP_COMMA,                           /**< ',' operator */
    TOK_OP_DOT,                             /**< '.' operator */
    TOK_OP_COLON,                           /**< ':' operator */
    TOK_OP_SEMICOLON,                       /**< ';' operator */
    TOK_OP_LPAREN,                          /**< '(' operator */
    TOK_OP_RPAREN,                          /**< ')' operator */
    TOK_OP_LBRACE,                          /**< '{' operator */
    TOK_OP_RBRACE,                          /**< '}' operator */
    TOK_OP_LBRACKET,                        /**< '[' operator */
    TOK_OP_RBRACKET,                        /**< ']' operator */
    TOK_OP_QUESTION                         /**< '?' operator */
} TokenType;

/**
 * @brief Token structure
 */
struct Token {
    TokenType type;                         /**< Token type */
    std::string value;                      /**< Token value */

    uint32_t line;                          /**< Token line coordinate */
    uint32_t column;                        /**< Token column coordinate */
    uint32_t pos;                           /**< Position of the first token character from source */
    std::string file_name;                  /**< Name of the file containing the token */

    Token(TokenType t, std::string v, uint32_t l, uint32_t c, uint32_t p, std::string fn) : type(t), value(v), line(l), column(c), pos(p), file_name(fn) {}
};