/**
 * @file token.hpp
 *
 * @brief Header file for defining the token
 */

#pragma once
#include <cstdint>
#include <sstream>
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
    TOK_NOTH,                               /**< 'noth' type keyword (only for functions) */
    
    TOK_LET,                                /**< 'let' keyword for variable definition */
    TOK_FUN,                                /**< 'fun' keyword for function definition */
    TOK_IF,                                 /**< 'if' keyword for conditionally expression operator */
    TOK_ELSE,                               /**< 'else' keyword for else branch in conditionally expression operator */
    TOK_FOR,                                /**< 'for' keyword for `for` cycle definition */
    TOK_WHILE,                              /**< 'while' keyword for `while` cycle definition */
    TOK_CONST,                              /**< 'const' keyword */
    TOK_RETURN,                             /**< 'return' keyword */

    TOK_ID,                                 /**< Identifier */
    TOK_CHARACTER_LIT,                      /**< Character literal */
    TOK_SHORT_LIT,                          /**< Integer (16 bits) literal */
    TOK_INT_LIT,                            /**< Integer (32 bits) literal */
    TOK_LONG_LIT,                           /**< Integer (64 bits) literal */
    TOK_FLOAT_LIT,                          /**< Floating point (32 bits) literal */
    TOK_DOUBLE_LIT,                         /**< Floating point (64 bits) literal */
    TOK_BOOLEAN_LIT,                        /**< Boolean literal */
    TOK_STRING_LIT,                         /**< String literal */

    TOK_OP_PLUS,                            /**< '+' operator */
    TOK_OP_PLUS_EQ,                         /**< '+=' operator */
    TOK_OP_INC,                             /**< '++' operator */
    TOK_OP_MINUS,                           /**< '-' operator */
    TOK_OP_MINUS_EQ,                        /**< '-=' operator */
    TOK_OP_DEC,                             /**< '--' operator */
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
    TOK_OP_QUESTION,                        /**< '?' operator */
    TOK_OP_NEXT                             /**< '->' operator */
} TokenType;

/**
 * @brief Token structure
 */
struct Token {
    TokenType type;                         /**< Token type */
    std::string value;                      /**< Token value */

    uint32_t line;                          /**< Token line coordinate */
    uint32_t column;                        /**< Token column coordinate */
    std::string file_name;                  /**< Name of the file containing the token */

    Token(TokenType t, std::string v, uint32_t l, uint32_t c, std::string fn) : type(t), value(v), line(l), column(c), file_name(fn) {}

    /**
     * @brief Method for converting token to string
     *
     * This method converting token structure to string. If type of token is unsupported
     *
     * @return String as converted token
     */
    std::string to_str() {
        std::stringstream ss;
        switch (type) {
            case TOK_CHAR:
                ss << "char";
                break;
            case TOK_SHORT:
                ss << "short";
                break;
            case TOK_INT:
                ss << "int";
                break;
            case TOK_LONG:
                ss << "long";
                break;
            case TOK_FLOAT:
                ss << "float";
                break;
            case TOK_DOUBLE:
                ss << "double";
                break;
            case TOK_BOOL:
                ss << "bool";
                break;
            case TOK_NOTH:
                ss << "noth";
                break;
            case TOK_LET:
                ss << "let";
                break;
            case TOK_FUN:
                ss << "fun";
                break;
            case TOK_IF:
                ss << "if";
                break;
            case TOK_ELSE:
                ss << "else";
                break;
            case TOK_FOR:
                ss << "for";
                break;
            case TOK_WHILE:
                ss << "while";
                break;
            case TOK_CONST:
                ss << "const";
                break;
            case TOK_RETURN:
                ss << "return";
                break;
            case TOK_ID:
                ss << "id";
                break;
            case TOK_CHARACTER_LIT:
                ss << "char_lit";
                break;
            case TOK_SHORT_LIT:
                ss << "short_lit";
                break;
            case TOK_INT_LIT:
                ss << "int_lit";
                break;
            case TOK_LONG_LIT:
                ss << "long_lit";
                break;
            case TOK_FLOAT_LIT:
                ss << "float_lit";
                break;
            case TOK_DOUBLE_LIT:
                ss << "double_lit";
                break;
            case TOK_BOOLEAN_LIT:
                ss << "bool_lit";
                break;
            case TOK_STRING_LIT:
                ss << "string_lit";
                break;
            case TOK_OP_PLUS:
                ss << "plus_op";
                break;
            case TOK_OP_PLUS_EQ:
                ss << "plus_eq_op";
                break;
            case TOK_OP_MINUS:
                ss << "minus_op";
                break;
            case TOK_OP_MINUS_EQ:
                ss << "minus_eq_op";
                break;
            case TOK_OP_MULT:
                ss << "mult_op";
                break;
            case TOK_OP_MULT_EQ:
                ss << "mult_eq_op";
                break;
            case TOK_OP_DIV:
                ss << "div_op";
                break;
            case TOK_OP_DIV_EQ:
                ss << "div_eq_op";
                break;
            case TOK_OP_MODULO:
                ss << "mod_op";
                break;
            case TOK_OP_MODULO_EQ:
                ss << "mod_eq_op";
                break;
            case TOK_OP_EQ:
                ss << "eq_op";
                break;
            case TOK_OP_EQ_EQ:
                ss << "eq_eq_op";
                break;
            case TOK_OP_NOT_EQ_EQ:
                ss << "not_eq_eq_op";
                break;
            case TOK_OP_GT:
                ss << "gt_op";
                break;
            case TOK_OP_GT_EQ:
                ss << "gt_eq_op";
                break;
            case TOK_OP_LS:
                ss << "ls_op";
            case TOK_OP_LS_EQ:
                ss << "ls_eq_op";
                break;
            case TOK_OP_L_NOT:
                ss << "l_not_op";
                break;
            case TOK_OP_L_AND:
                ss << "l_and_op";
                break;
            case TOK_OP_L_OR:
                ss << "l_or_op";
                break;
            case TOK_OP_COMMA:
                ss << "comma";
                break;
            case TOK_OP_DOT:
                ss << "dot";
                break;
            case TOK_OP_COLON:
                ss << "colon";
                break;
            case TOK_OP_SEMICOLON:
                ss << "semicolon";
                break;
            case TOK_OP_LPAREN:
                ss << "l_paren";
                break;
            case TOK_OP_RPAREN:
                ss << "r_paren";
                break;
            case TOK_OP_LBRACE:
                ss << "l_brace";
                break;
            case TOK_OP_RBRACE:
                ss << "r_brace";
                break;
            case TOK_OP_LBRACKET:
                ss << "l_bracket";
                break;
            case TOK_OP_RBRACKET:
                ss << "r_bracket";
                break;
            case TOK_OP_QUESTION:
                ss << "question";
                break;
            case TOK_OP_NEXT:
                ss << "next";
                break;
            default:
                ss << "<unknown>";
                break;
        }
        ss << " : '" << value << "' (" << column << ':' << line << ')';
        return ss.str();
    }
};