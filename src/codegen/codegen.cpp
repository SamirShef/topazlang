/**
 * @file codegen.cpp
 *
 * @brief codegen.hpp implementation
 */

#include "../../include/exception/exception.hpp"
#include "../../include/semantic/semantic.hpp"
#include "../../include/codegen/codegen.hpp"
#include "../../include/parser/parser.hpp"
#include "../../include/lexer/lexer.hpp"
#include <filesystem>
#include <fstream>

void CodeGenerator::generate() {
    for (const AST::StmtPtr& stmt : stmts) {
        generate_stmt(*stmt);
    }
}

void CodeGenerator::generate_stmt(AST::Stmt& stmt) {
    if (auto vds = dynamic_cast<AST::VarDeclStmt*>(&stmt)) {
        generate_var_decl_stmt(*vds);
    }
    else if (auto vas = dynamic_cast<AST::VarAsgnStmt*>(&stmt)) {
        generate_var_asgn_stmt(*vas);
    }
    else if (auto fds = dynamic_cast<AST::FuncDeclStmt*>(&stmt)) {
        generate_func_decl_stmt(*fds);
    }
    else if (auto fcs = dynamic_cast<AST::FuncCallStmt*>(&stmt)) {
        generate_func_call_stmt(*fcs);
    }
    else if (auto rs = dynamic_cast<AST::ReturnStmt*>(&stmt)) {
        generate_return_stmt(*rs);
    }
    else if (auto ies = dynamic_cast<AST::IfElseStmt*>(&stmt)) {
        generate_if_else_stmt(*ies);
    }
    else if (auto wcs = dynamic_cast<AST::WhileCycleStmt*>(&stmt)) {
        generate_while_cycle_stmt(*wcs);
    }
    else if (auto dwcs = dynamic_cast<AST::DoWhileCycleStmt*>(&stmt)) {
        generate_do_while_cycle_stmt(*dwcs);
    }
    else if (auto fcs = dynamic_cast<AST::ForCycleStmt*>(&stmt)) {
        generate_for_cycle_stmt(*fcs);
    }
    else if (auto bs = dynamic_cast<AST::BreakStmt*>(&stmt)) {
        generate_break_stmt(*bs);
    }
    else if (auto cs = dynamic_cast<AST::ContinueStmt*>(&stmt)) {
        generate_continue_stmt(*cs);
    }
    else if (auto ms = dynamic_cast<AST::ModuleStmt*>(&stmt)) {
        generate_module_stmt(*ms);
    }
    else if (auto ums = dynamic_cast<AST::UseModuleStmt*>(&stmt)) {
        generate_use_module_stmt(*ums);
    }
    else if (auto us = dynamic_cast<AST::UnsafeStmt*>(&stmt)) {
        generate_unsafe_stmt(*us);
    }
    else {
        throw_exception(SUB_CODEGEN, "Unsupported statement. Please check your Topaz compiler version and fix the problematic section of the code", stmt.line, file_name, is_debug);
    }
}

void CodeGenerator::generate_var_decl_stmt(AST::VarDeclStmt& vds) {
    llvm::Type *type = type_to_llvm(vds.type);
    llvm::Value *val = llvm::Constant::getNullValue(type);
    if (vds.expr != nullptr) {
        val = generate_expr(*vds.expr);
    }
    if (val->getType() != type) {
        val = implicitly_cast(val, type);
    }
    llvm::Value *var = nullptr;
    if (variables.size() == 1) {
        var = new llvm::GlobalVariable(*module, type, vds.type.is_const, llvm::GlobalValue::ExternalLinkage, llvm::dyn_cast<llvm::Constant>(val), vds.name);
    }
    else {
        var = builder.CreateAlloca(type, nullptr, vds.name + ".alloca");
        builder.CreateStore(val, var);
    }
    variables.top().emplace(vds.name, var);
}

void CodeGenerator::generate_var_asgn_stmt(AST::VarAsgnStmt& vas) {
    llvm::Value *var_inst = nullptr;
    auto vars = variables;
    while (!vars.empty()) {
        auto vars_it = vars.top().find(vas.name);
        if (vars_it != vars.top().end()) {
            var_inst = vars_it->second;
        }
        vars.pop();
    }
    if (var_inst == nullptr) {
        std::stringstream ss;
        ss << "Variable \033[0m'" << vas.name << "'\033[31m does not exists";
        throw_exception(SUB_CODEGEN, ss.str(), vas.line, file_name, is_debug);
    }
    builder.CreateStore(generate_expr(*vas.expr), var_inst);
}

