/**
 * @file semantic.cpp
 *
 * @brief semantic.hpp implementation
 */

#include "../../include/exception/exception.hpp"
#include "../../include/semantic/semantic.hpp"
#include <algorithm>
#include <cstddef>
#include <sstream>
#include <memory>
#include <utility>

void SemanticAnalyzer::analyze() {
    for (const AST::StmtPtr& stmt : stmts) {
        analyze_stmt(*stmt);
    }
}

void SemanticAnalyzer::analyze_stmt(AST::Stmt& stmt) {
    if (auto vds = dynamic_cast<AST::VarDeclStmt*>(&stmt)) {
        analyze_var_decl_stmt(*vds);
    }
    else if (auto vas = dynamic_cast<AST::VarAsgnStmt*>(&stmt)) {
        analyze_var_asgn_stmt(*vas);
    }
    else if (auto fds = dynamic_cast<AST::FuncDeclStmt*>(&stmt)) {
        analyze_func_decl_stmt(*fds);
    }
    else if (auto fcs = dynamic_cast<AST::FuncCallStmt*>(&stmt)) {
        analyze_func_call_stmt(*fcs);
    }
    else if (auto rs = dynamic_cast<AST::ReturnStmt*>(&stmt)) {
        analyze_return_stmt(*rs);
    }
    else {
        throw_exception(SUB_SEMANTIC, "Unsupported statement. Please check your Topaz compiler version and fix the problematic section of the code", stmt.line, file_name);
    }
}

void SemanticAnalyzer::analyze_var_decl_stmt(AST::VarDeclStmt& vds) {
    std::unique_ptr<Value> value = get_variable_value(vds.name, vds.line);
    if (value != nullptr) {
        std::stringstream ss;
        ss << "Variable \033[0m'" << vds.name << "'\033[31m already exists";
        throw_exception(SUB_SEMANTIC, ss.str(), vds.line, file_name);
    }
    AST::Type var_type = vds.type;
    Value var_val = Value(var_type, get_default_val_by_type(var_type, vds.line));
    if (vds.expr != nullptr) {
        var_val = analyze_expr(*vds.expr);
    }
    if (!has_common_type(var_val.type, var_type)) {
        std::stringstream ss;
        ss << "Type mismatch: an expression of the type \033[0m'" << var_val.type.to_str() << "'\033[31m, but the type is expected \033[0m'" << var_type.to_str() << "'\033[31m";
        throw_exception(SUB_SEMANTIC, ss.str(), vds.line, file_name);
    }
    variables.top().emplace(vds.name, var_val);
}

void SemanticAnalyzer::analyze_var_asgn_stmt(AST::VarAsgnStmt& vas) {
    std::unique_ptr<Value> var_val = get_variable_value(vas.name, vas.line);
    if (var_val == nullptr) {
        std::stringstream ss;
        ss << "Variable \033[0m'" << vas.name << "'\033[31m does not exists";
        throw_exception(SUB_SEMANTIC, ss.str(), vas.line, file_name);
    }
    AST::Type var_type = var_val->type;
    Value new_val = analyze_expr(*vas.expr);
    if (!has_common_type(new_val.type, var_type)) {
        std::stringstream ss;
        ss << "Type mismatch: an expression of the type \033[0m'" << new_val.type.to_str() << "'\033[31m, but the type is expected \033[0m'" << var_type.to_str() << "'\033[31m";
        throw_exception(SUB_SEMANTIC, ss.str(), vas.line, file_name);
    }
}

void SemanticAnalyzer::analyze_func_decl_stmt(AST::FuncDeclStmt& fds) {
    FunctionInfo *func = get_function_info(fds.name, fds.line);
    if (func != nullptr) {
        std::stringstream ss;
        ss << "Function \033[0m'" << fds.name << "'\033[31m does not exists";
        throw_exception(SUB_SEMANTIC, ss.str(), fds.line, file_name);
    }
    AST::Type ret_type = fds.ret_type;
    functions.emplace(fds.name, new FunctionInfo{.ret_type=ret_type, .args=std::move(fds.args), .block=std::move(fds.block)});
    functions_ret_types.push(ret_type);
    for (auto& arg : functions.at(fds.name)->args) {
        analyze_var_decl_stmt(*std::make_unique<AST::VarDeclStmt>(arg.type, nullptr, arg.name, fds.line));
    }
    for (auto& stmt : functions.at(fds.name)->block) {
        analyze_stmt(*stmt);
    }
    functions_ret_types.pop();
}

