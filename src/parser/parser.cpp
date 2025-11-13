#include "../../include/exception/exception.hpp"
#include "../../include/parser/parser.hpp"
#include <sstream>
#include <memory>

std::vector<AST::StmtPtr> Parser::parse() {
    std::vector<AST::StmtPtr> stmts;

    while (pos < tokens_count) {
        stmts.push_back(parse_stmt());
    }

    return stmts;
}

AST::StmtPtr Parser::parse_stmt() {
    if (match(TOK_LET)) {
        return parse_var_decl_stmt();
    }
    else {
        throw_excpetion(SUB_PARSER, "Unsupported statement", peek().line, peek().file_name);
    }
}

AST::StmtPtr Parser::parse_var_decl_stmt() {
    Token first_token = peek(-1);
    AST::Type type = consume_type();
    std::stringstream ss;
    ss << "Expected \033[0m':'\033[31m between type and variable name.\nPlease replace \033[0m'";
    ss << "let " << type.to_str() << "'\033[31m with: \033[0m'let " << type.to_str() << ": '";
    consume(TOK_OP_COLON, ss.str(), peek().line);

    ss.str("");
    ss << "Expected \033[0m':'\033[31m between type and variable name.\nSymbol '\033[0m" << peek().value << "'\033[31m is keyword. Please replase it with unique identifier";
    std::string name = consume(TOK_ID, ss.str(), peek().line).value;
    AST::ExprPtr expr = nullptr;
    if (pos == tokens_count) {
        ss.str("");
        ss << "Expected \033[0m';'\033[31m in the end of variable definition. Please add \033[0m';'\033[31m into the end of variable definition";
        throw_excpetion(SUB_PARSER, ss.str(), peek(-1).line, peek(-1).file_name);
    }
    if (match(TOK_OP_EQ)) {
        expr = parse_expr();
    }
    
    ss.str("");
    ss << "Expected \033[0m';'\033[31m in the end of variable definition. ";
    if (pos == tokens_count) {
        ss << "Please add \033[0m';'\033[31m into the end of variable definition";
    }
    else {
        ss << "Please replace \033[0m'" << peek().value << "'\033[31m with \033[0m';'";
    }
    consume(TOK_OP_SEMICOLON, ss.str(), peek().line);

    return std::make_unique<AST::VarDeclStmt>(type, std::move(expr), name, first_token.line);
}

AST::ExprPtr Parser::parse_expr() {
    return parse_l_and_expr();
}

AST::ExprPtr Parser::parse_l_and_expr() {
    AST::ExprPtr expr = parse_l_or_expr();
    while (match(TOK_OP_L_AND)) {
        uint32_t line = peek().line;
        expr = std::make_unique<AST::BinaryExpr>(TOK_OP_L_AND, std::move(expr), parse_l_or_expr(), line);
    }
    return expr;
}

AST::ExprPtr Parser::parse_l_or_expr() {
    AST::ExprPtr expr = parse_equality_expr();
    while (match(TOK_OP_L_OR)) {
        uint32_t line = peek().line;
        expr = std::make_unique<AST::BinaryExpr>(TOK_OP_L_OR, std::move(expr), parse_equality_expr(), line);
    }
    return expr;
}

AST::ExprPtr Parser::parse_equality_expr() {
    AST::ExprPtr expr = parse_comparation_expr();
    while (1) {
        uint32_t line = peek().line;
        if (match(TOK_OP_EQ_EQ)) {
            expr = std::make_unique<AST::BinaryExpr>(TOK_OP_EQ_EQ, std::move(expr), parse_comparation_expr(), line);
        }
        else if (match(TOK_OP_NOT_EQ_EQ)) {
            expr = std::make_unique<AST::BinaryExpr>(TOK_OP_NOT_EQ_EQ, std::move(expr), parse_comparation_expr(), line);
        }
        else {
            break;
        }
    }
    return expr;
}

AST::ExprPtr Parser::parse_comparation_expr() {
    AST::ExprPtr expr = parse_additive_expr();
    while (1) {
        uint32_t line = peek().line;
        if (match(TOK_OP_EQ_EQ)) {
            expr = std::make_unique<AST::BinaryExpr>(TOK_OP_EQ_EQ, std::move(expr), parse_additive_expr(), line);
        }
        else if (match(TOK_OP_NOT_EQ_EQ)) {
            expr = std::make_unique<AST::BinaryExpr>(TOK_OP_NOT_EQ_EQ, std::move(expr), parse_additive_expr(), line);
        }
        else {
            break;
        }
    }
    return expr;
}