void CodeGenerator::generate_func_decl_stmt(AST::FuncDeclStmt& fds) {
    llvm::Type *ret_type = type_to_llvm(fds.ret_type);
    std::vector<llvm::Type*> args;
    std::string func_name = get_mangled_name(fds.name);
    size_t args_count = fds.args.size();
    for (size_t i = 0; i < args_count; i++) {
        args.push_back(type_to_llvm(fds.args[i].type));
        switch (fds.args[i].type.type) {
            case AST::TYPE_BOOL:
                func_name += ".bool";
                break;
            case AST::TYPE_CHAR:
                func_name += ".char";
                break;
            case AST::TYPE_SHORT:
                func_name += ".short";
                break;
            case AST::TYPE_INT:
                func_name += ".int";
                break;
            case AST::TYPE_LONG:
                func_name += ".long";
                break;
            case AST::TYPE_FLOAT:
                func_name += ".float";
                break;
            case AST::TYPE_DOUBLE:
                func_name += ".double";
                break;
            case AST::TYPE_TRAIT:
                func_name += ".T_" + fds.args[i].type.name;
                break;
            case AST::TYPE_CLASS:
                func_name += ".C_" + fds.args[i].type.name;
                break;
        }
        if (fds.args[i].type.is_const) {
            func_name += "_const";
        }
        if (fds.args[i].type.is_ptr) {
            func_name += "_ptr";
        }
    }
    llvm::FunctionType *func_type = llvm::FunctionType::get(ret_type, args, false);
    llvm::Function *func = llvm::Function::Create(func_type, llvm::GlobalValue::ExternalLinkage, func_name, *module);
    
    llvm::BasicBlock *entry = llvm::BasicBlock::Create(context, "entry", func);
    builder.SetInsertPoint(entry);
    
    variables.push({});
    if (functions.find(get_mangled_name(fds.name)) == functions.end()) {
        functions.emplace(get_mangled_name(fds.name), std::vector<llvm::Function*>{func});
    }
    else {
        functions.at(get_mangled_name(fds.name)).push_back(func);
    }
    functions_ret_types.push(ret_type);
    size_t index = 0;
    for (llvm::Argument& arg : func->args()) {
        arg.setName(fds.args[index].name);
        llvm::AllocaInst *arg_alloca = builder.CreateAlloca(arg.getType(), nullptr, fds.args[index].name);
        builder.CreateStore(&arg, arg_alloca);
        variables.top().emplace(fds.args[index].name, arg_alloca);
        index++;
    }
    bool have_ret_in_global = false;
    for (auto& stmt : fds.block) {
        if (auto rs = dynamic_cast<AST::ReturnStmt*>(&*stmt)) {
            have_ret_in_global = true;
        }
        generate_stmt(*stmt);
    }
    if (!have_ret_in_global) {
        if (fds.ret_type.type == AST::TYPE_NOTH) {
            builder.CreateRetVoid();
        }
        else if (fds.ret_type.type <= AST::TYPE_DOUBLE) {
            builder.CreateRet(llvm::Constant::getNullValue(type_to_llvm(fds.ret_type)));
        }
        else {
            throw_exception(SUB_CODEGEN, "Not all paths return a value", fds.line, file_name, is_debug);
        }
    }
    variables.pop();
    functions_ret_types.pop();
}

