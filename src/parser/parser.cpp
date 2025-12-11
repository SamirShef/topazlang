/**
 * @file parser.cpp
 *
 * @brief parser.hpp implementation
 */

#include "../../include/exception/exception.hpp"
#include "../../include/parser/parser.hpp"

std::vector<AST::StmtPtr> Parser::parse() {
    std::vector<AST::StmtPtr> stmts;

    while (pos < tokens_count) {
        stmts.push_back(parse_stmt());
    }

    return stmts;
}

void Parser::reset() {
    pos = 0;
}

AST::AccessModifier current_access;

AST::StmtPtr Parser::parse_stmt(bool from_for) {
    AST::StmtPtr stmt = nullptr;
    current_access = AST::ACCESS_NONE;
    if (match(TOK_PUB)) {
        current_access = AST::ACCESS_PUBLIC;
    }
    else if (match(TOK_PRIV)) {
        current_access = AST::ACCESS_PRIVATE;
    }
    
    if (match(TOK_LET)) {
        stmt = parse_var_decl_stmt();
        if (!from_for) {
            consume_semicolon();
        }
    }
    else if (match(TOK_ID)) {
        if (match(TOK_OP_LPAREN)) {
            stmt = parse_func_call_stmt();
            if (!from_for) {
                consume_semicolon();
            }
            return stmt;
        }
        stmt = parse_var_asgn_stmt();
        if (!from_for) {
            consume_semicolon();
        }
    }
    else if (match(TOK_OP_MULT)) {
        stmt = parse_var_asgn_stmt();
        if (!from_for) {
            consume_semicolon();
        }
    }
    else if (match(TOK_FUN)) {
        stmt = parse_func_decl_stmt();
    }
    else if (match(TOK_RETURN)) {
        stmt = parse_return_stmt();
        if (!from_for) {
            consume_semicolon();
        }
    }
    else if (match(TOK_IF)) {
        stmt = parse_if_else_stmt();
    }
    else if (match(TOK_WHILE)) {
        stmt = parse_while_cycle_stmt();
    }
    else if (match(TOK_DO)) {
        stmt = parse_do_while_cycle_stmt();
        if (!from_for) {
            consume_semicolon();
        }
    }
    else if (match(TOK_FOR)) {
        stmt = parse_for_cycle_stmt();
    }
    else if (match(TOK_BREAK)) {
        stmt = parse_break_stmt();
        if (!from_for) {
            consume_semicolon();
        }
    }
    else if (match(TOK_CONTINUE)) {
        stmt = parse_continue_stmt();
        if (!from_for) {
            consume_semicolon();
        }
    }
    else if (match(TOK_MODULE)) {
        stmt = parse_module_stmt();
    }
    else if (match(TOK_USE)) {
        stmt = parse_use_module_stmt();
        if (!from_for) {
            consume_semicolon();
        }
    }
    else if (match(TOK_UNSAFE)) {
        stmt = parse_unsafe_stmt();
    }
    else if (match(TOK_EXTERN)) {
        stmt = parse_extern_stmt();
    }
    else {
        std::stringstream ss;
        ss << "Expected statement but got \033[0m'" << peek().value << "'\033[31m. Please check statement to mistakes";
        throw_exception(SUB_PARSER, ss.str(), peek().line, peek().file_name, is_debug);
    }
    return stmt;
}

AST::StmtPtr Parser::parse_var_decl_stmt() {
    Token first_token = peek(-1);
    AST::AccessModifier access = current_access;
    AST::Type type = consume_type();
    std::stringstream ss;
    ss << "Expected variable name. Token \033[0m'" << peek().value << "'\033[31m is keyword or operator. Please replase it with unique identifier";
    std::string name = consume(TOK_ID, ss.str(), peek().line).value;
    AST::ExprPtr expr = nullptr;
    if (match(TOK_OP_EQ)) {
        expr = parse_expr();
    }
    return std::make_unique<AST::VarDeclStmt>(access, type, std::move(expr), name, first_token.line);
}