AST::ExprPtr Parser::parse_additive_expr() {
    AST::ExprPtr expr = parse_multiplicative_expr();
    while (1) {
        uint32_t line = peek().line;
        if (match(TOK_OP_PLUS)) {
            expr = std::make_unique<AST::BinaryExpr>(TOK_OP_PLUS, std::move(expr), parse_multiplicative_expr(), line);
        }
        else if (match(TOK_OP_MINUS)) {
            expr = std::make_unique<AST::BinaryExpr>(TOK_OP_MINUS, std::move(expr), parse_multiplicative_expr(), line);
        }
        else {
            break;
        }
    }
    return expr;
}

AST::ExprPtr Parser::parse_multiplicative_expr() {
    AST::ExprPtr expr = parse_unary_expr();
    while (1) {
        uint32_t line = peek().line;
        if (match(TOK_OP_MULT)) {
            expr = std::make_unique<AST::BinaryExpr>(TOK_OP_MULT, std::move(expr), parse_unary_expr(), line);
        }
        else if (match(TOK_OP_DIV)) {
            expr = std::make_unique<AST::BinaryExpr>(TOK_OP_DIV, std::move(expr), parse_unary_expr(), line);
        }
        else if (match(TOK_OP_MODULO)) {
            expr = std::make_unique<AST::BinaryExpr>(TOK_OP_MODULO, std::move(expr), parse_unary_expr(), line);
        }
        else {
            break;
        }
    }
    return expr;
}

AST::ExprPtr Parser::parse_unary_expr() {
    uint32_t line = peek().line;
    while (1) {
        if (match(TOK_OP_MINUS)) {
            return std::make_unique<AST::UnaryExpr>(TOK_OP_PLUS, parse_primary_expr(), line);
        }
        else if (match(TOK_OP_L_NOT)) {
            return std::make_unique<AST::UnaryExpr>(TOK_OP_MINUS, parse_primary_expr(), line);
        }
        else {
            break;
        }
    }
    return parse_primary_expr();
}

AST::ExprPtr Parser::parse_primary_expr() {
    Token token = peek();
    switch (token.type) {
        case TOK_OP_LPAREN: {
            AST::ExprPtr expr = parse_expr();
            consume(TOK_OP_RPAREN, "Expected ')'. You forgot to specify the closing ')'", token.line);
            return expr;
        }
        case TOK_CHARACTER_LIT:
            pos++;
            return std::make_unique<AST::CharacterLiteral>(token.value[0], token.line);
        case TOK_SHORT_LIT:
            pos++;
            return std::make_unique<AST::ShortLiteral>(std::stoll(token.value), token.line);
        case TOK_INT_LIT:
            pos++;
            return std::make_unique<AST::IntLiteral>(std::stoll(token.value), token.line);
        case TOK_LONG_LIT:
            pos++;
            return std::make_unique<AST::LongLiteral>(std::stoll(token.value), token.line);
        case TOK_FLOAT_LIT:
            pos++;
            return std::make_unique<AST::FloatLiteral>(std::stold(token.value), token.line);
        case TOK_DOUBLE_LIT:
            pos++;
            return std::make_unique<AST::DoubleLiteral>(std::stold(token.value), token.line);
        case TOK_BOOLEAN_LIT:
            pos++;
            return std::make_unique<AST::BoolLiteral>(token.value == "true", token.line);
        case TOK_STRING_LIT:
            pos++;
            return std::make_unique<AST::StringLiteral>(token.value, token.line);
        default:
            throw_excpetion(SUB_PARSER, "Unsupported expression. Please check expression to mistakes", token.line, token.file_name);
    }
}

Token Parser::peek(int32_t rpos) const {
    if (pos + rpos >= tokens_count || pos + rpos < 0) {
        std::stringstream ss;
        ss << "Index out of range: " << pos + rpos << '/' << tokens_count;
        throw_excpetion(SUB_PARSER, ss.str(), peek().line, peek().file_name);
    }
    return tokens[pos + rpos];
}

bool Parser::match(TokenType type) {
    if (peek().type == type) {
        pos++;
        return true;
    }
    return false;
}

Token Parser::consume(TokenType type, std::string err_msg, uint32_t line) {
    Token token = peek();
    if (match(type)) {
        return token;
    }
    throw_excpetion(SUB_PARSER, err_msg, line, token.file_name);
}

AST::Type Parser::consume_type() {
    Token token = peek();
    pos++;
    return AST::Type(AST::TYPE_INT, token.value);
}