void SemanticAnalyzer::analyze_func_call_stmt(AST::FuncCallStmt& fcs) {
    FunctionInfo *func = get_function_info(fcs.name, fcs.line);
    if (func == nullptr) {
        std::stringstream ss;
        ss << "Function \033[0m'" << fcs.name << "'\033[31m does not exists";
        throw_exception(SUB_SEMANTIC, ss.str(), fcs.line, file_name);
    }
    if (fcs.args.size() != functions.at(fcs.name)->args.size()) {
        std::stringstream ss;
        ss << "Function \033[0m'" << fcs.name << "'\033[31m expected " << functions.at(fcs.name)->args.size() << " arguments, but got " << fcs.args.size();
        throw_exception(SUB_SEMANTIC, ss.str(), fcs.line, file_name);
    }
    size_t index = 0;
    for (auto& arg : fcs.args) {
        AST::Type arg_type = analyze_expr(*arg).type;
        if (!has_common_type(arg_type, functions.at(fcs.name)->args[index].type)) {
            std::stringstream ss;
            ss << "Type mismatch: an expression of the type \033[0m'" << arg_type.to_str() << "'\033[31m, but the type is expected \033[0m'" << functions.at(fcs.name)->args[index].type.to_str() << "'\033[31m";
            throw_exception(SUB_SEMANTIC, ss.str(), fcs.line, file_name);
        }
        index++;
    }
}

void SemanticAnalyzer::analyze_return_stmt(AST::ReturnStmt& rs) {
    if (rs.expr != nullptr) {
        Value val = analyze_expr(*rs.expr);
        if (!has_common_type(val.type, functions_ret_types.top())) {
            std::stringstream ss;
            ss << "Type mismatch: an expression of the type \033[0m'" << val.type.to_str() << "'\033[31m, but the type is expected \033[0m'" << functions_ret_types.top().to_str() << "'\033[31m";
            throw_exception(SUB_SEMANTIC, ss.str(), rs.line, file_name);
        }
    }
    else {
        if (functions_ret_types.top().type != AST::TYPE_NOTH) {
            throw_exception(SUB_SEMANTIC, "Nothing-type function cannot return values", rs.line, file_name);
        }
    }
}

SemanticAnalyzer::Value SemanticAnalyzer::analyze_expr(AST::Expr& expr) {
    if (auto lit = dynamic_cast<AST::Literal*>(&expr)) {
        return analyze_literal_expr(*lit);
    }
    else if (auto be = dynamic_cast<AST::BinaryExpr*>(&expr)) {
        return analyze_binary_expr(*be);
    }
    else if (auto ue = dynamic_cast<AST::UnaryExpr*>(&expr)) {
        return analyze_unary_expr(*ue);
    }
    else if (auto ve = dynamic_cast<AST::VarExpr*>(&expr)) {
        return analyze_var_expr(*ve);
    }
    else {
        throw_exception(SUB_SEMANTIC, "An unsupported expression was encountered during compilation. Please check your Topaz compiler version and fix the problematic section of the code", expr.line, file_name);
    }
}

SemanticAnalyzer::Value SemanticAnalyzer::analyze_literal_expr(AST::Literal& lit) {
    return Value(lit.type, lit.value);
}

