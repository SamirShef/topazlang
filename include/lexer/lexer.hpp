/**
 * @file lexer.hpp
 *
 * @brief Header file for defining the lexer
 */

#include "token.hpp"
#include <vector>
#include <map>

/**
 * @brief Lexer class
 */
class Lexer {
private:
    std::string source;                                         /**< Source code on Topaz */
    size_t source_len;                                          /**< Length of source code (optimization) */
    uint32_t pos;                                               /**< Position index into source code */
    uint32_t line;                                              /**< Line coordinate */
    uint32_t column;                                            /**< Column coordinate */
    std::string file_name;                                      /**< Name of the file containing the token */
    std::map<std::string, TokenType> keywords {                 /**< Keywords table */
        {"char", TOK_CHAR},
        {"short", TOK_SHORT},
        {"int", TOK_INT},
        {"long", TOK_LONG},
        {"float", TOK_FLOAT},
        {"double", TOK_DOUBLE},
        {"bool", TOK_BOOL},
        {"let", TOK_LET},
        {"fun", TOK_FUN},
        {"if", TOK_IF},
        {"else", TOK_ELSE},
        {"for", TOK_FOR},
        {"while", TOK_WHILE}
    };

public:
    Lexer(std::string src, std::string fn) : source(src), source_len(src.length()), pos(0), line(1), column(1), file_name(fn) {}

    /**
     * @brief Method for tokenizing source code
     *
     * This method tokenizing source code into vector of tokens and returns it
     *
     * @return Vector of tokens after tokenizing
     */
    std::vector<Token> tokenize();

private:
    /**
     * @brief Method for tokenizing identifier token
     *
     * This method tokenizing identifier token and returns it
     * If token value matches a keyword from the table, the keyword token is returned
     * If token value matches a 'true' or 'false', the boolean literal token is returned
     *
     * @return Token as identifier, keyword or boolean literal
     */
    Token tokenize_id();

    /**
     * @brief Method for tokenizing number literal
     *
     * This method tokenizing number literal token and returns it
     * If literal contains a suffix, the literal corresponding to the suffix is returned
     * Otherwise returns double literal if contains dot and integer literal otherwise
     *
     * @return Number literal token
     */
    Token tokenize_number_lit();

    /**
     * @brief Method for tokenizing string literal
     *
     * This method tokenizing string literal token and returns it
     * If literal contains escape-sequence, then it handling
     *
     * @return String literal token
     */
    Token tokenize_string_lit();

    /**
     * @brief Method for tokenizing character literal
     *
     * This method tokenizing character literal token and returns it
     * If literal contains escape-sequence, then it handling
     * If length of literal not equal 1, then throwing exception
     *
     * @return Character literal token
     */
    Token tokenize_character_lit();

    /**
     * @brief Method for tokenizing operator
     *
     * This method tokenizing operator token and returns it
     * If method does not recognized operator, then throwing exception
     *
     * @return Operator token
     */
    Token tokenize_op();

    /**
     * @brief Method for skipping comments
     *
     * This method skipping comments (single-line yet)
     *
     */
    void skip_comments();

    /**
     * @brief Method for getting escape-sequence in string or character literal
     *
     * This method getting escape-sequence character and returns it
     * If method does not recognized escape-sequence, than throwing exception
     *
     * @return Escape-sequence character
     */
    const char get_escape_sequence();

    /**
     * @brief Method for getting character from source code by lexer pos and passed offset
     *
     * This method getting character from source code by lexer pos and passed offset
     * If pos + offset out of bounds of range source code, then throwing exception
     *
     * @param rpos Offset
     *
     * @return Character from source code
     */
    const char peek(int32_t rpos = 0) const;

    /**
     * @brief Method for skipping current character from source code and returns it
     *
     * This method caching current character from source code, skip it, changing lexer's pos, line and column and return cached character
     *
     * @return Skipped character
     */
    const char advance();
};