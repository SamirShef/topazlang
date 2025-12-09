/**
 * @file semantic.cpp
 *
 * @brief semantic.hpp implementation
 */

#include "../../include/exception/exception.hpp"
#include "../../include/semantic/semantic.hpp"
#include "../../include/parser/parser.hpp"
#include "../../include/lexer/lexer.hpp"
#include <algorithm>
#include <climits>
#include <fstream>

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
        if (current_space != SPACE_FUNCTION) {
            throw_exception(SUB_SEMANTIC, "Assignment of variable cannot be in global or module space", stmt.line, file_name, is_debug);
        }
        analyze_var_asgn_stmt(*vas);
    }
    else if (auto fds = dynamic_cast<AST::FuncDeclStmt*>(&stmt)) {
        analyze_func_decl_stmt(*fds);
    }
    else if (auto fcs = dynamic_cast<AST::FuncCallStmt*>(&stmt)) {
        if (current_space != SPACE_FUNCTION) {
            throw_exception(SUB_SEMANTIC, "Calling of function cannot be in global or module space", stmt.line, file_name, is_debug);
        }
        analyze_func_call_stmt(*fcs);
    }
    else if (auto rs = dynamic_cast<AST::ReturnStmt*>(&stmt)) {
        analyze_return_stmt(*rs);
    }
    else if (auto ies = dynamic_cast<AST::IfElseStmt*>(&stmt)) {
        if (current_space != SPACE_FUNCTION) {
            throw_exception(SUB_SEMANTIC, "Control flow cannot be in global or module space", stmt.line, file_name, is_debug);
        }
        analyze_if_else_stmt(*ies);
    }
    else if (auto wcs = dynamic_cast<AST::WhileCycleStmt*>(&stmt)) {
        if (current_space != SPACE_FUNCTION) {
            throw_exception(SUB_SEMANTIC, "While loop cannot be in global or module space", stmt.line, file_name, is_debug);
        }
        analyze_while_cycle_stmt(*wcs);
    }
    else if (auto dwcs = dynamic_cast<AST::DoWhileCycleStmt*>(&stmt)) {
        if (current_space != SPACE_FUNCTION) {
            throw_exception(SUB_SEMANTIC, "Do-while loop cannot be in global or module space", stmt.line, file_name, is_debug);
        }
        analyze_do_while_cycle_stmt(*dwcs);
    }
    else if (auto fcs = dynamic_cast<AST::ForCycleStmt*>(&stmt)) {
        if (current_space != SPACE_FUNCTION) {
            throw_exception(SUB_SEMANTIC, "For loop cannot be in global or module space", stmt.line, file_name, is_debug);
        }
        analyze_for_cycle_stmt(*fcs);
    }
    else if (auto bs = dynamic_cast<AST::BreakStmt*>(&stmt)) {
        analyze_break_stmt(*bs);
    }
    else if (auto cs = dynamic_cast<AST::ContinueStmt*>(&stmt)) {
        analyze_continue_stmt(*cs);
    }
    else if (auto ms = dynamic_cast<AST::ModuleStmt*>(&stmt)) {
        analyze_module_stmt(*ms);
    }
    else if (auto ums = dynamic_cast<AST::UseModuleStmt*>(&stmt)) {
        analyze_use_module_stmt(*ums);
    }
    else {
        throw_exception(SUB_SEMANTIC, "Unsupported statement. Please check your Topaz compiler version and fix the problematic section of the code", stmt.line, file_name, is_debug);
    }
}

void SemanticAnalyzer::analyze_var_decl_stmt(AST::VarDeclStmt& vds, bool is_func_arg) {
    if (current_space != SPACE_MODULE && vds.access != AST::ACCESS_NONE) {
        std::stringstream ss;
        ss << "Variable \033[0m'" << vds.name << "'\033[31m cannot have access modifier outside the module";
        throw_exception(SUB_SEMANTIC, ss.str(), vds.line, file_name, is_debug);
    }
    std::unique_ptr<Value> value = get_variable_value(vds.name);
    if (value != nullptr) {
        std::stringstream ss;
        ss << "Variable \033[0m'" << vds.name << "'\033[31m already exists";
        throw_exception(SUB_SEMANTIC, ss.str(), vds.line, file_name, is_debug);
    }
    AST::Type var_type = vds.type;
    Value var_val = Value(var_type, get_default_val_by_type(var_type, vds.line), false, false);
    if (vds.expr != nullptr) {
        var_val = analyze_expr(*vds.expr);
    }
    if (is_func_arg) {
        var_val.is_value_unknown = true;
        var_val.is_literal = false;
    }
    bool ok = false;
    if (var_type.type >= AST::TYPE_CHAR && var_type.type <= AST::TYPE_LONG &&
        var_val.type.type >= AST::TYPE_CHAR && var_val.type.type <= AST::TYPE_LONG) {
        if (!var_val.is_value_unknown && var_val.is_literal) {
            double var_val_val;
            size_t max_val;
            switch (var_val.type.type) {
                case AST::TYPE_CHAR:
                    var_val_val = std::get<1>(var_val.value.value);
                    break;
                case AST::TYPE_SHORT:
                    var_val_val = std::get<2>(var_val.value.value);
                    break;
                case AST::TYPE_INT:
                    var_val_val = std::get<3>(var_val.value.value);
                    break;
                case AST::TYPE_LONG:
                    var_val_val = std::get<4>(var_val.value.value);
                    break;
            }
            switch (var_type.type) {
                case AST::TYPE_CHAR:
                    max_val = CHAR_MAX;
                    break;
                case AST::TYPE_SHORT:
                    max_val = INT16_MAX;
                    break;
                case AST::TYPE_INT:
                    max_val = INT_MAX;
                    break;
                case AST::TYPE_LONG:
                    max_val = INT64_MAX;
                    break;
            }
            if (var_val_val >= 0 && var_val_val <= max_val) {
                ok = true;
            }
            else if (var_val_val < 0 && std::abs(var_val_val) <= max_val + 1) {
                ok = true;
            }
            else {
                std::stringstream ss;
                ss << "Value of expression is does not fit into the variable type: \033[0m'" << var_type.to_str() << " (from " << (long)(-(max_val + 1)) << " to "
                   << max_val << ")'\033[31m, passed value: \033[0m'" << var_val_val << "'\033[31m";
                throw_exception(SUB_SEMANTIC, ss.str(), vds.line, file_name, is_debug);
            }
        }
    }
    if (!ok && !has_common_type(var_val.type, var_type)) {
        std::stringstream ss;
        ss << "Type mismatch: an expression of the type \033[0m'" << var_val.type.to_str() << "'\033[31m, but the type is expected \033[0m'" << var_type.to_str() << "'\033[31m";
        throw_exception(SUB_SEMANTIC, ss.str(), vds.line, file_name, is_debug);
    }
    var_val = implicitly_cast(var_val, var_type, vds.line);
    variables.top().emplace(vds.name, var_val);
}