SemanticAnalyzer::Value SemanticAnalyzer::analyze_binary_expr(AST::BinaryExpr& be) {
    Value left_val = analyze_expr(*be.left_expr);
    Value right_val = analyze_expr(*be.right_expr);
    AST::Type left_type = left_val.type;
    AST::Type right_type = right_val.type;

    AST::Type output_type = get_common_type(left_type, right_type, be.line);

    if (left_type.type >= AST::TYPE_BOOL && left_type.type <= AST::TYPE_DOUBLE && right_type.type > AST::TYPE_DOUBLE ||
             right_type.type >= AST::TYPE_BOOL && right_type.type <= AST::TYPE_DOUBLE && left_type.type > AST::TYPE_DOUBLE) {
        std::stringstream ss;
        ss << "Type mismatch: it is not possible to use the binary \033[0m'" << be.op.value <<"'\033[31m operator with \033[0m'" << left_type.to_str() << "'\033[31m and \033[0m'" << right_type.to_str() <<"'\033[31m types";
        throw_exception(SUB_SEMANTIC, ss.str(), be.line, file_name);
    }
    else {
        if (left_type.type == AST::TYPE_STRING_LIT && right_type.type == AST::TYPE_STRING_LIT) {
            if (be.op.type != TOK_OP_PLUS) {
                std::stringstream ss;
                ss << "Type mismatch: it is not possible to use the binary \033[0m'" << be.op.value <<"'\033[31m operator with \033[0m'" << left_type.to_str() << "'\033[31m and \033[0m'" << right_type.to_str() <<"'\033[31m types";
                throw_exception(SUB_SEMANTIC, ss.str(), be.line, file_name);
            }
            return Value(AST::Type(AST::TYPE_STRING_LIT, "string"), std::get<7>(left_val.value.value) + std::get<7>(right_val.value.value));
        }
        switch (be.op.type) {
            #define VALUE(op, type) Value(output_type, static_cast<type>(binary_two_variants(left_val, right_val, op, be.line)))
            case TOK_OP_PLUS:
            case TOK_OP_MINUS:
            case TOK_OP_MULT:
            case TOK_OP_DIV:
            case TOK_OP_MODULO:
                if (be.op.type >= TOK_OP_PLUS && be.op.type <= TOK_OP_MODULO &&
                    (be.op.type != TOK_OP_PLUS || left_type.type != AST::TYPE_STRING_LIT || right_type.type != AST::TYPE_STRING_LIT) &&
                    (left_type.type > AST::TYPE_DOUBLE || right_type.type > AST::TYPE_DOUBLE)) {
                    std::stringstream ss;
                    ss << "Type mismatch: it is not possible to use the binary \033[0m'" << be.op.value <<"'\033[31m operator with \033[0m'" << left_type.to_str() << "'\033[31m and \033[0m'" << right_type.to_str() <<"'\033[31m types";
                    throw_exception(SUB_SEMANTIC, ss.str(), be.line, file_name);
                }
            case TOK_OP_EQ_EQ:
            case TOK_OP_NOT_EQ_EQ:
            case TOK_OP_GT:
            case TOK_OP_GT_EQ:
            case TOK_OP_LS:
            case TOK_OP_LS_EQ:
                if (be.op.type >= TOK_OP_EQ_EQ) {
                    output_type = AST::Type(AST::TYPE_BOOL, "bool");
                }
                if (be.op.type > TOK_OP_NOT_EQ_EQ && (left_type.type > AST::TYPE_DOUBLE || left_type.type == AST::TYPE_BOOL || right_type.type > AST::TYPE_DOUBLE || right_type.type == AST::TYPE_BOOL)) {
                    std::stringstream ss;
                    ss << "Type mismatch: it is not possible to use the binary \033[0m'" << be.op.value <<"'\033[31m operator with \033[0m'" << left_type.to_str() << "'\033[31m and \033[0m'" << right_type.to_str() <<"'\033[31m types";
                    throw_exception(SUB_SEMANTIC, ss.str(), be.line, file_name);
                }
            case TOK_OP_L_AND:
            case TOK_OP_L_OR:
                if (be.op.type >= TOK_OP_L_AND && be.op.type <= TOK_OP_L_OR && (left_type.type != AST::TYPE_BOOL || right_type.type != AST::TYPE_BOOL)) {
                    std::stringstream ss;
                    ss << "Type mismatch: it is not possible to use the binary \033[0m'" << be.op.value <<"'\033[31m operator with \033[0m'" << left_type.to_str() << "'\033[31m and \033[0m'" << right_type.to_str() <<"'\033[31m types";
                    throw_exception(SUB_SEMANTIC, ss.str(), be.line, file_name);
                }
                switch (output_type.type) {
                    case AST::TYPE_BOOL:
                        return VALUE(be.op.type, bool);
                    case AST::TYPE_CHAR:
                        return VALUE(be.op.type, char);
                    case AST::TYPE_SHORT:
                        return VALUE(be.op.type, short);
                    case AST::TYPE_INT:
                        return VALUE(be.op.type, int);
                    case AST::TYPE_LONG:
                        return VALUE(be.op.type, long);
                    case AST::TYPE_FLOAT:
                        return VALUE(be.op.type, float);
                    case AST::TYPE_DOUBLE:
                        return VALUE(be.op.type, double);
                }
            default: {}
            #undef VALUE
        }
    }
}