void CodeGenerator::generate_func_call_stmt(AST::FuncCallStmt& fcs) {
    std::vector<llvm::Function*> function_candidates = functions.at(get_mangled_name(fcs.name));
    std::vector<llvm::Value*> args;
    for (auto& arg : fcs.args) {
        args.push_back(generate_expr(*arg));
    }

    size_t last_score = SIZE_MAX;
    size_t best_candidate_index = 0;
    size_t candidate_index = 0;
    for (auto& candidate : function_candidates) {
        auto candidate_args_it = candidate->args();
        size_t score = 0;
        size_t index = 0;
        for (auto& arg : candidate_args_it) {
            llvm::Type *candidate_arg_type = arg.getType();
            llvm::Type *calling_arg_type = generate_expr(*fcs.args[index])->getType();

            if (candidate_arg_type != calling_arg_type) {
                if (candidate_arg_type->isIntegerTy() && calling_arg_type->isIntegerTy()) {
                    score++;
                }
                else if (candidate_arg_type->isFloatingPointTy() && calling_arg_type->isFloatingPointTy()) {
                    score++;
                }
                else if (candidate_arg_type->isFloatingPointTy() && calling_arg_type->isIntegerTy()) {
                    score += 2;
                }
                else {
                    score += 99;
                }
            }

            index++;
        }

        if (score <= last_score) {
            best_candidate_index = candidate_index;
            last_score = score;
        }

        candidate_index++;
    }

    builder.CreateCall(function_candidates[best_candidate_index], args, function_candidates[best_candidate_index]->getName() + ".call");
}

void CodeGenerator::generate_return_stmt(AST::ReturnStmt& rs) {
    if (rs.expr != nullptr) {
        llvm::Value *val = generate_expr(*rs.expr);
        if (val->getType() != functions_ret_types.top()) {
            val = implicitly_cast(val, functions_ret_types.top());
        }
        builder.CreateRet(val);
    }
    else {
        builder.CreateRetVoid();
    }
}

void CodeGenerator::generate_if_else_stmt(AST::IfElseStmt& ies) {
    llvm::Function *parent = builder.GetInsertBlock()->getParent();
    llvm::Value *cond_val = generate_expr(*ies.cond);
    llvm::BasicBlock *then_bb = llvm::BasicBlock::Create(context, "if.then", parent);
    llvm::BasicBlock *else_bb = llvm::BasicBlock::Create(context, "if.else", parent);
    llvm::BasicBlock *merge_bb = llvm::BasicBlock::Create(context, "if.merge", parent);

    builder.CreateCondBr(cond_val, then_bb, else_bb ? else_bb : merge_bb);

    builder.SetInsertPoint(then_bb);
    variables.push({});
    for (auto& stmt : ies.then_block) {
        generate_stmt(*stmt);
    }
    variables.pop();

    if (builder.GetInsertBlock()->getTerminator() == nullptr) {
        builder.CreateBr(merge_bb);
    }

    builder.SetInsertPoint(else_bb);
    variables.push({});
    for (auto& stmt : ies.else_block) {
        generate_stmt(*stmt);
    }
    variables.pop();
    
    if (builder.GetInsertBlock()->getTerminator() == nullptr) {
        builder.CreateBr(merge_bb);
    }
    builder.SetInsertPoint(merge_bb);
}

void CodeGenerator::generate_while_cycle_stmt(AST::WhileCycleStmt& wcs) {
    llvm::Function *parent = builder.GetInsertBlock()->getParent();
    llvm::BasicBlock *cond_bb = llvm::BasicBlock::Create(context, "while.cond", parent);
    llvm::BasicBlock *body_bb = llvm::BasicBlock::Create(context, "while.body", parent);
    llvm::BasicBlock *exit_bb = llvm::BasicBlock::Create(context, "while.exit", parent);
    
    builder.CreateBr(cond_bb);
    builder.SetInsertPoint(cond_bb);
    llvm::Value *cond_value = generate_expr(*wcs.cond);
    
    builder.CreateCondBr(cond_value, body_bb, exit_bb);
    builder.SetInsertPoint(body_bb);
    variables.push({});
    loop_blocks.emplace(exit_bb, cond_bb);
    for (auto& stmt : wcs.block) {
        generate_stmt(*stmt);
    }
    loop_blocks.pop();
    variables.pop();

    builder.CreateBr(cond_bb);
    builder.SetInsertPoint(exit_bb);
}