AST::StmtPtr Parser::parse_var_asgn_stmt() {
    Token var_token = peek(-1);
    bool is_deref = false;
    if (peek(-1).type == TOK_OP_MULT) {
        var_token = peek();
        is_deref = true;
        pos++;
    }
    AST::ExprPtr expr = nullptr;
    if (match(TOK_OP_EQ)) {
        expr = parse_expr();
    }
    else if (is_compound_asgn_operator(peek())) {
        expr = create_compound_asgn_operator(var_token.value);
    }
    else {
        expr = create_inc_dec_operator(var_token.value);
    }
    return std::make_unique<AST::VarAsgnStmt>(var_token.value, std::move(expr), is_deref, var_token.line);
}

AST::StmtPtr Parser::parse_func_decl_stmt() {
    Token first_token = peek(-1);
    AST::AccessModifier access = current_access;
    std::stringstream ss;
    ss << "Expected function name. Token \033[0m'" << peek().value << "'\033[31m is keyword or operator. Please replase it with unique identifier";
    std::string name = consume(TOK_ID, ss.str(), peek().line).value;
    std::vector<AST::Argument> args;
    if (match(TOK_OP_LPAREN)) {
        while (!match(TOK_OP_RPAREN)) {
            args.push_back(parse_argument());
            if (peek().type != TOK_OP_RPAREN) {
                ss.str("");
                ss << "Expected \033[0m','\033[31m between function arguments.\nPlease replace \033[0m'";
                ss << args[args.size() - 1].name << ": " << args[args.size() - 1].type.to_str() << " " << peek().value << "'\033[31m with: \033[0m'"
                   << args[args.size() - 1].name << ": " << args[args.size() - 1].type.to_str() << ", " << peek().value << "'";
                consume(TOK_OP_COMMA, ss.str(), peek().line);
            }
        }
    }

    AST::Type ret_type = AST::Type(AST::TYPE_NOTH, "noth");
    if (match(TOK_OP_NEXT)) {
        ret_type = consume_type();
    }
    
    std::vector<AST::StmtPtr> block;
    consume(TOK_OP_LBRACE, "Expected \033[0m'{'\033[31m after funtion arguments. Prototypes of functions is unsupported in current Topaz compiler version", peek().line);
    while (!match(TOK_OP_RBRACE)) {
        block.push_back(parse_stmt());
    }
    return std::make_unique<AST::FuncDeclStmt>(access, name, std::move(args), ret_type, std::move(block), first_token.line);
}

AST::StmtPtr Parser::parse_func_call_stmt() {
    Token name_token = peek(-2);
    std::vector<AST::ExprPtr> args;
    while (!match(TOK_OP_RPAREN)) {
        args.push_back(parse_expr());
        if (peek().type != TOK_OP_RPAREN) {
            std::stringstream ss;
            ss << "Expected \033[0m','\033[31m between function arguments.\nPlease replace \033[0m'";
            ss << peek(-1).value << " " << peek().value << "'\033[31m with: \033[0m'"
               << peek(-1).value << ", " << peek().value << "'";
            consume(TOK_OP_COMMA, ss.str(), peek().line);
        }
    }
    return std::make_unique<AST::FuncCallStmt>(name_token.value, std::move(args), name_token.line);
}

AST::Argument Parser::parse_argument() {
    std::stringstream ss;
    ss << "Expected function argument name. Token \033[0m'" << peek().value << "'\033[31m is keyword or operator. Please replase it with unique identifier";
    std::string name = consume(TOK_ID, ss.str(), peek().line).value;

    ss.str("");
    ss << "Expected \033[0m':'\033[31m between function argument name and type.\nPlease replace \033[0m'";
    ss << name << "'\033[31m with: \033[0m'" << name << ": '";
    consume(TOK_OP_COLON, ss.str(), peek().line);

    AST::Type type = consume_type();
    return AST::Argument(name, type);
}

AST::StmtPtr Parser::parse_return_stmt() {
    Token first_token = peek(-1);
    AST::ExprPtr ret_expr = nullptr;
    if (peek().type != TOK_OP_SEMICOLON) {
        ret_expr = parse_expr();
    }
    return std::make_unique<AST::ReturnStmt>(std::move(ret_expr), first_token.line);
}