SemanticAnalyzer::Value SemanticAnalyzer::analyze_unary_expr(AST::UnaryExpr& ue) {
    Value val = analyze_expr(*ue.expr);
    AST::Type type = val.type;
    
    switch (ue.op.type) {
        #define VALUE(op, needed_type) Value(type, static_cast<needed_type>(unary_two_variants(val, op, ue.line)))
        case TOK_OP_MINUS:
            if (ue.op.type == TOK_OP_MINUS && (type.type > AST::TYPE_DOUBLE || type.type == AST::TYPE_BOOL)) {
                std::stringstream ss;
                ss << "Type mismatch: it is not possible to use the unary \033[0m'" << ue.op.value <<"'\033[31m operator with \033[0m'" << type.to_str() << "'\033[31m type";
                throw_exception(SUB_SEMANTIC, ss.str(), ue.line, file_name);
            }
        case TOK_OP_L_NOT:
            if (ue.op.type == TOK_OP_L_NOT && type.type != AST::TYPE_BOOL) {
                std::stringstream ss;
                ss << "Type mismatch: it is not possible to use the unary \033[0m'" << ue.op.value <<"'\033[31m operator with \033[0m'" << type.to_str() << "'\033[31m type";
                throw_exception(SUB_SEMANTIC, ss.str(), ue.line, file_name);
            }
            switch (type.type) {
                case AST::TYPE_BOOL:
                    return VALUE(ue.op.type, bool);
                case AST::TYPE_CHAR:
                    return VALUE(ue.op.type, char);
                case AST::TYPE_SHORT:
                    return VALUE(ue.op.type, short);
                case AST::TYPE_INT:
                    return VALUE(ue.op.type, int);
                case AST::TYPE_LONG:
                    return VALUE(ue.op.type, long);
                case AST::TYPE_FLOAT:
                    return VALUE(ue.op.type, float);
                case AST::TYPE_DOUBLE:
                    return VALUE(ue.op.type, double);
            }
        default: {}
        #undef VALUE
    }
}

SemanticAnalyzer::Value SemanticAnalyzer::analyze_var_expr(AST::VarExpr& ve) {
    std::unique_ptr<Value> value = get_variable_value(ve.name, ve.line);
    if (value == nullptr) {
        std::stringstream ss;
        ss << "Variable \033[0m'" << ve.name << "'\033[31m does not exists";
        throw_exception(SUB_SEMANTIC, ss.str(), ve.line, file_name);
    }
    return *value;
}

AST::Value SemanticAnalyzer::get_default_val_by_type(AST::Type type, uint32_t line) {
    switch (type.type) {
        case AST::TYPE_BOOL:
            return AST::Value(false);
        case AST::TYPE_CHAR:
            return AST::Value('\0');
        case AST::TYPE_SHORT:
            return AST::Value(static_cast<short>(0));
        case AST::TYPE_INT:
            return AST::Value(0);
        case AST::TYPE_LONG:
            return AST::Value(0L);
        case AST::TYPE_FLOAT:
            return AST::Value(0.0F);
        case AST::TYPE_DOUBLE:
            return AST::Value(0.0);
        default:
            std::stringstream ss;
            ss << "Cannot generate default value for '" << type.to_str() << "' type";
            throw_exception(SUB_SEMANTIC, ss.str(), line, file_name);
    }
}

std::unique_ptr<SemanticAnalyzer::Value> SemanticAnalyzer::get_variable_value(std::string name, uint32_t line) {
    auto vars = variables;
    while (!vars.empty()) {
        auto vars_it = vars.top().find(name);
        if (vars_it != vars.top().end()) {
            return std::make_unique<Value>(vars_it->second);
        }
        vars.pop();
    }
    return nullptr;
}

SemanticAnalyzer::FunctionInfo *SemanticAnalyzer::get_function_info(std::string name, uint32_t line) {
    auto func_it = functions.find(name);
    if (func_it != functions.end()) {
        return &*func_it->second;
    }
    return nullptr;
}

bool SemanticAnalyzer::has_common_type(AST::Type left, AST::Type right) {
    if (left == right) {
        return true;
    }
    if (implicitly_cast_allowed_types.find(left.type) != implicitly_cast_allowed_types.end() &&
        std::find(implicitly_cast_allowed_types[left.type].begin(), implicitly_cast_allowed_types[left.type].end(), right.type) != implicitly_cast_allowed_types[left.type].end()) {
        return true;
    }
    return false;
}

AST::Type SemanticAnalyzer::get_common_type(AST::Type left, AST::Type right, uint32_t line) {
    if (left == right) {
        return left;
    }

    if (has_common_type(left, right)) {
        return AST::Type(implicitly_cast_allowed_types[left.type][right.type], right.name, right.is_const, right.is_ptr, right.is_nullable);
    }

    std::stringstream ss;
    ss << "Type mismatch: there is no common type for \033[0m'" << left.to_str() << "'\033[31m and \033[0m'" << right.to_str() << "'\033[31m";
    throw_exception(SUB_SEMANTIC, ss.str(), line, file_name);
}

