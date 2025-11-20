/**
 * @file lexer.cpp
 *
 * @brief lexer.hpp implementation
 */

#include "../../include/exception/exception.hpp"
#include "../../include/lexer/lexer.hpp"
#include <iostream>
#include <sstream>

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (pos < source_len) {
        const char c = peek();
        if (c == ' ' || c == '\n') {
            advance();
        }
        else if (c == '/') {
            if (peek(1) == '/') {
                skip_comments();
            }
            else {
                tokens.push_back(tokenize_op());
            }
        }
        else if (isalpha(c)) {
            tokens.push_back(tokenize_id());
        }
        else if (isdigit(c)) {
            tokens.push_back(tokenize_number_lit());
        }
        else if (c == '\"') {
            tokens.push_back(tokenize_string_lit());
        }
        else if (c == '\'') {
            tokens.push_back(tokenize_character_lit());
        }
        else {
            tokens.push_back(tokenize_op());
        }
    }

    return tokens;
}

Token Lexer::tokenize_id() {
    std::string value;
    uint32_t tmp_l = line;
    uint32_t tmp_c = column;

    while (pos < source_len && (isalpha(peek()) || isdigit(peek()) || peek() == '_')) {
        value += advance();
    }

    if (keywords.find(value) != keywords.end()) {
        return Token(keywords[value], value, tmp_l, tmp_c, file_name);
    }
    else if (value == "true" || value == "false") {
        return Token(TOK_BOOLEAN_LIT, value, tmp_l, tmp_c, file_name);
    }
    return Token(TOK_ID, value, tmp_l, tmp_c, file_name);
}

Token Lexer::tokenize_number_lit() {
    std::string value;
    uint32_t tmp_l = line;
    uint32_t tmp_c = column;
    bool has_dot = false;

    while (pos < source_len && (isdigit(peek()) || peek() == '.' || peek() == '_')) {
        if (peek() == '_') {
            advance();
            continue;
        }
        else if (peek() == '.') {
            if (has_dot) {
                throw_exception(SUB_LEXER, "Invalid number literal: twice dot", line, file_name);
            }
            else if (pos < source_len && peek(1) == '_') {
                throw_exception(SUB_LEXER, "Invalid number literal: \033[0m'_'\033[31m cannot be immediately after the dot", line, file_name);
            }
            else if (pos < source_len && !isdigit(peek(1))) {
                throw_exception(SUB_LEXER, "Invalid number literal: dot cannot be the end", line, file_name);
            }
            has_dot = true;
        }
        value += advance();
    }

    const char suffix = pos < source_len ? peek() : '\0';
    switch (tolower(suffix)) {
        case 'f':
            advance();
            return Token(TOK_FLOAT_LIT, value, tmp_l, tmp_c, file_name);
        case 's':
            if (has_dot) {
                throw_exception(SUB_LEXER, "Invalid number literal: specified suffix \033[0m's'\033[31m does not match for floating point literal", line, file_name);
            }
            advance();
            return Token(TOK_SHORT_LIT, value, tmp_l, tmp_c, file_name);
        case 'l':
            if (has_dot) {
                throw_exception(SUB_LEXER, "Invalid number literal: specified suffix \033[0m'l'\033[31m does not match for floating point literal", line, file_name);
            }
            advance();
            return Token(TOK_LONG_LIT, value, tmp_l, tmp_c, file_name);
        default:
            if (has_dot) {
                return Token(TOK_DOUBLE_LIT, value, tmp_l, tmp_c, file_name);
            }
            else {
                return Token(TOK_INT_LIT, value, tmp_l, tmp_c, file_name);
            }
    }
}

Token Lexer::tokenize_string_lit() {
    std::string value;
    uint32_t tmp_l = line;
    uint32_t tmp_c = column;

    advance();
    while (pos < source_len && peek() != '\"') {
        char c = advance();
        if (c == '\\') {
            c = get_escape_sequence();
        }
        value += c;
    }
    if (pos == source_len) {
        throw_exception(SUB_LEXER, "Invalid string literal: missed closing double quote", line, file_name);
    }
    advance();

    return Token(TOK_STRING_LIT, value, tmp_l, tmp_c, file_name);
}

Token Lexer::tokenize_character_lit() {
    std::string value;
    uint32_t tmp_l = line;
    uint32_t tmp_c = column;

    advance();
    while (pos < source_len && peek() != '\'') {
        char c = advance();
        if (c == '\\') {
            c = get_escape_sequence();
        }
        value += c;
    }
    if (pos == source_len) {
        throw_exception(SUB_LEXER, "Invalid character literal: missed closing single quote", line, file_name);
    }
    else if (value.length() != 1) {
        throw_exception(SUB_LEXER, "Invalid character literal: length should be equal to 1", line, file_name);
    }
    advance();

    return Token(TOK_CHARACTER_LIT, value, tmp_l, tmp_c, file_name);
}