void SemanticAnalyzer::analyze_var_asgn_stmt(AST::VarAsgnStmt& vas) {
    std::unique_ptr<Value> var_val = get_variable_value(vas.name);
    if (var_val == nullptr) {
        std::stringstream ss;
        ss << "Variable \033[0m'" << vas.name << "'\033[31m does not exists";
        throw_exception(SUB_SEMANTIC, ss.str(), vas.line, file_name, is_debug);
    }
    if (var_val->type.is_const) {
        std::stringstream ss;
        ss << "Variable \033[0m'" << vas.name << "'\033[31m is a constant";
        throw_exception(SUB_SEMANTIC, ss.str(), vas.line, file_name, is_debug);
    }
    AST::Type var_type = var_val->type;
    Value new_val = analyze_expr(*vas.expr);
    if (!has_common_type(new_val.type, var_type)) {
        std::stringstream ss;
        ss << "Type mismatch: an expression of the type \033[0m'" << new_val.type.to_str() << "'\033[31m, but the type is expected \033[0m'" << var_type.to_str() << "'\033[31m";
        throw_exception(SUB_SEMANTIC, ss.str(), vas.line, file_name, is_debug);
    }
}

void SemanticAnalyzer::analyze_func_decl_stmt(AST::FuncDeclStmt& fds) {
    if (current_space != SPACE_MODULE && fds.access != AST::ACCESS_NONE) {
        std::stringstream ss;
        ss << "Function \033[0m'" << fds.ret_type.to_str() << ' ' << get_mangled_name(fds.name) << '(';
        for (size_t i = 0; i < fds.args.size(); i++) {
            ss << fds.args[i].type.to_str();
            if (i < fds.args.size() - 1) {
                ss << ", ";
            }
        }
        ss << ")'\033[31m cannot have access modifier outside the module";
        throw_exception(SUB_SEMANTIC, ss.str(), fds.line, file_name, is_debug);
    }
    
    Space previous_space = current_space;
    current_space = SPACE_FUNCTION;
    
    auto func_candidates = get_function_candidates(get_mangled_name(fds.name));
    if (!func_candidates.empty()) {
        bool ok = false;                                        // Can you define this function (true) or not (false)
        for (auto& candidate : func_candidates) {
            if (candidate->args.size() != fds.args.size()) {
                ok = true;
            }
            else {
                size_t coincidences = 0;
                for (size_t i = 0; i < candidate->args.size(); i++) {
                    if (candidate->args[i].type == fds.args[i].type) {
                        coincidences++;
                    }
                }
                if (coincidences == candidate->args.size()) {
                    ok = false;
                    break;
                }
                else {
                    ok = true;
                }
            }
        }
        if (!ok) {
            std::stringstream ss;
            ss << "Function \033[0m'" << fds.ret_type.to_str() << ' ' << get_mangled_name(fds.name) << '(';
            for (size_t i = 0; i < fds.args.size(); i++) {
                ss << fds.args[i].type.to_str();
                if (i < fds.args.size() - 1) {
                    ss << ", ";
                }
            }
            ss << ")'\033[31m already exists";
            throw_exception(SUB_SEMANTIC, ss.str(), fds.line, file_name, is_debug);
        }
    }
    AST::Type ret_type = fds.ret_type;
    variables.push({});
    functions_ret_types.push(ret_type);
    std::shared_ptr<FunctionInfo> new_func = std::make_shared<FunctionInfo>(ret_type, std::move(fds.args), std::move(fds.block));
    if (func_candidates.empty()) {
        functions.emplace(get_mangled_name(fds.name), std::vector<std::shared_ptr<FunctionInfo>>{new_func});
    }
    else {
        functions.at(get_mangled_name(fds.name)).push_back(new_func);
    }
    for (auto& arg : new_func->args) {
        analyze_var_decl_stmt(*std::make_unique<AST::VarDeclStmt>(AST::ACCESS_NONE, arg.type, nullptr, arg.name, fds.line), true);
    }
    bool have_ret_in_global = false;
    for (auto& stmt : new_func->block) {
        analyze_stmt(*stmt);
        if (auto rs = dynamic_cast<AST::ReturnStmt*>(&*stmt)) {
            have_ret_in_global = true;
        }
        else if (auto ies = dynamic_cast<AST::IfElseStmt*>(&*stmt)) {
            Value *val = get_function_return_value_from_if_else(*ies);
            if (val != nullptr) {
                have_ret_in_global = true;
            }
        }
        else if (auto wcs = dynamic_cast<AST::WhileCycleStmt*>(&*stmt)) {
            Value *val = get_function_return_value_from_while_cycle(*wcs);
            if (val != nullptr) {
                have_ret_in_global = true;
            }
        }
        else if (auto dwcs = dynamic_cast<AST::DoWhileCycleStmt*>(&*stmt)) {
            Value *val = get_function_return_value_from_do_while_cycle(*dwcs);
            if (val != nullptr) {
                have_ret_in_global = true;
            }
        }
        else if (auto fcs = dynamic_cast<AST::ForCycleStmt*>(&*stmt)) {
            Value *val = get_function_return_value_from_for_cycle(*fcs);
            if (val != nullptr) {
                have_ret_in_global = true;
            }
        }
    }
    if (!have_ret_in_global && ret_type.type != AST::TYPE_NOTH) {
        std::stringstream ss;
        ss << "Non-nothing function must be have return statement in the global space in the body of function. Please add or move return statement to the global space in the body of function";
        throw_exception(SUB_SEMANTIC, ss.str(), fds.line, file_name, is_debug);
    }
    functions_ret_types.pop();
    variables.pop();

    current_space = previous_space;
}

void SemanticAnalyzer::analyze_func_call_stmt(AST::FuncCallStmt& fcs) {
    auto func_candidates = get_function_candidates(get_mangled_name(fcs.name));
    if (func_candidates.empty()) {
        std::stringstream ss;
        ss << "Function \033[0m'" << get_mangled_name(fcs.name) << '(';
        for (size_t i = 0; i < fcs.args.size(); i++) {
            ss << analyze_expr(*fcs.args[i]).type.to_str();
            if (i < fcs.args.size() - 1) {
                ss << ", ";
            }
        }
        ss << ")'\033[31m does not exists";
        throw_exception(SUB_SEMANTIC, ss.str(), fcs.line, file_name, is_debug);
    }
    bool found = false;
    size_t coincidences = 0;
    for (auto& candidate : func_candidates) {
        for (size_t i = 0; i < candidate->args.size(); i++) {
            if (fcs.args.size() != candidate->args.size()) {
                continue;
            }
            Value arg_val = analyze_expr(*fcs.args[i]);
            if (has_common_type(arg_val.type, candidate->args[i].type)) {
                coincidences++;
            }
        }
        if (coincidences == candidate->args.size()) {
            found = true;
            break;
        }
    }
    if (!found) {
        std::stringstream ss;
        ss << "Function \033[0m'" << get_mangled_name(fcs.name) << "'\033[31m does not have needed candidate.\nExists candidates:\n\033[0m";
        size_t index = 0;
        for (auto& candidate : func_candidates) {
            ss << candidate->ret_type.to_str() << ' ' << get_mangled_name(fcs.name) << '(';
            for (size_t i = 0; i < candidate->args.size(); i++) {
                ss << candidate->args[i].type.to_str();
                if (i < candidate->args.size() - 1) {
                    ss << ", ";
                }
            }
            ss << ')';
            if (index < func_candidates.size() - 1) {
                ss << '\n';
            }
            index++;
        }
        ss << "\033[31m";
        throw_exception(SUB_SEMANTIC, ss.str(), fcs.line, file_name, is_debug); 
    }
    analyze_func_call_expr(*std::make_unique<AST::FuncCallExpr>(fcs.name, std::move(fcs.args), fcs.line));
}