double SemanticAnalyzer::binary_two_variants(Value left, Value right, TokenType op, uint32_t line) {
    double left_val = 0;
    double right_val = 0;
    switch (left.type.type) {
        case AST::TYPE_BOOL:
            left_val = std::get<0>(left.value.value);
            break;
        case AST::TYPE_CHAR:
            left_val = std::get<1>(left.value.value);
            break;
        case AST::TYPE_SHORT:
            left_val = std::get<2>(left.value.value);
            break;
        case AST::TYPE_INT:
            left_val = std::get<3>(left.value.value);
            break;
        case AST::TYPE_LONG:
            left_val = std::get<4>(left.value.value);
            break;
        case AST::TYPE_FLOAT:
            left_val = std::get<5>(left.value.value);
            break;
        case AST::TYPE_DOUBLE:
            left_val = std::get<6>(left.value.value);
            break;
    }
    switch (right.type.type) {
        case AST::TYPE_BOOL:
            right_val = std::get<0>(right.value.value);
            break;
        case AST::TYPE_CHAR:
            right_val = std::get<1>(right.value.value);
            break;
        case AST::TYPE_SHORT:
            right_val = std::get<2>(right.value.value);
            break;
        case AST::TYPE_INT:
            right_val = std::get<3>(right.value.value);
            break;
        case AST::TYPE_LONG:
            right_val = std::get<4>(right.value.value);
            break;
        case AST::TYPE_FLOAT:
            right_val = std::get<5>(right.value.value);
            break;
        case AST::TYPE_DOUBLE:
            right_val = std::get<6>(right.value.value);
            break;
    }
    switch (op) {
        case TOK_OP_PLUS:
            return left_val + right_val;
        case TOK_OP_MINUS:
            return left_val - right_val;
        case TOK_OP_MULT:
            return left_val * right_val;
        case TOK_OP_DIV:
            if (right_val == 0) {
                throw_exception(SUB_SEMANTIC, "Division by zero", line, file_name);
            }
            return left_val / right_val;
        case TOK_OP_MODULO:
            return std::fmod(left_val, right_val);
        case TOK_OP_EQ_EQ:
            return static_cast<bool>(left_val == right_val);
        case TOK_OP_NOT_EQ_EQ:
            return static_cast<bool>(left_val != right_val);
        case TOK_OP_GT:
            return static_cast<bool>(left_val > right_val);
        case TOK_OP_GT_EQ:
            return static_cast<bool>(left_val >= right_val);
        case TOK_OP_LS:
            return static_cast<bool>(left_val < right_val);
        case TOK_OP_LS_EQ:
            return static_cast<bool>(left_val <= right_val);
        case TOK_OP_L_AND:
            return static_cast<bool>(left_val && right_val);
        case TOK_OP_L_OR:
            return static_cast<bool>(left_val || right_val);
        default:
            std::stringstream ss;
            ss << "Unsupported binary operator: \033[0m'" << op << "'";
            throw_exception(SUB_SEMANTIC, ss.str(), line, file_name);
    }
}

double SemanticAnalyzer::unary_two_variants(Value value, TokenType op, uint32_t line) {
    double val = 0;
    switch (value.type.type) {
        case AST::TYPE_BOOL:
            val = std::get<0>(value.value.value);
            break;
        case AST::TYPE_CHAR:
            val = std::get<1>(value.value.value);
            break;
        case AST::TYPE_SHORT:
            val = std::get<2>(value.value.value);
            break;
        case AST::TYPE_INT:
            val = std::get<3>(value.value.value);
            break;
        case AST::TYPE_LONG:
            val = std::get<4>(value.value.value);
            break;
        case AST::TYPE_FLOAT:
            val = std::get<5>(value.value.value);
            break;
        case AST::TYPE_DOUBLE:
            val = std::get<6>(value.value.value);
            break;
    }
    switch (op) {
        case TOK_OP_MINUS:
            return -val;
        case TOK_OP_L_NOT:
            return static_cast<bool>(!val);
        default:
            std::stringstream ss;
            ss << "Unsupported binary operator: \033[0m'" << op << "'";
            throw_exception(SUB_SEMANTIC, ss.str(), line, file_name);
    }
}