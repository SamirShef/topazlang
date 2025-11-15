#include "../../include/exception/exception.hpp"
#include "../../include/codegen/codegen.hpp"
#include <cstddef>
#include <llvm/IR/Argument.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/Casting.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/Constant.h>
#include <llvm/ADT/APInt.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>
#include <sstream>
#include <vector>

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
    else if (auto rs = dynamic_cast<AST::ReturnStmt*>(&stmt)) {
        generate_return_stmt(*rs);
    }
    else {
        throw_excpetion(SUB_CODEGEN, "Unsupported statement. Please check your Topaz compiler version and fix the problematic section of the code", stmt.line, file_name);
    }
}

void CodeGenerator::generate_var_decl_stmt(AST::VarDeclStmt& vds) {
    llvm::Type *type = type_to_llvm(vds.type);
    llvm::Value *val = llvm::Constant::getNullValue(type);
    if (vds.expr != nullptr) {
        val = generate_expr(*vds.expr);
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
        throw_excpetion(SUB_CODEGEN, ss.str(), vas.line, file_name);
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
    size_t index = 0;
    for (llvm::Argument& arg : func->args()) {
        arg.setName(fds.args[index].name);
        llvm::AllocaInst* arg_alloca = builder.CreateAlloca(arg.getType(), nullptr, fds.args[index].name);
        builder.CreateStore(&arg, arg_alloca);
        variables.top().emplace(fds.args[index].name, arg_alloca);
        index++;
    }
    size_t block_size = fds.block.size();
    for (size_t i = 0; i < block_size; i++) {
        generate_stmt(*fds.block[i]);
    }
    variables.pop();
}

void CodeGenerator::generate_return_stmt(AST::ReturnStmt& rs) {
    if (rs.expr != nullptr) {
        builder.CreateRet(generate_expr(*rs.expr));
    }
    else {
        builder.CreateRetVoid();
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
    else {
        throw_excpetion(SUB_CODEGEN, "An unsupported expression was encountered during compilation. Please check your Topaz compiler version and fix the problematic section of the code", expr.line, file_name);
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
            return llvm::ConstantFP::get(type_to_llvm(lit.type), std::get<float_t>(value));
        case AST::TYPE_DOUBLE:
            return llvm::ConstantFP::get(type_to_llvm(lit.type), std::get<double_t>(value));
        case AST::TYPE_BOOL:
            return llvm::ConstantInt::get(type_to_llvm(lit.type), llvm::APInt(1, std::get<bool>(value)));
        case AST::TYPE_STRING_LIT:
            return builder.CreateGlobalString(std::get<std::string>(value));
        default:
            throw_excpetion(SUB_CODEGEN, "An unsupported literal type was encountered during compilation. Please check your Topaz compiler version and fix the problematic section of the code", lit.line, file_name);
    }
}

llvm::Value *CodeGenerator::generate_binary_expr(AST::BinaryExpr& be) {
    llvm::Value *left = generate_expr(*be.left_expr);
    llvm::Type *left_type = left->getType();
    llvm::Value *right = generate_expr(*be.right_expr);
    llvm::Type *right_type = right->getType();

    switch (be.op) {
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
            throw_excpetion(SUB_CODEGEN, "An unsupported binary operator was encountered during compilation. Please check your Topaz compiler version and fix the problematic section of the code", be.line, file_name);
    }
}

llvm::Value *CodeGenerator::generate_unary_expr(AST::UnaryExpr& ue) {
    llvm::Value* value = generate_expr(*ue.expr);
    
    switch (ue.op) {
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
            throw_excpetion(SUB_CODEGEN, "An unsupported unary operator was encountered during compilation. Please check your Topaz compiler version and fix the problematic section of the code", ue.line, file_name);
    }
}

llvm::Value *CodeGenerator::generate_var_expr(AST::VarExpr& ve) {
    auto vars = variables;
    while (!vars.empty()) {
        auto vars_it = vars.top().find(ve.name);
        if (vars_it != vars.top().end()) {
            return builder.CreateLoad(vars_it->second->getType(), vars_it->second, ve.name + ".load");
        }
        vars.pop();
    }
    std::stringstream ss;
    ss << "Variable \033[0m'" << ve.name << "'\033[31m does not exists";
    throw_excpetion(SUB_CODEGEN, ss.str(), ve.line, file_name);
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
            throw_excpetion(SUB_CODEGEN, "Unsupported type", -1, file_name);
    }
}