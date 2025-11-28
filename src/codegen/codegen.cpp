/**
 * @file codegen.cpp
 *
 * @brief codegen.hpp implementation
 */

#include "../../include/exception/exception.hpp"
#include "../../include/codegen/codegen.hpp"

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
    else {
        throw_exception(SUB_CODEGEN, "Unsupported statement. Please check your Topaz compiler version and fix the problematic section of the code", stmt.line, file_name);
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
        throw_exception(SUB_CODEGEN, ss.str(), vas.line, file_name);
    }
    builder.CreateStore(generate_expr(*vas.expr), var_inst);
}

void CodeGenerator::generate_func_decl_stmt(AST::FuncDeclStmt& fds) {
    llvm::Type *ret_type = type_to_llvm(fds.ret_type);
    std::vector<llvm::Type*> args;
    size_t args_count = fds.args.size();
    for (size_t i = 0; i < args_count; i++) {
        args.push_back(type_to_llvm(fds.args[i].type));
    }
    llvm::FunctionType *func_type = llvm::FunctionType::get(ret_type, args, false);
    llvm::Function *func = llvm::Function::Create(func_type, llvm::GlobalValue::ExternalLinkage, fds.name, *module);
    
    llvm::BasicBlock *entry = llvm::BasicBlock::Create(context, "entry", func);
    builder.SetInsertPoint(entry);
    
    variables.push({});
    functions.emplace(fds.name, func);
    functions_ret_types.push(ret_type);
    size_t index = 0;
    for (llvm::Argument& arg : func->args()) {
        arg.setName(fds.args[index].name);
        llvm::AllocaInst* arg_alloca = builder.CreateAlloca(arg.getType(), nullptr, fds.args[index].name);
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
    if (!have_ret_in_global && fds.ret_type.type == AST::TYPE_NOTH) {
        builder.CreateRetVoid();
    }
    variables.pop();
    functions_ret_types.pop();
}

void CodeGenerator::generate_func_call_stmt(AST::FuncCallStmt& fcs) {
    llvm::Function *func = functions.at(fcs.name);
    std::vector<llvm::Value*> args;
    for (auto& arg : fcs.args) {
        args.push_back(generate_expr(*arg));
    }

    builder.CreateCall(func, args, fcs.name + ".call");
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
    llvm::Function* parent = builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* indexator_bb = llvm::BasicBlock::Create(context, "for.indexator", parent);
    llvm::BasicBlock* cond_bb = llvm::BasicBlock::Create(context, "for.cond", parent);
    llvm::BasicBlock* iteration_bb = llvm::BasicBlock::Create(context, "for.iteration", parent);
    llvm::BasicBlock* body_bb = llvm::BasicBlock::Create(context, "for.body", parent);
    llvm::BasicBlock* exit_bb = llvm::BasicBlock::Create(context, "for.exit", parent);

    builder.CreateBr(indexator_bb);
    builder.SetInsertPoint(indexator_bb);
    generate_stmt(*fcs.indexator);

    builder.CreateBr(cond_bb);
    builder.SetInsertPoint(cond_bb);
    llvm::Value* condition_value = generate_expr(*fcs.cond);

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
    else {
        throw_exception(SUB_CODEGEN, "An unsupported expression was encountered during compilation. Please check your Topaz compiler version and fix the problematic section of the code", expr.line, file_name);
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
            throw_exception(SUB_CODEGEN, "An unsupported literal type was encountered during compilation. Please check your Topaz compiler version and fix the problematic section of the code", lit.line, file_name);
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
            throw_exception(SUB_CODEGEN, "An unsupported binary operator was encountered during compilation. Please check your Topaz compiler version and fix the problematic section of the code", be.line, file_name);
    }
}

llvm::Value *CodeGenerator::generate_unary_expr(AST::UnaryExpr& ue) {
    llvm::Value* value = generate_expr(*ue.expr);
    
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
        default:
            throw_exception(SUB_CODEGEN, "An unsupported unary operator was encountered during compilation. Please check your Topaz compiler version and fix the problematic section of the code", ue.line, file_name);
    }
}

llvm::Value *CodeGenerator::generate_var_expr(AST::VarExpr& ve) {
    auto vars = variables;
    while (!vars.empty()) {
        auto vars_it = vars.top().find(ve.name);
        if (vars_it != vars.top().end()) {
            llvm::Type* type = nullptr;
            if (auto global = llvm::dyn_cast<llvm::GlobalVariable>(vars_it->second)) {
                type = global->getValueType();
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
    throw_exception(SUB_CODEGEN, ss.str(), ve.line, file_name);
}

llvm::Value *CodeGenerator::generate_func_call_expr(AST::FuncCallExpr& fce) {
    llvm::Function *func = functions.at(fce.name);
    std::vector<llvm::Value*> args;
    for (auto& arg : fce.args) {
        args.push_back(generate_expr(*arg));
    }

    return builder.CreateCall(func, args, fce.name + ".call");
}

llvm::Type *CodeGenerator::type_to_llvm(AST::Type type) {
    switch (type.type) {
        case AST::TYPE_CHAR:
            return llvm::Type::getInt8Ty(context);
        case AST::TYPE_SHORT:
            return llvm::Type::getInt16Ty(context);
        case AST::TYPE_INT:
            return llvm::Type::getInt32Ty(context);
        case AST::TYPE_LONG:
            return llvm::Type::getInt64Ty(context);
        case AST::TYPE_FLOAT:
            return llvm::Type::getFloatTy(context);
        case AST::TYPE_DOUBLE:
            return llvm::Type::getDoubleTy(context);
        case AST::TYPE_BOOL:
            return llvm::Type::getInt1Ty(context);
        case AST::TYPE_NOTH:
            return llvm::Type::getVoidTy(context);
        default:
            throw_exception(SUB_CODEGEN, "Unsupported type", -1, file_name);
    }
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
    return nullptr;
}