void SemanticAnalyzer::analyze_return_stmt(AST::ReturnStmt& rs) {
    if (functions_ret_types.empty()) {
        throw_exception(SUB_SEMANTIC, "\033[0m'return'\033[31m statement must be in a function", rs.line, file_name, is_debug);
    }
    if (rs.expr != nullptr) {
        Value val = analyze_expr(*rs.expr);
        if (!has_common_type(val.type, functions_ret_types.top())) {
            std::stringstream ss;
            ss << "Type mismatch: an expression of the type \033[0m'" << val.type.to_str() << "'\033[31m, but the type is expected \033[0m'" << functions_ret_types.top().to_str() << "'\033[31m";
            throw_exception(SUB_SEMANTIC, ss.str(), rs.line, file_name, is_debug);
        }
    }
    else {
        if (functions_ret_types.top().type != AST::TYPE_NOTH) {
            throw_exception(SUB_SEMANTIC, "Nothing-type function cannot return values", rs.line, file_name, is_debug);
        }
    }
}

void SemanticAnalyzer::analyze_if_else_stmt(AST::IfElseStmt& ies) {
    Value cond_val = analyze_expr(*ies.cond);
    if (cond_val.type.type != AST::TYPE_BOOL) {
        std::stringstream ss;
        ss << "Type mismatch: the condition of the \033[0m'if'\033[31m operator must be of type \033[0m'bool'\033[31m, but got \033[0m'" << cond_val.type.to_str() << "'\033[31m";
        throw_exception(SUB_SEMANTIC, ss.str(), ies.line, file_name, is_debug);
    }
    variables.push({});
    for (auto& stmt : ies.then_block) {
        analyze_stmt(*stmt);
    }
    variables.pop();
    variables.push({});
    if (ies.else_block.size() != 0) {
        for (auto& stmt : ies.else_block) {
            analyze_stmt(*stmt);
        }
    }
    variables.pop();
}

void SemanticAnalyzer::analyze_while_cycle_stmt(AST::WhileCycleStmt& wcs) {
    Value cond_val = analyze_expr(*wcs.cond);
    if (cond_val.type.type != AST::TYPE_BOOL) {
        std::stringstream ss;
        ss << "Type mismatch: the condition of the \033[0m'while'\033[31m cycle must be of type \033[0m'bool'\033[31m, but got \033[0m'" << cond_val.type.to_str() << "'\033[31m";
        throw_exception(SUB_SEMANTIC, ss.str(), wcs.line, file_name, is_debug);
    }
    variables.push({});
    depth_of_loops++;
    for (auto& stmt : wcs.block) {
        analyze_stmt(*stmt);
    }
    depth_of_loops--;
    variables.pop();
}

void SemanticAnalyzer::analyze_do_while_cycle_stmt(AST::DoWhileCycleStmt& dwcs) {
    Value cond_val = analyze_expr(*dwcs.cond);
    if (cond_val.type.type != AST::TYPE_BOOL) {
        std::stringstream ss;
        ss << "Type mismatch: the condition of the \033[0m'do-while'\033[31m cycle must be of type \033[0m'bool'\033[31m, but got \033[0m'" << cond_val.type.to_str() << "'\033[31m";
        throw_exception(SUB_SEMANTIC, ss.str(), dwcs.line, file_name, is_debug);
    }
    variables.push({});
    depth_of_loops++;
    for (auto& stmt : dwcs.block) {
        analyze_stmt(*stmt);
    }
    depth_of_loops--;
    variables.pop();
}

void SemanticAnalyzer::analyze_for_cycle_stmt(AST::ForCycleStmt& fcs) {
    if (!dynamic_cast<AST::VarDeclStmt*>(&*fcs.indexator) && !dynamic_cast<AST::VarAsgnStmt*>(&*fcs.indexator)) {
        throw_exception(SUB_SEMANTIC, "Indexator statement in \033[0m'for'\033[31m cycle must be a variable definition/assignment", fcs.indexator->line, file_name, is_debug);
    }
    
    variables.push({});
    analyze_stmt(*fcs.indexator);
    Value cond_val = analyze_expr(*fcs.cond);
    if (cond_val.type.type != AST::TYPE_BOOL) {
        std::stringstream ss;
        ss << "Type mismatch: the condition of the \033[0m'for'\033[31m cycle must be of type \033[0m'bool'\033[31m, but got \033[0m'" << cond_val.type.to_str() << "'\033[31m";
        throw_exception(SUB_SEMANTIC, ss.str(), fcs.line, file_name, is_debug);
    }
    analyze_stmt(*fcs.iteration);
    depth_of_loops++;
    for (auto& stmt : fcs.block) {
        analyze_stmt(*stmt);
    }
    depth_of_loops--;
    variables.pop();
}

void SemanticAnalyzer::analyze_break_stmt(AST::BreakStmt& bs) {
    if (depth_of_loops == 0) {
        throw_exception(SUB_SEMANTIC, "\033[0m'break'\033[31m statement must be in a cycle", bs.line, file_name, is_debug);
    }
}

void SemanticAnalyzer::analyze_continue_stmt(AST::ContinueStmt& cs) {
    if (depth_of_loops == 0) {
        throw_exception(SUB_SEMANTIC, "\033[0m'continue'\033[31m statement must be in a cycle", cs.line, file_name, is_debug);
    }
}

