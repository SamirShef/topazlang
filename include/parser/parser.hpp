#pragma once
#include "../lexer/lexer.hpp"
#include "ast.hpp"

class Parser {
private:
    std::string source;
    std::vector<Token> tokens;
    size_t tokens_count;
    uint32_t pos;

public:
    Parser(std::string src, std::vector<Token> t) : source(src), tokens(t), tokens_count(t.size()), pos(0) {}

    std::vector<AST::StmtPtr> parse();

private:
    AST::StmtPtr parse_stmt();
    AST::StmtPtr parse_var_decl_stmt();
    
    AST::ExprPtr parse_expr();
    AST::ExprPtr parse_l_and_expr();
    AST::ExprPtr parse_l_or_expr();
    AST::ExprPtr parse_equality_expr();
    AST::ExprPtr parse_comparation_expr();
    AST::ExprPtr parse_additive_expr();
    AST::ExprPtr parse_multiplicative_expr();
    AST::ExprPtr parse_unary_expr();
    AST::ExprPtr parse_primary_expr();

    Token peek(int32_t rpos = 0) const;
    bool match(TokenType type);
    Token consume(TokenType type, std::string err_msg, uint32_t line);
    AST::Type consume_type();
};