void CodeGenerator::generate_do_while_cycle_stmt(AST::DoWhileCycleStmt& dwcs) {
    llvm::Function *parent = builder.GetInsertBlock()->getParent();
    llvm::BasicBlock *cond_bb = llvm::BasicBlock::Create(context, "do.while.cond", parent);
    llvm::BasicBlock *body_bb = llvm::BasicBlock::Create(context, "do.while.body", parent);
    llvm::BasicBlock *exit_bb = llvm::BasicBlock::Create(context, "do.while.exit", parent);

    builder.CreateBr(body_bb);
    builder.SetInsertPoint(body_bb);
    variables.push({});
    loop_blocks.emplace(exit_bb, cond_bb);
    for (auto& stmt : dwcs.block) {
        generate_stmt(*stmt);
    }
    loop_blocks.pop();
    variables.pop();

    builder.CreateBr(cond_bb);
    builder.SetInsertPoint(cond_bb);
    llvm::Value *cond_value = generate_expr(*dwcs.cond);
    builder.CreateCondBr(cond_value, body_bb, exit_bb);

    builder.SetInsertPoint(exit_bb);
}

void CodeGenerator::generate_for_cycle_stmt(AST::ForCycleStmt& fcs) {
    llvm::Function *parent = builder.GetInsertBlock()->getParent();
    llvm::BasicBlock *indexator_bb = llvm::BasicBlock::Create(context, "for.indexator", parent);
    llvm::BasicBlock *cond_bb = llvm::BasicBlock::Create(context, "for.cond", parent);
    llvm::BasicBlock *iteration_bb = llvm::BasicBlock::Create(context, "for.iteration", parent);
    llvm::BasicBlock *body_bb = llvm::BasicBlock::Create(context, "for.body", parent);
    llvm::BasicBlock *exit_bb = llvm::BasicBlock::Create(context, "for.exit", parent);

    builder.CreateBr(indexator_bb);
    builder.SetInsertPoint(indexator_bb);
    generate_stmt(*fcs.indexator);

    builder.CreateBr(cond_bb);
    builder.SetInsertPoint(cond_bb);
    llvm::Value *condition_value = generate_expr(*fcs.cond);

    builder.CreateCondBr(condition_value, body_bb, exit_bb);
    builder.SetInsertPoint(body_bb);
    variables.push({});
    loop_blocks.emplace(exit_bb, iteration_bb);
    for (auto& stmt : fcs.block) {
        generate_stmt(*stmt);
    }
    loop_blocks.pop();
    variables.pop();

    builder.CreateBr(iteration_bb);
    builder.SetInsertPoint(iteration_bb);
    generate_stmt(*fcs.iteration);

    builder.CreateBr(cond_bb);
    builder.SetInsertPoint(exit_bb);
}

void CodeGenerator::generate_break_stmt(AST::BreakStmt& bs) {
    builder.CreateBr(loop_blocks.top().first);
}

void CodeGenerator::generate_continue_stmt(AST::ContinueStmt& cs) {
    builder.CreateBr(loop_blocks.top().second);
}

void CodeGenerator::generate_module_stmt(AST::ModuleStmt& ms) {
    current_path.push(PathPart{.name=ms.name, .object=PathPart::OBJ_MODULE});

    for (auto& stmt : ms.block) {
        generate_stmt(*stmt);
    }

    current_path.pop();
}