void SemanticAnalyzer::analyze_module_stmt(AST::ModuleStmt& ms) {
    if (current_space != SPACE_MODULE && ms.access != AST::ACCESS_NONE) {
        std::stringstream ss;
        ss << "Module \033[0m'" << ms.name << "'\033[31m cannot have access modifier outside the module";
        throw_exception(SUB_SEMANTIC, ss.str(), ms.line, file_name, is_debug);
    }
    
    Space previous_space = current_space;
    current_space = SPACE_MODULE;

    variables.push({});
    if (modules.find(get_mangled_name(ms.name)) != modules.end()) {
        std::stringstream ss;
        ss << "Module \033[0m'" << ms.name << "'\033[31m already exists";
        throw_exception(SUB_SEMANTIC, ss.str(), ms.line, file_name, is_debug);
    }
    modules.emplace(get_mangled_name(ms.name), new ModuleInfo());
    ModuleInfo *module = modules.find(get_mangled_name(ms.name))->second;
    current_path.push(PathPart{.name=ms.name, .object=PathPart::OBJ_MODULE});
    for (auto& stmt : ms.block) {
        if (auto sms = dynamic_cast<AST::ModuleStmt*>(&*stmt)) {
            analyze_module_stmt(*sms);
            module->modules.emplace(sms->name, std::make_pair(sms->access, new ModuleInfo()));
            ModuleInfo *submodule = module->modules.find(sms->name)->second.second;
            submodule->functions = modules.find(get_mangled_name(sms->name))->second->functions;
            submodule->modules = modules.find(get_mangled_name(sms->name))->second->modules;
        }
        else if (auto fds = dynamic_cast<AST::FuncDeclStmt*>(&*stmt)) {
            module->functions.emplace(fds->name, std::make_pair(fds->access, get_mangled_name(fds->name)));
            analyze_func_decl_stmt(*fds);
        }
    }
    current_path.pop();
    variables.pop();
    
    current_space = previous_space;
}

void SemanticAnalyzer::analyze_use_module_stmt(AST::UseModuleStmt& ums) {
    std::string all_name;
    for (size_t i = 0; i < ums.path.size(); i++) {
        all_name += ums.path[i];
        if (i != ums.path.size() - 1) {
            all_name += "-";
        }
    }
    bool found = false;
    if (modules.find(all_name) != modules.end()) {
        found = true;
    }
    if (!found) {
        std::filesystem::path path_to_mod_without_ext;
        for (size_t i = 0; i < ums.path.size(); i++) {
            path_to_mod_without_ext += "/" + ums.path[i];
        }
        std::filesystem::path path_to_mod_without_ext_in_libs = libs_path + path_to_mod_without_ext.string();
        std::string path_to_mod_without_ext_in_libs_as_str = path_to_mod_without_ext_in_libs.string();
        std::ifstream file;
        if (std::filesystem::is_directory(path_to_mod_without_ext_in_libs_as_str)) {
            if (std::filesystem::exists(path_to_mod_without_ext_in_libs_as_str + "/main.tp")) {
                file = std::ifstream(path_to_mod_without_ext_in_libs_as_str + "/main.tp");
                if (!file.is_open()) {
                    throw_exception(SUB_SEMANTIC, "Error openning file: does not exist!", ums.line, file_name, is_debug);
                }
                std::ostringstream content;
                content << file.rdbuf();
                file.close();
                Lexer lex(content.str(), path_to_mod_without_ext_in_libs_as_str + "/main.tp", is_debug);
                std::vector<Token> tokens = lex.tokenize();
                Parser parser(tokens, is_debug);
                std::vector<AST::StmtPtr> stmts = parser.parse();
                SemanticAnalyzer semantic(stmts, libs_path, path_to_mod_without_ext_in_libs_as_str + "/main.tp", is_debug);
                semantic.analyze();
                std::map<std::string, ModuleInfo*> modules = semantic.get_modules();
                if (modules.find(all_name) == modules.end()) {
                    std::stringstream ss;
                    ss << "File \033[0m'" << path_to_mod_without_ext_in_libs_as_str + "/main.tp" << "'\033[31m does not have a module with name \033[0m'" << all_name << "'\033[31m inside";
                    throw_exception(SUB_SEMANTIC, ss.str(), ums.line, file_name, is_debug);
                }
                size_t current_path_size = current_path.size();
                for (auto& module : modules) {
                    this->modules.emplace(module.first, module.second);
                    std::vector<PathPart> resolved_name = get_resolved_name(module.first);
                    current_path.push({resolved_name.back().name, PathPart::OBJ_MODULE});
                    auto functions = semantic.get_functions();
                    for (auto& func : module.second->functions) {
                        this->functions.emplace(get_mangled_name(func.first), std::move(functions.at(get_mangled_name(func.first))));
                    }
                }
                for (size_t i = current_path.size() - current_path_size; i > 0; i--) {
                    current_path.pop();
                }
            }
        }
        else {
            std::string path_to_file;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(libs_path)) {
                if (entry.path().filename() == ums.path.back() + ".tp" && !std::filesystem::is_directory(entry.path())) {
                    path_to_file = entry.path();
                    file = std::ifstream(path_to_file);
                    break;
                }
            }
            if (!file.is_open()) {
                std::stringstream ss;
                ss << "Module \033[0m'" << all_name << "'\033[31m does not exists";
                throw_exception(SUB_SEMANTIC, ss.str(), ums.line, file_name, is_debug);
            }
            std::ostringstream content;
            content << file.rdbuf();
            file.close();
            Lexer lex(content.str(), path_to_mod_without_ext_in_libs_as_str + ".tp", is_debug);
            std::vector<Token> tokens = lex.tokenize();
            Parser parser(tokens, is_debug);
            std::vector<AST::StmtPtr> stmts = parser.parse();
            SemanticAnalyzer semantic(stmts, libs_path, path_to_mod_without_ext_in_libs_as_str + ".tp", is_debug);
            semantic.analyze();
            std::map<std::string, ModuleInfo*> modules = semantic.get_modules();
            if (modules.find(all_name) == modules.end()) {
                std::stringstream ss;
                ss << "File \033[0m'" << path_to_file << "'\033[31m does not have a module with name \033[0m'" << all_name << "'\033[31m inside";
                throw_exception(SUB_SEMANTIC, ss.str(), ums.line, file_name, is_debug);
            }
            size_t current_path_size = current_path.size();
            for (auto& module : modules) {
                this->modules.emplace(module.first, module.second);
                std::vector<PathPart> resolved_name = get_resolved_name(module.first);
                current_path.push({resolved_name.back().name, PathPart::OBJ_MODULE});
                auto functions = semantic.get_functions();
                for (auto& func : module.second->functions) {
                    this->functions.emplace(get_mangled_name(func.first), std::move(functions.at(get_mangled_name(func.first))));
                }
            }
            for (size_t i = current_path.size() - current_path_size; i > 0; i--) {
                current_path.pop();
            }
        }
    }
    if (std::find(names_of_imported_modules.begin(), names_of_imported_modules.end(), all_name) != names_of_imported_modules.end()) {
        std::stringstream ss;
        ss << "Module \033[0m'" << ums.path.back() << "'\033[31m already imported";
        throw_exception(SUB_SEMANTIC, ss.str(), ums.line, file_name, is_debug);
    }
    names_of_imported_modules.push_back(all_name);
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
    else if (auto fce = dynamic_cast<AST::FuncCallExpr*>(&expr)) {
        return analyze_func_call_expr(*fce);
    }
    else if (auto oce = dynamic_cast<AST::ChainObjects*>(&expr)) {
        return analyze_obj_chain_expr(*oce);
    }
    else {
        throw_exception(SUB_SEMANTIC, "An unsupported expression was encountered during compilation. Please check your Topaz compiler version and fix the problematic section of the code", expr.line, file_name, is_debug);
    }
}