Token Lexer::tokenize_op() {
    uint32_t tmp_l = line;
    uint32_t tmp_c = column;
    const char c = advance();

    switch (c) {
        case '+':
            if (peek() == '=') {
                advance();
                return Token(TOK_OP_PLUS_EQ, "+=", tmp_l, tmp_c, file_name);
            }
            else if (peek() == '+') {
                advance();
                return Token(TOK_OP_INC, "++", tmp_l, tmp_c, file_name);
            }
            return Token(TOK_OP_PLUS, "+", tmp_l, tmp_c, file_name);
        case '-':
            if (peek() == '=') {
                advance();
                return Token(TOK_OP_MINUS_EQ, "-=", tmp_l, tmp_c, file_name);
            }
            else if (peek() == '-') {
                advance();
                return Token(TOK_OP_DEC, "--", tmp_l, tmp_c, file_name);
            }
            else if (peek() == '>') {
                advance();
                return Token(TOK_OP_NEXT, "->", tmp_l, tmp_c, file_name);
            }
            return Token(TOK_OP_MINUS, "-", tmp_l, tmp_c, file_name);
        case '*':
            if (peek() == '=') {
                advance();
                return Token(TOK_OP_MULT_EQ, "*=", tmp_l, tmp_c, file_name);
            }
            return Token(TOK_OP_MULT, "*", tmp_l, tmp_c, file_name);
        case '/':
            if (peek() == '=') {
                advance();
                return Token(TOK_OP_DIV_EQ, "/=", tmp_l, tmp_c, file_name);
            }
            return Token(TOK_OP_DIV, "/", tmp_l, tmp_c, file_name);
        case '%':
            if (peek() == '=') {
                advance();
                return Token(TOK_OP_MODULO_EQ, "%=", tmp_l, tmp_c, file_name);
            }
            return Token(TOK_OP_MODULO, "%", tmp_l, tmp_c, file_name);
        case '=':
            if (peek() == '=') {
                advance();
                return Token(TOK_OP_EQ_EQ, "==", tmp_l, tmp_c, file_name);
            }
            return Token(TOK_OP_EQ, "=", tmp_l, tmp_c, file_name);
        case '!':
            if (peek() == '=') {
                advance();
                return Token(TOK_OP_NOT_EQ_EQ, "!=", tmp_l, tmp_c, file_name);
            }
            return Token(TOK_OP_L_NOT, "!", tmp_l, tmp_c, file_name);
        case '>':
            if (peek() == '=') {
                advance();
                return Token(TOK_OP_GT_EQ, ">=", tmp_l, tmp_c, file_name);
            }
            return Token(TOK_OP_GT, ">", tmp_l, tmp_c, file_name);
        case '<':
            if (peek() == '=') {
                advance();
                return Token(TOK_OP_LS_EQ, "<=", tmp_l, tmp_c, file_name);
            }
            return Token(TOK_OP_LS, "<", tmp_l, tmp_c, file_name);
        case '&':
            if (peek() == '&') {
                advance();
                return Token(TOK_OP_L_AND, "&&", tmp_l, tmp_c, file_name);
            }
            throw_exception(SUB_LEXER, "Operator '&' (aka bitwise and) is unsupported", line, file_name);
        case '|':
            if (peek() == '|') {
                advance();
                return Token(TOK_OP_L_OR, "||", tmp_l, tmp_c, file_name);
            }
            throw_exception(SUB_LEXER, "Operator '|' (aka bitwise or) is unsupported", line, file_name);
        case ',':
            return Token(TOK_OP_COMMA, ",", tmp_l, tmp_c, file_name);
        case '.':
            return Token(TOK_OP_DOT, ".", tmp_l, tmp_c, file_name);
        case ':':
            return Token(TOK_OP_COLON, ":", tmp_l, tmp_c, file_name);
        case ';':
            return Token(TOK_OP_SEMICOLON, ";", tmp_l, tmp_c, file_name);
        case '(':
            return Token(TOK_OP_LPAREN, "(", tmp_l, tmp_c, file_name);
        case ')':
            return Token(TOK_OP_RPAREN, ")", tmp_l, tmp_c, file_name);
        case '{':
            return Token(TOK_OP_LBRACE, "{", tmp_l, tmp_c, file_name);
        case '}':
            return Token(TOK_OP_RBRACE, "}", tmp_l, tmp_c, file_name);
        case '[':
            return Token(TOK_OP_LBRACKET, "[", tmp_l, tmp_c, file_name);
        case ']':
            return Token(TOK_OP_RBRACKET, "]", tmp_l, tmp_c, file_name);
        case '?':
            return Token(TOK_OP_QUESTION, "?", tmp_l, tmp_c, file_name);
        default:
            std::stringstream ss;
            ss << "Unsupported operator: \033[0m'" << c << "'";
            throw_exception(SUB_LEXER, ss.str(), line, file_name);
    }
}

void Lexer::skip_comments() {
    advance();
    advance();
    while (pos < source_len && peek() != '\n') {
        advance();
    }
}

const char Lexer::get_escape_sequence() {
    const char c = advance();
    switch (c) {
        case 'n':
            return '\n';
        case 't':
            return '\t';
        case 'v':
            return '\v';
        case 'b':
            return '\b';
        case 'r':
            return '\r';
        case 'f':
            return '\f';
        case 'a':
            return '\a';
        case '\\':
            return '\\';
        case '\'':
            return '\'';
        case '"':
            return '\"';
        case '?':
            return '\?';
        default:
            std::stringstream ss;
            ss << "Unsupported escape sequence: \033[0m'\\" << c;
            throw_exception(SUB_LEXER, ss.str(), line, file_name);
    }
}

const char Lexer::peek(int32_t rpos) const {
    if (pos + rpos >= source_len || pos + rpos < 0) {
        std::stringstream ss;
        ss << "Index out of range: " << pos + rpos << '/' << source_len;
        throw_exception(SUB_LEXER, ss.str(), line, file_name);
    }
    return source[pos + rpos];
}

const char Lexer::advance() {
    const char c = peek();
    pos++;
    column++;
    if (c == '\n') {
        line++;
        column = 1;
    }
    return c;
}