AST::StmtPtr Parser::parse_if_else_stmt() {
    Token first_token = peek(-1);
    AST::ExprPtr cond = parse_expr();
    std::vector<AST::StmtPtr> then_block;
    consume(TOK_OP_LBRACE, "Expected \033[0m'{'\033[31m after condition", peek().line);
    while (!match(TOK_OP_RBRACE)) {
        then_block.push_back(parse_stmt());
    }
    std::vector<AST::StmtPtr> else_block;
    if (match(TOK_ELSE)) {
        if (match(TOK_OP_LBRACE)) {
            while (!match(TOK_OP_RBRACE)) {
                else_block.push_back(parse_stmt());
            }
        }
        else {
            else_block.push_back(parse_stmt());
        }
    }
    return std::make_unique<AST::IfElseStmt>(std::move(cond), std::move(then_block), std::move(else_block), first_token.line);
}

AST::StmtPtr Parser::parse_while_cycle_stmt() {
    Token first_token = peek(-1);
    AST::ExprPtr cond = parse_expr();
    std::vector<AST::StmtPtr> block;
    consume(TOK_OP_LBRACE, "Expected \033[0m'{'\033[31m after condition", peek().line);
    while (!match(TOK_OP_RBRACE)) {
        block.push_back(parse_stmt());
    }
    return std::make_unique<AST::WhileCycleStmt>(std::move(cond), std::move(block), first_token.line);
}

AST::StmtPtr Parser::parse_do_while_cycle_stmt() {
    Token first_token = peek(-1);
    std::vector<AST::StmtPtr> block;
    consume(TOK_OP_LBRACE, "Expected \033[0m'{'\033[31m after condition", peek().line);
    while (!match(TOK_OP_RBRACE)) {
        block.push_back(parse_stmt());
    }
    consume(TOK_WHILE, "Expected \033[0m'while'\033[31m after block in the do-while cycle", peek().line);
    AST::ExprPtr cond = parse_expr();
    return std::make_unique<AST::DoWhileCycleStmt>(std::move(cond), std::move(block), first_token.line);
}

AST::StmtPtr Parser::parse_for_cycle_stmt() {
    Token first_token = peek(-1);
    AST::StmtPtr indexator = parse_stmt(true);
    consume(TOK_OP_COMMA, "Expected \033[0m','\033[31m after indexator declaration", peek().line);
    AST::ExprPtr cond = parse_expr();
    consume(TOK_OP_COMMA, "Expected \033[0m','\033[31m after expression", peek().line);
    AST::StmtPtr iteration = parse_stmt(true);
    std::vector<AST::StmtPtr> block;
    consume(TOK_OP_LBRACE, "Expected \033[0m'{'\033[31m", peek().line);
    while (!match(TOK_OP_RBRACE)) {
        block.push_back(parse_stmt());
    }

    return std::make_unique<AST::ForCycleStmt>(std::move(indexator), std::move(cond), std::move(iteration), std::move(block), first_token.line);
}

AST::StmtPtr Parser::parse_break_stmt() {
    Token first_token = peek(-1);
    return std::make_unique<AST::BreakStmt>(first_token.line);
}

AST::StmtPtr Parser::parse_continue_stmt() {
    Token first_token = peek(-1);
    return std::make_unique<AST::ContinueStmt>(first_token.line);
}

AST::StmtPtr Parser::parse_module_stmt() {
    Token first_token = peek(-1);
    AST::AccessModifier access = current_access;
    std::stringstream ss;
    ss << "Expected module name. Token \033[0m'" << peek().value << "'\033[31m is keyword or operator. Please replase it with unique identifier";
    std::string name = consume(TOK_ID, ss.str(), peek().line).value;
    std::vector<AST::StmtPtr> block;
    consume(TOK_OP_LBRACE, "Expected \033[0m'{'\033[31m", peek().line);
    while (!match(TOK_OP_RBRACE)) {
        block.push_back(parse_stmt());
    }
    return std::make_unique<AST::ModuleStmt>(access, first_token.file_name, name, std::move(block), first_token.line);
}