SemanticAnalyzer::Value SemanticAnalyzer::analyze_literal_expr(AST::Literal& lit) {
    return Value(lit.type, lit.value, false, true);
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
        throw_exception(SUB_SEMANTIC, ss.str(), be.line, file_name, is_debug);
    }
    else {
        if (left_type.type == AST::TYPE_STRING_LIT && right_type.type == AST::TYPE_STRING_LIT) {
            if (be.op.type != TOK_OP_PLUS) {
                std::stringstream ss;
                ss << "Type mismatch: it is not possible to use the binary \033[0m'" << be.op.value <<"'\033[31m operator with \033[0m'" << left_type.to_str() << "'\033[31m and \033[0m'" << right_type.to_str() <<"'\033[31m types";
                throw_exception(SUB_SEMANTIC, ss.str(), be.line, file_name, is_debug);
            }
            return Value(AST::Type(AST::TYPE_STRING_LIT, "string"), std::get<7>(left_val.value.value) + std::get<7>(right_val.value.value), left_val.is_value_unknown || right_val.is_value_unknown, left_val.is_literal && right_val.is_literal);
        }
        switch (be.op.type) {
            #define VALUE(op, type) Value(output_type, static_cast<type>(binary_two_variants(left_val, right_val, op, be.line)), left_val.is_value_unknown || right_val.is_value_unknown, left_val.is_literal && right_val.is_literal)
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
                    throw_exception(SUB_SEMANTIC, ss.str(), be.line, file_name, is_debug);
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
                    throw_exception(SUB_SEMANTIC, ss.str(), be.line, file_name, is_debug);
                }
            case TOK_OP_L_AND:
            case TOK_OP_L_OR:
                if (be.op.type >= TOK_OP_L_AND && be.op.type <= TOK_OP_L_OR && (left_type.type != AST::TYPE_BOOL || right_type.type != AST::TYPE_BOOL)) {
                    std::stringstream ss;
                    ss << "Type mismatch: it is not possible to use the binary \033[0m'" << be.op.value <<"'\033[31m operator with \033[0m'" << left_type.to_str() << "'\033[31m and \033[0m'" << right_type.to_str() <<"'\033[31m types";
                    throw_exception(SUB_SEMANTIC, ss.str(), be.line, file_name, is_debug);
                }
                if (left_val.is_value_unknown || right_val.is_value_unknown) {
                    return Value(output_type, 0, left_val.is_value_unknown || right_val.is_value_unknown, left_val.is_literal && right_val.is_literal);
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

    if (val.is_value_unknown) {
        return Value(type, 0, val.is_value_unknown, val.is_literal);
    }
    
    switch (ue.op.type) {
        #define VALUE(op, needed_type) Value(type, static_cast<needed_type>(unary_two_variants(val, op, ue.line)), val.is_value_unknown, val.is_literal)
        case TOK_OP_MINUS:
            if (ue.op.type == TOK_OP_MINUS && (type.type > AST::TYPE_DOUBLE || type.type == AST::TYPE_BOOL)) {
                std::stringstream ss;
                ss << "Type mismatch: it is not possible to use the unary \033[0m'" << ue.op.value <<"'\033[31m operator with \033[0m'" << type.to_str() << "'\033[31m type";
                throw_exception(SUB_SEMANTIC, ss.str(), ue.line, file_name, is_debug);
            }
        case TOK_OP_L_NOT:
            if (ue.op.type == TOK_OP_L_NOT && type.type != AST::TYPE_BOOL) {
                std::stringstream ss;
                ss << "Type mismatch: it is not possible to use the unary \033[0m'" << ue.op.value <<"'\033[31m operator with \033[0m'" << type.to_str() << "'\033[31m type";
                throw_exception(SUB_SEMANTIC, ss.str(), ue.line, file_name, is_debug);
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
    std::unique_ptr<Value> var = get_variable_value(ve.name);
    if (var == nullptr) {
        if (modules.find(get_mangled_name(ve.name)) != modules.end()) {
            return Value(AST::Type(AST::TYPE_MODULE, get_mangled_name(ve.name)), get_mangled_name(ve.name), true, false);
        }
        std::stringstream ss;
        ss << "Variable \033[0m'" << ve.name << "'\033[31m does not exists";
        throw_exception(SUB_SEMANTIC, ss.str(), ve.line, file_name, is_debug);
    }
    return *var;
}

SemanticAnalyzer::Value SemanticAnalyzer::analyze_func_call_expr(AST::FuncCallExpr& fce) {
    auto func_candidates = get_function_candidates(get_mangled_name(fce.name));
    if (func_candidates.empty()) {
        std::stringstream ss;
        ss << "Function \033[0m'" << get_mangled_name(fce.name) << '(';
        for (size_t i = 0; i < fce.args.size(); i++) {
            ss << analyze_expr(*fce.args[i]).type.to_str();
            if (i < fce.args.size() - 1) {
                ss << ", ";
            }
        }
        ss << ")'\033[31m does not exists";
        throw_exception(SUB_SEMANTIC, ss.str(), fce.line, file_name, is_debug);
    }
    bool found = false;
    size_t last_score = SIZE_MAX;
    size_t best_candidate_index = 0;
    for (size_t candidate_index = 0; candidate_index < func_candidates.size(); candidate_index++) {
        size_t score = 0;
        size_t coincidences = 0;
        auto candidate = func_candidates[candidate_index];
        for (size_t i = 0; i < candidate->args.size(); i++) {
            if (fce.args.size() != candidate->args.size()) {
                continue;
            }
            Value arg_val = analyze_expr(*fce.args[i]);
            if (has_common_type(arg_val.type, candidate->args[i].type)) {
                if (arg_val.type != candidate->args[i].type) {
                    if (arg_val.type.type <= AST::TYPE_LONG && candidate->args[i].type.type <= AST::TYPE_LONG) {
                        score++;
                    }
                    else if (arg_val.type.type >= AST::TYPE_FLOAT && arg_val.type.type <= AST::TYPE_DOUBLE
                          && candidate->args[i].type.type >= AST::TYPE_FLOAT && candidate->args[i].type.type <= AST::TYPE_DOUBLE) {
                        score++;
                    }
                    else if (arg_val.type.type >= AST::TYPE_FLOAT && arg_val.type.type <= AST::TYPE_DOUBLE
                          && candidate->args[i].type.type >= AST::TYPE_FLOAT && candidate->args[i].type.type <= AST::TYPE_DOUBLE) {
                        score++;
                    }
                    else if (arg_val.type.type <= AST::TYPE_DOUBLE && candidate->args[i].type.type <= AST::TYPE_DOUBLE) {
                        score += 2;
                    }
                }
                coincidences++;
            }
            else {
                score += 99;
            }

        }
        if (score <= last_score) {
            best_candidate_index = candidate_index;
            last_score = score;
        }
        if (coincidences == candidate->args.size()) {
            found = true;
        }
    }
    if (!found) {
        std::stringstream ss;
        ss << "Function \033[0m'" << get_mangled_name(fce.name) << '(';
        for (size_t i = 0; i < fce.args.size(); i++) {
            ss << analyze_expr(*fce.args[i]).type.to_str();
            if (i < fce.args.size() - 1) {
                ss << ", ";
            }
        }
        ss << ")'\033[31m does not have needed candidate.\nExists candidates:\n\033[0m";
        size_t index = 0;
        for (auto& candidate : func_candidates) {
            ss << candidate->ret_type.to_str() << ' ' << get_mangled_name(fce.name) << '(';
            for (size_t i = 0; i < candidate->args.size(); i++) {
                ss << candidate->args[i].type.to_str();
                if (i < candidate->args.size() - 1) {
                    ss << ", ";
                }
            }
            ss << ')';
            if (index < func_candidates.size() - 1) {
                ss << '\n';
            }
            index++;
        }
        ss << "\033[31m";
        throw_exception(SUB_SEMANTIC, ss.str(), fce.line, file_name, is_debug); 
    }

    size_t index = 0;
    auto best_candidate = func_candidates[best_candidate_index];
    for (auto& arg : fce.args) {
        AST::Type arg_type = analyze_expr(*arg).type;
        if (!has_common_type(arg_type, best_candidate->args[index].type)) {
            std::stringstream ss;
            ss << "In the " << index + 1 << "th argument: Type mismatch: an expression of the type \033[0m'" << arg_type.to_str() << "'\033[31m, but the type is expected \033[0m'" << best_candidate->args[index].type.to_str() << "'\033[31m";
            throw_exception(SUB_SEMANTIC, ss.str(), fce.line, file_name, is_debug);
        }
        index++;
    }
    return get_function_return_value(best_candidate, fce);
}

SemanticAnalyzer::Value SemanticAnalyzer::analyze_obj_chain_expr(AST::ChainObjects& co) {
    Value value = analyze_expr(*co.chain[0]);                   // Value of target object
    for (size_t i = 1; i < co.chain.size(); i++) {
        value = analyze_obj_from_chain(value, *co.chain[i]);
    }
    if (value.type.type == AST::TYPE_MODULE) {
        throw_exception(SUB_SEMANTIC, "Cannot specify module as expression", co.line, file_name, is_debug);
    }
    return value;
}

SemanticAnalyzer::Value SemanticAnalyzer::analyze_obj_from_chain(Value target, AST::Expr& obj) {
    if (auto fce = dynamic_cast<AST::FuncCallExpr*>(&obj)) {
        if (target.type.type == AST::TYPE_MODULE) {
            ModuleInfo *info = modules.find(target.type.name)->second;
            if (info == nullptr) {
                std::stringstream ss;
                ss << "Module \033[0m'" << target.type.name << "'\033[31m does not exists";
                throw_exception(SUB_SEMANTIC, ss.str(), fce->line, file_name, is_debug);
            }
            if (info->functions.find(fce->name) == info->functions.end()) {
                std::stringstream ss;
                ss << "Function \033[0m'" << get_mangled_name(fce->name) << '(';
                for (size_t i = 0; i < fce->args.size(); i++) {
                    ss << analyze_expr(*fce->args[i]).type.to_str();
                    if (i < fce->args.size() - 1) {
                        ss << ", ";
                    }
                }
                ss << ")'\033[31m does not exists in module \033[0m'" << target.type.name << "'\033[31m";
                throw_exception(SUB_SEMANTIC, ss.str(), fce->line, file_name, is_debug);
            }
            if (info->functions.at(fce->name).first != AST::ACCESS_PUBLIC) {
                std::stringstream ss;
                ss << "Function \033[0m'" << get_mangled_name(fce->name) << '(';
                for (size_t i = 0; i < fce->args.size(); i++) {
                    ss << analyze_expr(*fce->args[i]).type.to_str();
                    if (i < fce->args.size() - 1) {
                        ss << ", ";
                    }
                }
                ss << ")'\033[31m in module \033[0m'" << target.type.name << "'\033[31m is private member";
                throw_exception(SUB_SEMANTIC, ss.str(), fce->line, file_name, is_debug);
            }
            current_path.push(PathPart{target.type.name, SemanticAnalyzer::PathPart::OBJ_MODULE});
            Value value = analyze_func_call_expr(*fce);
            current_path.pop();
            return value;
        }
    }
    else if (auto ve = dynamic_cast<AST::VarExpr*>(&obj)) {
        ModuleInfo *info = modules.find(target.type.name)->second;
        if (info == nullptr) {
            std::stringstream ss;
            ss << "Module \033[0m'" << target.type.name << "'\033[31m does not exists";
            throw_exception(SUB_SEMANTIC, ss.str(), fce->line, file_name, is_debug);
        }
        if (info->modules.find(ve->name) != info->modules.end()) {
            if (info->modules.at(ve->name).first != AST::ACCESS_PUBLIC) {
                std::stringstream ss;
                ss << "Module \033[0m'" << ve->name << "'\033[31m in module \033[0m'" << target.type.name << "'\033[31m is private member";
                throw_exception(SUB_SEMANTIC, ss.str(), ve->line, file_name, is_debug);
            }
            target.type.name += "-" + ve->name;
            return target;
        }
        std::stringstream ss;
        ss << "Module \033[0m'" << ve->name << "'\033[31m does not exists in module \033[0m'" << target.type.name << "'\033[31m";
        throw_exception(SUB_SEMANTIC, ss.str(), ve->line, file_name, is_debug);
    }
    else {
        std::stringstream ss;
        ss << "Module \033[0m'" << target.type.name << "'\033[31m does not have passed object type";
        throw_exception(SUB_SEMANTIC, ss.str(), fce->line, file_name, is_debug);
    }
}

SemanticAnalyzer::Value SemanticAnalyzer::get_function_return_value(std::shared_ptr<FunctionInfo> func, AST::FuncCallExpr& fce) {
    variables.push({});
    functions_ret_types.push(func->ret_type);
    for (size_t i = 0; i < fce.args.size(); i++) {
        Value val = analyze_expr(*fce.args[i]);
        val.type = func->args[i].type;
        val.is_value_unknown = true;
        val.is_literal = false;
        variables.top().emplace(func->args[i].name, val);
    }
    for (auto& stmt : func->block) {
        analyze_stmt(*stmt);
        if (auto rs = dynamic_cast<AST::ReturnStmt*>(&*stmt)) {
            Value val = analyze_expr(*rs->expr);
            val = implicitly_cast(val, functions_ret_types.top(), rs->line);
            variables.pop();
            functions_ret_types.pop();
            return val;
        }
        else if (auto ies = dynamic_cast<AST::IfElseStmt*>(&*stmt)) {
            Value *val = get_function_return_value_from_if_else(*ies);
            if (val != nullptr) {
                variables.pop();
                functions_ret_types.pop();
                return *val;
            }
        }
        else if (auto wcs = dynamic_cast<AST::WhileCycleStmt*>(&*stmt)) {
            Value *val = get_function_return_value_from_while_cycle(*wcs);
            if (val != nullptr) {
                variables.pop();
                functions_ret_types.pop();
                return *val;
            }
        }
        else if (auto dwcs = dynamic_cast<AST::DoWhileCycleStmt*>(&*stmt)) {
            Value *val = get_function_return_value_from_do_while_cycle(*dwcs);
            if (val != nullptr) {
                variables.pop();
                functions_ret_types.pop();
                return *val;
            }
        }
        else if (auto fcs = dynamic_cast<AST::ForCycleStmt*>(&*stmt)) {
            Value *val = get_function_return_value_from_for_cycle(*fcs);
            if (val != nullptr) {
                variables.pop();
                functions_ret_types.pop();
                return *val;
            }
        }
    }
    std::stringstream ss;
    ss << "Not all paths returns value in function \033[0m'" << fce.name << "'\033[31m. Please add \033[0m'return'\033[31m statement into the end of the function";
    throw_exception(SUB_SEMANTIC, ss.str(), fce.line, file_name, is_debug);
}

SemanticAnalyzer::Value *SemanticAnalyzer::get_function_return_value_from_if_else(AST::IfElseStmt& ies) {
    Value cond_val = analyze_expr(*ies.cond);
    if (cond_val.is_literal && !cond_val.is_value_unknown && std::get<bool>(cond_val.value.value) == true) {
        for (auto& stmt : ies.then_block) {
            analyze_stmt(*stmt);
            if (auto rs = dynamic_cast<AST::ReturnStmt*>(&*stmt)) {
                static Value val = analyze_expr(*rs->expr);
                val = implicitly_cast(val, functions_ret_types.top(), rs->line);
                return &val;
            }
            else if (auto ies = dynamic_cast<AST::IfElseStmt*>(&*stmt)) {
                return get_function_return_value_from_if_else(*ies);
            }
            else if (auto wcs = dynamic_cast<AST::WhileCycleStmt*>(&*stmt)) {
                return get_function_return_value_from_while_cycle(*wcs);
            }
            else if (auto dwcs = dynamic_cast<AST::DoWhileCycleStmt*>(&*stmt)) {
                return get_function_return_value_from_do_while_cycle(*dwcs);
            }
            else if (auto fcs = dynamic_cast<AST::ForCycleStmt*>(&*stmt)) {
                return get_function_return_value_from_for_cycle(*fcs);
            }
        }
    }
    else if (cond_val.is_literal && !cond_val.is_value_unknown && std::get<bool>(cond_val.value.value) == false) {
        for (auto& stmt : ies.else_block) {
            analyze_stmt(*stmt);
            if (auto rs = dynamic_cast<AST::ReturnStmt*>(&*stmt)) {
                static Value val = analyze_expr(*rs->expr);
                val = implicitly_cast(val, functions_ret_types.top(), rs->line);
                return &val;
            }
            else if (auto ies = dynamic_cast<AST::IfElseStmt*>(&*stmt)) {
                return get_function_return_value_from_if_else(*ies);
            }
            else if (auto wcs = dynamic_cast<AST::WhileCycleStmt*>(&*stmt)) {
                return get_function_return_value_from_while_cycle(*wcs);
            }
            else if (auto dwcs = dynamic_cast<AST::DoWhileCycleStmt*>(&*stmt)) {
                return get_function_return_value_from_do_while_cycle(*dwcs);
            }
            else if (auto fcs = dynamic_cast<AST::ForCycleStmt*>(&*stmt)) {
                return get_function_return_value_from_for_cycle(*fcs);
            }
        }
    }
    return nullptr;
}

SemanticAnalyzer::Value *SemanticAnalyzer::get_function_return_value_from_while_cycle(AST::WhileCycleStmt& wcs) {
    Value cond_val = analyze_expr(*wcs.cond);
    if (cond_val.is_literal && !cond_val.is_value_unknown && std::get<bool>(cond_val.value.value) == true) {
        for (auto& stmt : wcs.block) {
            analyze_stmt(*stmt);
            if (auto rs = dynamic_cast<AST::ReturnStmt*>(&*stmt)) {
                static Value val = analyze_expr(*rs->expr);
                val = implicitly_cast(val, functions_ret_types.top(), rs->line);
                return &val;
            }
            else if (auto ies = dynamic_cast<AST::IfElseStmt*>(&*stmt)) {
                return get_function_return_value_from_if_else(*ies);
            }
            else if (auto wcs = dynamic_cast<AST::WhileCycleStmt*>(&*stmt)) {
                return get_function_return_value_from_while_cycle(*wcs);
            }
            else if (auto dwcs = dynamic_cast<AST::DoWhileCycleStmt*>(&*stmt)) {
                return get_function_return_value_from_do_while_cycle(*dwcs);
            }
            else if (auto fcs = dynamic_cast<AST::ForCycleStmt*>(&*stmt)) {
                return get_function_return_value_from_for_cycle(*fcs);
            }
        }
    }
    return nullptr;
}

SemanticAnalyzer::Value *SemanticAnalyzer::get_function_return_value_from_do_while_cycle(AST::DoWhileCycleStmt& dwcs) {
    for (auto& stmt : dwcs.block) {
        analyze_stmt(*stmt);
        if (auto rs = dynamic_cast<AST::ReturnStmt*>(&*stmt)) {
            static Value val = analyze_expr(*rs->expr);
            val = implicitly_cast(val, functions_ret_types.top(), rs->line);
            return &val;
        }
        else if (auto ies = dynamic_cast<AST::IfElseStmt*>(&*stmt)) {
            return get_function_return_value_from_if_else(*ies);
        }
        else if (auto wcs = dynamic_cast<AST::WhileCycleStmt*>(&*stmt)) {
            return get_function_return_value_from_while_cycle(*wcs);
        }
        else if (auto dwcs = dynamic_cast<AST::DoWhileCycleStmt*>(&*stmt)) {
            return get_function_return_value_from_do_while_cycle(*dwcs);
        }
        else if (auto fcs = dynamic_cast<AST::ForCycleStmt*>(&*stmt)) {
            return get_function_return_value_from_for_cycle(*fcs);
        }
    }
    return nullptr;
}

SemanticAnalyzer::Value *SemanticAnalyzer::get_function_return_value_from_for_cycle(AST::ForCycleStmt& fcs) {
    variables.push({});
    analyze_stmt(*fcs.indexator);
    Value cond_val = analyze_expr(*fcs.cond);
    if (cond_val.is_literal && !cond_val.is_value_unknown && std::get<bool>(cond_val.value.value) == true) {
        for (auto& stmt : fcs.block) {
            analyze_stmt(*stmt);
            if (auto rs = dynamic_cast<AST::ReturnStmt*>(&*stmt)) {
                static Value val = analyze_expr(*rs->expr);
                val = implicitly_cast(val, functions_ret_types.top(), rs->line);
                variables.pop();
                return &val;
            }
            else if (auto ies = dynamic_cast<AST::IfElseStmt*>(&*stmt)) {
                variables.pop();
                return get_function_return_value_from_if_else(*ies);
            }
            else if (auto wcs = dynamic_cast<AST::WhileCycleStmt*>(&*stmt)) {
                variables.pop();
                return get_function_return_value_from_while_cycle(*wcs);
            }
            else if (auto dwcs = dynamic_cast<AST::DoWhileCycleStmt*>(&*stmt)) {
                variables.pop();
                return get_function_return_value_from_do_while_cycle(*dwcs);
            }
            else if (auto fcs = dynamic_cast<AST::ForCycleStmt*>(&*stmt)) {
                variables.pop();
                return get_function_return_value_from_for_cycle(*fcs);
            }
        }
    }
    variables.pop();
    return nullptr;
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
            throw_exception(SUB_SEMANTIC, ss.str(), line, file_name, is_debug);
    }
}

std::unique_ptr<SemanticAnalyzer::Value> SemanticAnalyzer::get_variable_value(std::string name) {
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

std::vector<std::shared_ptr<SemanticAnalyzer::FunctionInfo>> SemanticAnalyzer::get_function_candidates(std::string name) {
    auto func_it = functions.find(name);
    if (func_it != functions.end()) {
        return func_it->second;
    }
    return {};
}

bool SemanticAnalyzer::has_common_type(AST::Type left, AST::Type right) {
    if (left.type == right.type) {
        return true;
    }
    if (implicitly_cast_allowed_types.find(left.type) != implicitly_cast_allowed_types.end() &&
        std::find(implicitly_cast_allowed_types[left.type].begin(), implicitly_cast_allowed_types[left.type].end(), right.type) != implicitly_cast_allowed_types[left.type].end()) {
        return true;
    }
    return false;
}

AST::Type SemanticAnalyzer::get_common_type(AST::Type left, AST::Type right, uint32_t line) {
    if (left.type == right.type) {
        return left;
    }

    if (has_common_type(left, right)) {
        return AST::Type(*std::find(implicitly_cast_allowed_types[left.type].begin(), implicitly_cast_allowed_types[left.type].end(), right.type).base(), right.name, right.is_const, right.is_ptr, right.is_nullable);
    }
    if (has_common_type(right, left)) {
        return AST::Type(*std::find(implicitly_cast_allowed_types[right.type].begin(), implicitly_cast_allowed_types[right.type].end(), left.type).base(), left.name, left.is_const, left.is_ptr, left.is_nullable);
    }

    std::stringstream ss;
    ss << "Type mismatch: there is no common type for \033[0m'" << left.to_str() << "'\033[31m and \033[0m'" << right.to_str() << "'\033[31m";
    throw_exception(SUB_SEMANTIC, ss.str(), line, file_name, is_debug);
}

SemanticAnalyzer::Value SemanticAnalyzer::implicitly_cast(Value val, AST::Type type, uint32_t line) {
    AST::Type output_type = get_common_type(val.type, type, line);
    Value res = Value(output_type, 0, val.is_value_unknown, val.is_literal);

    double val_from_variant = 0;
    switch (val.value.value.index()) {
        case 0:
            val_from_variant = std::get<0>(val.value.value);
            break;
        case 1:
            val_from_variant = std::get<1>(val.value.value);
            break;
        case 2:
            val_from_variant = std::get<2>(val.value.value);
            break;
        case 3:
            val_from_variant = std::get<3>(val.value.value);
            break;
        case 4:
            val_from_variant = std::get<4>(val.value.value);
            break;
        case 5:
            val_from_variant = std::get<5>(val.value.value);
            break;
        case 6:
            val_from_variant = std::get<6>(val.value.value);
            break;
    }
    switch (type.type) {
        #define VALUE(type) static_cast<type>(val_from_variant)
        case AST::TYPE_BOOL:
            res.value.value = VALUE(bool);
            break;
        case AST::TYPE_CHAR:
            res.value.value = VALUE(char);
            break;
        case AST::TYPE_SHORT:
            res.value.value = VALUE(short);
            break;
        case AST::TYPE_INT:
            res.value.value = VALUE(int);
            break;
        case AST::TYPE_LONG:
            res.value.value = VALUE(long);
            break;
        case AST::TYPE_FLOAT:
            res.value.value = VALUE(float);
            break;
        case AST::TYPE_DOUBLE:
            res.value.value = VALUE(double);
            break;
        #undef VALUE
    }

    return res;
}

double SemanticAnalyzer::binary_two_variants(Value left, Value right, TokenType op, uint32_t line) {
    if (left.is_value_unknown || right.is_value_unknown) {
        return 0;
    }
    
    double left_val = 0;
    double right_val = 0;
    switch (left.value.value.index()) {
        case 0:
            left_val = std::get<0>(left.value.value);
            break;
        case 1:
            left_val = std::get<1>(left.value.value);
            break;
        case 2:
            left_val = std::get<2>(left.value.value);
            break;
        case 3:
            left_val = std::get<3>(left.value.value);
            break;
        case 4:
            left_val = std::get<4>(left.value.value);
            break;
        case 5:
            left_val = std::get<5>(left.value.value);
            break;
        case 6:
            left_val = std::get<6>(left.value.value);
            break;
    }
    switch (right.value.value.index()) {
        case 0:
            right_val = std::get<0>(right.value.value);
            break;
        case 1:
            right_val = std::get<1>(right.value.value);
            break;
        case 2:
            right_val = std::get<2>(right.value.value);
            break;
        case 3:
            right_val = std::get<3>(right.value.value);
            break;
        case 4:
            right_val = std::get<4>(right.value.value);
            break;
        case 5:
            right_val = std::get<5>(right.value.value);
            break;
        case 6:
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
            if (!right.is_value_unknown && right_val == 0) {
                throw_exception(SUB_SEMANTIC, "Division by zero", line, file_name, is_debug);
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
            throw_exception(SUB_SEMANTIC, ss.str(), line, file_name, is_debug);
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
            throw_exception(SUB_SEMANTIC, ss.str(), line, file_name, is_debug);
    }
}

std::string SemanticAnalyzer::get_mangled_name(std::string base_name) {
    std::string res;
    auto path = current_path;
    while (!path.empty()) {
        PathPart part = path.top();
        if (part.object == PathPart::OBJ_MODULE) {
            res = part.name + "-" + res;
        }
        else {
            res = part.name + "#" + res;
        }
        path.pop();
    }
    return res + base_name;
}

std::vector<SemanticAnalyzer::PathPart> SemanticAnalyzer::get_resolved_name(std::string mangled_name) {
    std::vector<PathPart> res;
    std::string name;
    for (const char c : mangled_name) {
        if (c == '-') {
            res.push_back({name, PathPart::OBJ_MODULE});
            name = "";
        }
        else if (c == '#') {
            res.push_back({name, PathPart::OBJ_CLASS});
            name = "";
        }
        else {
            name += c;
        }
    }
    res.push_back({name, PathPart::OBJ_MODULE});
    return res;
}