void CodeGenerator::generate_use_module_stmt(AST::UseModuleStmt& ums) {
    std::string all_name;
    for (size_t i = 0; i < ums.path.size(); i++) {
        all_name += ums.path[i];
        if (i != ums.path.size() - 1) {
            all_name += "-";
        }
    }
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
            std::ostringstream content;
            content << file.rdbuf();
            file.close();
            Lexer lex(content.str(), path_to_mod_without_ext_in_libs_as_str + "/main.tp", is_debug);
            std::vector<Token> tokens = lex.tokenize();
            Parser parser(tokens, is_debug);
            std::vector<AST::StmtPtr> stmts = parser.parse();
            SemanticAnalyzer semantic(stmts, libs_path, path_to_mod_without_ext_in_libs_as_str + "/main.tp", is_debug);
            semantic.analyze();
            std::map<std::string, SemanticAnalyzer::ModuleInfo*> modules = semantic.get_modules();
            size_t current_path_size = current_path.size();
            for (auto& module : modules) {
                std::vector<PathPart> resolved_name = get_resolved_name(module.first);
                current_path.push({resolved_name.back().name, PathPart::OBJ_MODULE});
                auto functions = semantic.get_functions();
                for (auto& func : module.second->functions) {
                    auto candidates_at_functions = functions.at(get_mangled_name(func.first));
                    for (auto& candidate : candidates_at_functions) {
                        generate_func_decl_stmt(*std::make_unique<AST::FuncDeclStmt>(func.second.first, func.first,
                                                std::move(candidate->args), candidate->ret_type, std::move(candidate->block), -1));
                    }
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
        std::map<std::string, SemanticAnalyzer::ModuleInfo*> modules = semantic.get_modules();
        size_t current_path_size = current_path.size();
        for (auto& module : modules) {
            std::vector<PathPart> resolved_name = get_resolved_name(module.first);
            current_path.push({resolved_name.back().name, PathPart::OBJ_MODULE});
            auto functions = semantic.get_functions();
            for (auto& func : module.second->functions) {
                auto candidates_at_functions = functions.at(get_mangled_name(func.first));
                for (auto& candidate : candidates_at_functions) {
                    generate_func_decl_stmt(*std::make_unique<AST::FuncDeclStmt>(func.second.first, func.first,
                                            std::move(candidate->args), candidate->ret_type, std::move(candidate->block), -1));
                }
            }
        }
        for (size_t i = current_path.size() - current_path_size; i > 0; i--) {
            current_path.pop();
        }
    }
}

void CodeGenerator::generate_unsafe_stmt(AST::UnsafeStmt& us) {
    for (auto& stmt : us.block) {
        generate_stmt(*stmt);
    }
}

llvm::Value *CodeGenerator::generate_expr(AST::Expr& expr) {
    if (auto lit = dynamic_cast<AST::Literal*>(&expr)) {
        return generate_literal_expr(*lit);
    }
    else if (auto be = dynamic_cast<AST::BinaryExpr*>(&expr)) {
        return generate_binary_expr(*be);
    }
    else if (auto ue = dynamic_cast<AST::UnaryExpr*>(&expr)) {
        return generate_unary_expr(*ue);
    }
    else if (auto ve = dynamic_cast<AST::VarExpr*>(&expr)) {
        return generate_var_expr(*ve);
    }
    else if (auto fce = dynamic_cast<AST::FuncCallExpr*>(&expr)) {
        return generate_func_call_expr(*fce);
    }
    else if (auto oce = dynamic_cast<AST::ChainObjects*>(&expr)) {
        return generate_obj_chain_expr(*oce);
    }
    else {
        throw_exception(SUB_CODEGEN, "An unsupported expression was encountered during compilation. Please check your Topaz compiler version and fix the problematic section of the code", expr.line, file_name, is_debug);
    }
}

llvm::Value *CodeGenerator::generate_literal_expr(AST::Literal& lit) {
    auto& value = lit.value.value;

    switch (lit.type.type) {
        case AST::TYPE_CHAR:
            return llvm::ConstantInt::get(type_to_llvm(lit.type), llvm::APInt(8, std::get<char8_t>(value)));
        case AST::TYPE_SHORT:
            return llvm::ConstantInt::get(type_to_llvm(lit.type), llvm::APInt(16, std::get<int16_t>(value)));
        case AST::TYPE_INT:
            return llvm::ConstantInt::get(type_to_llvm(lit.type), llvm::APInt(32, std::get<int32_t>(value)));
        case AST::TYPE_LONG:
            return llvm::ConstantInt::get(type_to_llvm(lit.type), llvm::APInt(64, std::get<int64_t>(value)));
        case AST::TYPE_FLOAT:
            return llvm::ConstantFP::get(type_to_llvm(lit.type), llvm::APFloat(std::get<float_t>(value)));
        case AST::TYPE_DOUBLE:
            return llvm::ConstantFP::get(type_to_llvm(lit.type), llvm::APFloat(std::get<double_t>(value)));
        case AST::TYPE_BOOL:
            return llvm::ConstantInt::get(type_to_llvm(lit.type), llvm::APInt(1, std::get<bool>(value)));
        case AST::TYPE_STRING_LIT: {
                llvm::Constant *str_const = llvm::ConstantDataArray::getString(context, std::get<std::string>(value), true);
                llvm::GlobalVariable *str_var = new llvm::GlobalVariable(*module, str_const->getType(), true, llvm::GlobalValue::PrivateLinkage, str_const, "string.lit");
                return str_var;
            }
        default:
            throw_exception(SUB_CODEGEN, "An unsupported literal type was encountered during compilation. Please check your Topaz compiler version and fix the problematic section of the code", lit.line, file_name, is_debug);
    }
}

llvm::Value *CodeGenerator::generate_binary_expr(AST::BinaryExpr& be) {
    llvm::Value *left = generate_expr(*be.left_expr);
    llvm::Type *left_type = left->getType();
    llvm::Value *right = generate_expr(*be.right_expr);
    llvm::Type *right_type = right->getType();
    
    llvm::Type *common_type = get_common_type(left_type, right_type);
    if (left_type != common_type) {
        left = implicitly_cast(left, common_type);
        left_type = left->getType();
    }
    else if (right_type != common_type) {
        right = implicitly_cast(right, common_type);
        right_type = right->getType();
    }
    switch (be.op.type) {
        case TOK_OP_PLUS:
            if (left_type->isFloatingPointTy() || right_type->isFloatingPointTy()) {
                return builder.CreateFAdd(left, right, "fadd.tmp");
            }
            return builder.CreateAdd(left, right, "add.tmp");
        case TOK_OP_MINUS:
            if (left_type->isFloatingPointTy() || right_type->isFloatingPointTy()) {
                return builder.CreateFSub(left, right, "fsub.tmp");
            }
            return builder.CreateSub(left, right, "sub.tmp");
        case TOK_OP_MULT:
            if (left_type->isFloatingPointTy() || right_type->isFloatingPointTy()) {
                return builder.CreateFMul(left, right, "fmul.tmp");
            }
            return builder.CreateMul(left, right, "mul.tmp");
        case TOK_OP_DIV:
            if (left_type->isFloatingPointTy() || right_type->isFloatingPointTy()) {
                return builder.CreateFDiv(left, right, "fdiv.tmp");
            }
            return builder.CreateSDiv(left, right, "div.tmp");
        case TOK_OP_MODULO:
            if (left_type->isFloatingPointTy() || right_type->isFloatingPointTy()) {
                return builder.CreateFRem(left, right, "frem.tmp");
            }
            return builder.CreateSRem(left, right, "rem.tmp");
        case TOK_OP_EQ_EQ:
            if (left_type->isFloatingPointTy() || right_type->isFloatingPointTy()) {
                return builder.CreateFCmpUEQ(left, right, "feq.tmp");
            }
            return builder.CreateICmpEQ(left, right, "eq.tmp");
        case TOK_OP_NOT_EQ_EQ:
            if (left_type->isFloatingPointTy() || right_type->isFloatingPointTy()) {
                return builder.CreateNeg(builder.CreateFCmpUEQ(left, right, "feq.tmp"), "fnoteq.tmp");
            }
            return builder.CreateNeg(builder.CreateICmpEQ(left, right, "eq.tmp"), "noteq.tmp");
        case TOK_OP_GT:
            if (left_type->isFloatingPointTy() || right_type->isFloatingPointTy()) {
                return builder.CreateFCmpUGT(left, right, "fgt.tmp");
            }
            return builder.CreateICmpSGT(left, right, "gt.tmp");
        case TOK_OP_GT_EQ:
            if (left_type->isFloatingPointTy() || right_type->isFloatingPointTy()) {
                return builder.CreateFCmpUGE(left, right, "fge.tmp");
            }
            return builder.CreateICmpSGE(left, right, "ge.tmp");
        case TOK_OP_LS:
            if (left_type->isFloatingPointTy() || right_type->isFloatingPointTy()) {
                return builder.CreateFCmpULT(left, right, "flt.tmp");
            }
            return builder.CreateICmpSLT(left, right, "lt.tmp");
        case TOK_OP_LS_EQ:
            if (left_type->isFloatingPointTy() || right_type->isFloatingPointTy()) {
                return builder.CreateFCmpULE(left, right, "fle.tmp");
            }
            return builder.CreateICmpSLE(left, right, "le.tmp");
        case TOK_OP_L_AND:
            return builder.CreateLogicalAnd(left, right, "land.tmp");
        case TOK_OP_L_OR:
            return builder.CreateLogicalAnd(left, right, "lor.tmp");
        default:
            throw_exception(SUB_CODEGEN, "An unsupported binary operator was encountered during compilation. Please check your Topaz compiler version and fix the problematic section of the code", be.line, file_name, is_debug);
    }
}

llvm::Value *CodeGenerator::generate_unary_expr(AST::UnaryExpr& ue) {
    llvm::Value *raw_value = nullptr;
    auto vars = variables;
    if (auto ve = dynamic_cast<AST::VarExpr*>(&*ue.expr)) {
        while (!vars.empty()) {
            auto vars_it = vars.top().find(ve->name);
            if (vars_it != vars.top().end()) {
                raw_value = vars_it->second;
                break;
            }
            vars.pop();
        }
    }
    llvm::Value *value = generate_expr(*ue.expr);
    
    switch (ue.op.type) {
        case TOK_OP_MINUS:
            if (value->getType()->isFloatingPointTy()) {
                return builder.CreateFNeg(value, "neg.tmp");
            }
            return builder.CreateNeg(value, "neg.tmp");
        case TOK_OP_L_NOT:
            if (value->getType()->isFloatingPointTy()) {
                return builder.CreateFCmpOEQ(value, builder.getInt32(0), "lnot.tmp");
            }
            return builder.CreateICmpEQ(value, builder.getInt32(0), "lnot.tmp");
        case TOK_OP_MULT:
            return builder.CreateLoad(value->getType(), value);
        case TOK_OP_REF: {
            return raw_value;
        }
        default:
            throw_exception(SUB_CODEGEN, "An unsupported unary operator was encountered during compilation. Please check your Topaz compiler version and fix the problematic section of the code", ue.line, file_name, is_debug);
    }
}

llvm::Value *CodeGenerator::generate_var_expr(AST::VarExpr& ve) {
    auto vars = variables;
    while (!vars.empty()) {
        auto vars_it = vars.top().find(ve.name);
        if (vars_it != vars.top().end()) {
            llvm::Type *type = nullptr;
            if (auto global = llvm::dyn_cast<llvm::GlobalVariable>(vars_it->second)) {
                type = global->getValueType();
                if (functions_ret_types.empty()) {
                    return global->getInitializer();
                }
            }
            else if (auto local = llvm::dyn_cast<llvm::AllocaInst>(vars_it->second)) {
                type = local->getAllocatedType();
            }
            return builder.CreateLoad(type, vars_it->second, ve.name + ".load");
        }
        vars.pop();
    }
    std::stringstream ss;
    ss << "Variable \033[0m'" << ve.name << "'\033[31m does not exists";
    throw_exception(SUB_CODEGEN, ss.str(), ve.line, file_name, is_debug);
}

llvm::Value *CodeGenerator::generate_func_call_expr(AST::FuncCallExpr& fce) {
    auto function_candidates = functions.at(get_mangled_name(fce.name));
    std::vector<llvm::Value*> args;
    for (auto& arg : fce.args) {
        args.push_back(generate_expr(*arg));
    }

    size_t last_score = SIZE_MAX;
    size_t best_candidate_index = 0;
    size_t candidate_index = 0;
    for (auto& candidate : function_candidates) {
        auto candidate_args_it = candidate->args();
        size_t score = 0;
        size_t index = 0;
        for (auto& arg : candidate_args_it) {
            llvm::Type *candidate_arg_type = arg.getType();
            llvm::Type *calling_arg_type = generate_expr(*fce.args[index])->getType();

            if (candidate_arg_type != calling_arg_type) {
                if (candidate_arg_type->isIntegerTy() && calling_arg_type->isIntegerTy()) {
                    score++;
                }
                else if (candidate_arg_type->isFloatingPointTy() && calling_arg_type->isFloatingPointTy()) {
                    score++;
                }
                else if (candidate_arg_type->isFloatingPointTy() && calling_arg_type->isIntegerTy()) {
                    score += 2;
                }
                else {
                    score += 99;
                }
            }

            index++;
        }

        if (score <= last_score) {
            best_candidate_index = candidate_index;
            last_score = score;
        }

        candidate_index++;
    }

    return builder.CreateCall(function_candidates[best_candidate_index], args, function_candidates[best_candidate_index]->getName() + ".call");
}

llvm::Value *CodeGenerator::generate_obj_chain_expr(AST::ChainObjects& co) {
    llvm::Value *value = nullptr;                               // Value of target object
    size_t current_path_size = current_path.size();
    for (size_t i = 0; i < co.chain.size(); i++) {
        if (auto ve = dynamic_cast<AST::VarExpr*>(&*co.chain[i])) {
            current_path.push({ve->name, PathPart::OBJ_MODULE});
        }
        else if (auto fce = dynamic_cast<AST::FuncCallExpr*>(&*co.chain[i])) {
            value = generate_func_call_expr(*fce);
        }
    }
    for (size_t i = current_path.size() - current_path_size; i > 0; i--) {
        current_path.pop();
    }
    return value;
}

llvm::Type *CodeGenerator::type_to_llvm(AST::Type type) {
    llvm::Type *base_type = nullptr;
    switch (type.type) {
        case AST::TYPE_CHAR:
            base_type = llvm::Type::getInt8Ty(context);
            break;
        case AST::TYPE_SHORT:
            base_type = llvm::Type::getInt16Ty(context);
            break;
        case AST::TYPE_INT:
            base_type = llvm::Type::getInt32Ty(context);
            break;
        case AST::TYPE_LONG:
            base_type = llvm::Type::getInt64Ty(context);
            break;
        case AST::TYPE_FLOAT:
            base_type = llvm::Type::getFloatTy(context);
            break;
        case AST::TYPE_DOUBLE:
            base_type = llvm::Type::getDoubleTy(context);
            break;
        case AST::TYPE_BOOL:
            base_type = llvm::Type::getInt1Ty(context);
            break;
        case AST::TYPE_NOTH:
            base_type = llvm::Type::getVoidTy(context);
            break;
        default:
            throw_exception(SUB_CODEGEN, "Unsupported type", -1, file_name, is_debug);
        }
    if (type.is_ptr) {
        return llvm::PointerType::get(context, 0);
    }
    return base_type;
}

llvm::Type *CodeGenerator::get_common_type(llvm::Type *left, llvm::Type *right) {
    if (left == right) {
        return left;
    }
    else if (left->isIntegerTy() || right->isIntegerTy()) {
        if (left->isIntegerTy() && right->isIntegerTy()) {
            unsigned left_width = left->getIntegerBitWidth();
            unsigned right_width = right->getIntegerBitWidth();

            return left_width > right_width ? left : right;
        }
        else if (left->isFloatingPointTy() || right->isFloatingPointTy()) {
            return left->isFloatingPointTy() ? left : right;
        }
    }
    else if (left->isFloatingPointTy() && right->isFloatingPointTy()) {
        return left->isDoubleTy() && right->isFloatTy() ? left : right;
    }
    else if (left->isDoubleTy() || right->isDoubleTy()) {
        return llvm::Type::getDoubleTy(context);
    }
    return nullptr;
}

llvm::Value *CodeGenerator::implicitly_cast(llvm::Value *val, llvm::Type *expected_type) {
    llvm::Type *val_type = val->getType();
    if (val_type == expected_type) {
        return val;
    }
    else if (val_type->isIntegerTy() && expected_type->isIntegerTy()) {
        unsigned long value_width = val_type->getIntegerBitWidth();
        unsigned long expected_width = expected_type->getIntegerBitWidth();

        if (value_width > expected_width) {
            return builder.CreateTrunc(val, expected_type, "trunc.tmp");
        }
        else {
            return builder.CreateSExt(val, expected_type, "sext.tmp");
        }
    }
    else if (val_type->isFloatingPointTy() && expected_type->isFloatingPointTy()) {
        if (val_type->isFloatTy() && expected_type->isDoubleTy()) {
            return builder.CreateFPExt(val, expected_type, "fpext.tmp");
        }
        else {
            return builder.CreateFPTrunc(val, expected_type, "fptrunc.tmp");
        }
    }
    else if (val_type->isIntegerTy() && expected_type->isFloatingPointTy()) {
        return builder.CreateSIToFP(val, expected_type, "sitofp.tmp");
    }
    else if (val_type->isPointerTy()) {
        return builder.CreatePointerCast(val, expected_type, "ptrcast.tmp");
    }
    return nullptr;
}

std::string CodeGenerator::get_mangled_name(std::string base_name) {
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

std::vector<CodeGenerator::PathPart> CodeGenerator::get_resolved_name(std::string mangled_name) {
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