AST::StmtPtr Parser::parse_use_module_stmt() {
    Token first_token = peek(-1);
    std::vector<std::string> path;
    do {
        std::stringstream ss;
        ss << "Expected module name. Token \033[0m'" << peek().value << "'\033[31m is keyword or operator. Please replase it with unique identifier";
        path.push_back(consume(TOK_ID, ss.str(), peek().line).value);
    } while (match(TOK_OP_DOT));
    return std::make_unique<AST::UseModuleStmt>(std::move(path), first_token.line);
}

AST::StmtPtr Parser::parse_unsafe_stmt() {
    Token first_token = peek(-1);
    std::vector<AST::StmtPtr> block;
    consume(TOK_OP_LBRACE, "Expected \033[0m'{'\033[31m", peek().line);
    while (!match(TOK_OP_RBRACE)) {
        block.push_back(parse_stmt());
    }
    return std::make_unique<AST::UnsafeStmt>(std::move(block), first_token.line);
}

AST::StmtPtr Parser::parse_func_decl_proto_stmt() {
    Token first_token = peek();
    std::stringstream ss;
    ss << "Expected function name. Token \033[0m'" << peek().value << "'\033[31m is keyword or operator. Please replase it with unique identifier";
    std::string name = consume(TOK_ID, ss.str(), peek().line).value;
    std::vector<AST::Argument> args;
    if (match(TOK_OP_LPAREN)) {
        while (!match(TOK_OP_RPAREN)) {
            args.push_back(parse_argument());
            if (peek().type != TOK_OP_RPAREN) {
                ss.str("");
                ss << "Expected \033[0m','\033[31m between function arguments.\nPlease replace \033[0m'";
                ss << args[args.size() - 1].name << ": " << args[args.size() - 1].type.to_str() << " " << peek().value << "'\033[31m with: \033[0m'"
                   << args[args.size() - 1].name << ": " << args[args.size() - 1].type.to_str() << ", " << peek().value << "'";
                consume(TOK_OP_COMMA, ss.str(), peek().line);
            }
        }
    }

    AST::Type ret_type = AST::Type(AST::TYPE_NOTH, "noth");
    if (match(TOK_OP_NEXT)) {
        ret_type = consume_type();
    }
    consume_semicolon();
    return std::make_unique<AST::FuncDeclStmt>(AST::ACCESS_NONE, name, std::move(args), ret_type, std::vector<AST::StmtPtr>{}, first_token.line);
}

AST::StmtPtr Parser::parse_extern_stmt() {
    Token first_token = peek(-1);
    std::string lang_name_lit = consume(TOK_STRING_LIT, "Expected string literal (language name)", peek().line).value;
    std::vector<AST::StmtPtr> block;
    consume(TOK_OP_LBRACE, "Expected \033[0m'{'\033[31m", peek().line);
    while (!match(TOK_OP_RBRACE)) {
        block.push_back(parse_func_decl_proto_stmt());
    }
    return std::make_unique<AST::ExternStmt>(lang_name_lit, std::move(block), first_token.line);
}

AST::ExprPtr Parser::parse_expr() {
    return parse_l_and_expr();
}

AST::ExprPtr Parser::parse_l_and_expr() {
    AST::ExprPtr expr = parse_l_or_expr();
    while (match(TOK_OP_L_AND)) {
        Token token = peek(-1);
        expr = std::make_unique<AST::BinaryExpr>(token, std::move(expr), parse_l_or_expr(), token.line);
    }
    return expr;
}

AST::ExprPtr Parser::parse_l_or_expr() {
    AST::ExprPtr expr = parse_equality_expr();
    while (match(TOK_OP_L_OR)) {
        Token token = peek(-1);
        expr = std::make_unique<AST::BinaryExpr>(token, std::move(expr), parse_equality_expr(), token.line);
    }
    return expr;
}

AST::ExprPtr Parser::parse_equality_expr() {
    AST::ExprPtr expr = parse_comparation_expr();
    while (1) {
        Token token = peek();
        if (match(TOK_OP_EQ_EQ)) {
            expr = std::make_unique<AST::BinaryExpr>(token, std::move(expr), parse_comparation_expr(), token.line);
        }
        else if (match(TOK_OP_NOT_EQ_EQ)) {
            expr = std::make_unique<AST::BinaryExpr>(token, std::move(expr), parse_comparation_expr(), token.line);
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
        Token token = peek();
        if (match(TOK_OP_GT)) {
            expr = std::make_unique<AST::BinaryExpr>(token, std::move(expr), parse_additive_expr(), token.line);
        }
        else if (match(TOK_OP_GT_EQ)) {
            expr = std::make_unique<AST::BinaryExpr>(token, std::move(expr), parse_additive_expr(), token.line);
        }
        else if (match(TOK_OP_LS)) {
            expr = std::make_unique<AST::BinaryExpr>(token, std::move(expr), parse_additive_expr(), token.line);
        }
        else if (match(TOK_OP_LS_EQ)) {
            expr = std::make_unique<AST::BinaryExpr>(token, std::move(expr), parse_additive_expr(), token.line);
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
        Token token = peek();
        if (match(TOK_OP_PLUS)) {
            expr = std::make_unique<AST::BinaryExpr>(token, std::move(expr), parse_multiplicative_expr(), token.line);
        }
        else if (match(TOK_OP_MINUS)) {
            expr = std::make_unique<AST::BinaryExpr>(token, std::move(expr), parse_multiplicative_expr(), token.line);
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
        Token token = peek();
        if (match(TOK_OP_MULT)) {
            expr = std::make_unique<AST::BinaryExpr>(token, std::move(expr), parse_unary_expr(), token.line);
        }
        else if (match(TOK_OP_DIV)) {
            expr = std::make_unique<AST::BinaryExpr>(token, std::move(expr), parse_unary_expr(), token.line);
        }
        else if (match(TOK_OP_MODULO)) {
            expr = std::make_unique<AST::BinaryExpr>(token, std::move(expr), parse_unary_expr(), token.line);
        }
        else {
            break;
        }
    }
    return expr;
}

AST::ExprPtr Parser::parse_unary_expr() {
    Token token = peek();
    while (1) {
        if (match(TOK_OP_MINUS)) {
            return std::make_unique<AST::UnaryExpr>(token, parse_primary_expr(), token.line);
        }
        else if (match(TOK_OP_L_NOT)) {
            return std::make_unique<AST::UnaryExpr>(token, parse_primary_expr(), token.line);
        }
        else if (match(TOK_OP_MULT)) {
            return std::make_unique<AST::UnaryExpr>(token, parse_unary_expr(), token.line);
        }
        else if (match(TOK_OP_REF)) {
            return std::make_unique<AST::UnaryExpr>(token, parse_unary_expr(), token.line);
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
            pos++;
            AST::ExprPtr expr = parse_expr();
            consume(TOK_OP_RPAREN, "Expected ')'. You forgot to specify the closing ')'", token.line);
            return expr;
        }
        case TOK_ID:
            pos++;
            if (match(TOK_OP_LPAREN)) {
                std::vector<AST::ExprPtr> args;
                uint32_t c_pos = pos;
                while (!match(TOK_OP_RPAREN)) {
                    args.push_back(parse_expr());
                    if (peek().type != TOK_OP_RPAREN) {
                        std::stringstream ss;
                        ss << "Expected \033[0m','\033[31m between function arguments.\nPlease replace \033[0m'";
                        ss << peek(-1).value << " " << peek().value << "'\033[31m with: \033[0m'"
                           << peek(-1).value << ", " << peek().value << "'";
                        consume(TOK_OP_COMMA, ss.str(), peek().line);
                    }
                }
                if (match(TOK_OP_DOT)) {
                    pos = c_pos - 2;                            // return to the identifier
                    return parse_obj_chain_expr();
                }
                return std::make_unique<AST::FuncCallExpr>(token.value, std::move(args), token.line);
            }
            else if (peek().type == TOK_OP_INC || peek().type == TOK_OP_DEC) {
                return create_inc_dec_operator(token.value);
            }
            else if (match(TOK_OP_DOT)) {
                pos -= 2;                                       // return to the identifier
                return parse_obj_chain_expr();
            }
            return std::make_unique<AST::VarExpr>(token.value, token.line);
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
            std::stringstream ss;
            ss << "Expected expression, but got \033[0m'" << peek().value << "'\033[31m. Please check expression to mistakes";
            throw_exception(SUB_PARSER, ss.str(), token.line, token.file_name, is_debug);
    }
}

AST::ExprPtr Parser::parse_obj_chain_expr() {
    Token first_token = peek();
    std::vector<AST::ExprPtr> chain;
    do {
        std::stringstream ss;
        ss << "Expected object name. Token \033[0m'" << peek().value << "'\033[31m is keyword or operator. Please replase it with unique identifier";
        Token token = consume(TOK_ID, ss.str(), peek().line);
        if (match(TOK_OP_LPAREN)) {
            std::vector<AST::ExprPtr> args;
            while (!match(TOK_OP_RPAREN)) {
                args.push_back(parse_expr());
                if (peek().type != TOK_OP_RPAREN) {
                    std::stringstream ss;
                    ss << "Expected \033[0m','\033[31m between function arguments.\nPlease replace \033[0m'";
                    ss << peek(-1).value << " " << peek().value << "'\033[31m with: \033[0m'"
                       << peek(-1).value << ", " << peek().value << "'";
                    consume(TOK_OP_COMMA, ss.str(), peek().line);
                }
            }
            chain.push_back(std::make_unique<AST::FuncCallExpr>(token.value, std::move(args), token.line));
        }
        else {
            chain.push_back(std::make_unique<AST::VarExpr>(token.value, token.line));
        }
    } while (match(TOK_OP_DOT));
    return std::make_unique<AST::ChainObjects>(std::move(chain), first_token.line);
}

Token Parser::peek(int32_t rpos) const {
    if (pos + rpos >= tokens_count || pos + rpos < 0) {
        std::stringstream ss;
        ss << "Index out of range: " << pos + rpos << '/' << tokens_count;
        throw_exception(SUB_PARSER, ss.str(), peek().line, peek().file_name, is_debug);
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
    throw_exception(SUB_PARSER, err_msg, line, token.file_name, is_debug);
}

AST::Type Parser::consume_type() {
    Token token = peek();
    bool is_const = false;
    bool is_ptr = false;
    bool is_nullable = false;
    if (match(TOK_CONST)) {
        is_const = true;
    }
    switch (peek().type) {
        case TOK_BOOL:
        case TOK_CHAR:
        case TOK_SHORT:
        case TOK_INT:
        case TOK_LONG:
        case TOK_FLOAT:
        case TOK_DOUBLE:
        case TOK_NOTH: {
            Token type = peek();
            pos++;
            if (match(TOK_OP_MULT)) {
                is_ptr = true;
            }
            if (match(TOK_OP_QUESTION)) {
                is_nullable = true;
            }
            return AST::Type(ttype_to_tvalue(type.type), type.value, is_const, is_ptr, is_nullable);
        }
        default: {
            std::stringstream ss;
            ss << "Token \033[0m'" << peek().value << "'\033[31m is not type. Please replase it to exists type";
            throw_exception(SUB_PARSER, ss.str(), peek().line, peek().file_name, is_debug);
        }
    }
}

void Parser::consume_semicolon() {
    std::stringstream ss;
    ss << "Expected \033[0m';'\033[31m in the end of variable definition. ";
    if (pos == tokens_count) {
        ss << "Please add \033[0m';'\033[31m into the end of variable definition";
    }
    else {
        ss << "Please replace \033[0m'" << peek().value << "'\033[31m with \033[0m';'";
    }
    consume(TOK_OP_SEMICOLON, ss.str(), peek().line);
}

AST::TypeValue Parser::ttype_to_tvalue(TokenType type) {
    switch (type) {
        case TOK_BOOL:
            return AST::TYPE_BOOL;
        case TOK_CHAR:
            return AST::TYPE_CHAR;
        case TOK_SHORT:
            return AST::TYPE_SHORT;
        case TOK_INT:
            return AST::TYPE_INT;
        case TOK_LONG:
            return AST::TYPE_LONG;
        case TOK_FLOAT:
            return AST::TYPE_FLOAT;
        case TOK_DOUBLE:
            return AST::TYPE_DOUBLE;
        case TOK_NOTH:
            return AST::TYPE_NOTH;
        default:
            std::stringstream ss;
            ss << "Token \033[0m'" << peek().value << "'\033[31m is not type. Please replase it to exists types";
            throw_exception(SUB_PARSER, ss.str(), peek().line, peek().file_name, is_debug);
    }
}

bool Parser::is_compound_asgn_operator(Token token) {
    switch (token.type) {
        case TOK_OP_PLUS_EQ:
        case TOK_OP_MINUS_EQ:
        case TOK_OP_MULT_EQ:
        case TOK_OP_DIV_EQ:
        case TOK_OP_MODULO_EQ:
            return true;
        default:
            return false;
    }
}

AST::ExprPtr Parser::create_compound_asgn_operator(std::string var_name) {
    Token token = peek();
    pos++;
    switch (token.type) {
        case TOK_OP_PLUS_EQ:
            return std::make_unique<AST::BinaryExpr>(Token(TOK_OP_PLUS, "+", token.line, token.column, token.file_name), std::make_unique<AST::VarExpr>(var_name, token.line), parse_expr(), token.line);
        case TOK_OP_MINUS_EQ:
            return std::make_unique<AST::BinaryExpr>(Token(TOK_OP_MINUS, "-", token.line, token.column, token.file_name), std::make_unique<AST::VarExpr>(var_name, token.line), parse_expr(), token.line);
        case TOK_OP_MULT_EQ:
            return std::make_unique<AST::BinaryExpr>(Token(TOK_OP_MULT, "*", token.line, token.column, token.file_name), std::make_unique<AST::VarExpr>(var_name, token.line), parse_expr(), token.line);
        case TOK_OP_DIV_EQ:
            return std::make_unique<AST::BinaryExpr>(Token(TOK_OP_DIV, "/", token.line, token.column, token.file_name), std::make_unique<AST::VarExpr>(var_name, token.line), parse_expr(), token.line);
        case TOK_OP_MODULO_EQ:
            return std::make_unique<AST::BinaryExpr>(Token(TOK_OP_MODULO, "%", token.line, token.column, token.file_name), std::make_unique<AST::VarExpr>(var_name, token.line), parse_expr(), token.line);
        default: {
            std::stringstream ss;
            ss << "Unsupported compound assignment operator: \033[0m'" << token.value << "'\033[31m. Please check your Topaz compiler version and fix the problematic section of the code";
            throw_exception(SUB_PARSER, ss.str(), token.line, peek().file_name, is_debug);
        }
    }
}

AST::ExprPtr Parser::create_inc_dec_operator(std::string var_name) {
    Token token = peek();
    pos++;
    switch (token.type) {
        case TOK_OP_INC:
            return std::make_unique<AST::BinaryExpr>(Token(TOK_OP_PLUS, "+", token.line, token.column, token.file_name), std::make_unique<AST::VarExpr>(var_name, token.line), std::make_unique<AST::IntLiteral>(1, token.line), token.line);
        case TOK_OP_DEC:
            return std::make_unique<AST::BinaryExpr>(Token(TOK_OP_MINUS, "-", token.line, token.column, token.file_name), std::make_unique<AST::VarExpr>(var_name, token.line), std::make_unique<AST::IntLiteral>(1, token.line), token.line);
        default: {
            std::stringstream ss;
            ss << "Unsupported increment/decrement operator: \033[0m'" << token.value << "'\033[31m. Please check your Topaz compiler version and fix the problematic section of the code";
            throw_exception(SUB_PARSER, ss.str(), token.line, peek().file_name, is_debug);
        }
    }
}