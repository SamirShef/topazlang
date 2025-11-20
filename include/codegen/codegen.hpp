/**
 * @file codegen.hpp
 *
 * @brief Header file for defining compiler code generator
 */

#pragma once
#include "../parser/ast.hpp"
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>
#include <stack>
#include <map>

/**
 * @brief Code generator class
 */
class CodeGenerator {
private:
    std::string file_name;                                                      /**< Absolute path to the Topaz source code */
    std::vector<AST::StmtPtr>& stmts;                                           /**< AST Tree (statements from Parser) */
    llvm::LLVMContext context;                                                  /**< LLVM Context */
    llvm::IRBuilder<> builder;                                                  /**< LLVM IR Builder */
    std::unique_ptr<llvm::Module> module;                                       /**< LLVM Module (module name is relative path to the Topaz source code) */
    std::stack<std::map<std::string, llvm::Value*>> variables;                  /**< View scope of the variables table */
    std::map<std::string, llvm::Function*> functions;                           /**< Functions table */

public:
    CodeGenerator(std::vector<AST::StmtPtr>& s, std::string fn) : context(), builder(context), module(std::make_unique<llvm::Module>(fn, context)), stmts(s), file_name(fn) {
        variables.push({});
    }

    /**
     * @brief Method for generating LLVM IR code
     *
     * This method generating LLVM IR code for all AST statements
     */
    void generate();

    /**
     * @brief Method for printing generated LLVM IR code
     *
     * This method printing generated LLVM IR code into llvm::outs(). This method needs the entry point 'main'
     */
    void print_ir() {
        module->print(llvm::outs(), nullptr);
    }

    /**
     * @brief Method for getting current LLVM Module
     *
     * This method returning current LLVM Module. This method needs the entry point 'main'
     *
     * @return Current LLVM Module
     */
    std::unique_ptr<llvm::Module> get_module() {
        return std::move(module);
    }

private:
    /**
     * @brief Method for generating LLVM IR code for passing statement
     *
     * This method generating LLVM IR code for passing statement. If passed statement is unsupported by current version of compiler, then throwing exception
     *
     * @param stmt Statement for code generating
     */
    void generate_stmt(AST::Stmt& stmt);

    /**
     * @brief Method for generating LLVM IR code for variable definition
     *
     * This method generating LLVM IR code for variable definition. If variable definition in the global view scope, then generating global variable.
     * Otherwise generating local variable
     *
     * @param vds Variable declaration statement
     */
    void generate_var_decl_stmt(AST::VarDeclStmt& vds);

    /**
     * @brief Method for generating LLVM IR code for variable assignment
     *
     * This method generating LLVM IR code for variable assignment
     *
     * @param vds Variable assignment statement
     */
    void generate_var_asgn_stmt(AST::VarAsgnStmt& vas);

    /**
     * @brief Method for generating LLVM IR code for function definition
     *
     * This method generating LLVM IR code for function definition
     *
     * @param fds Function declaration statement
     */
    void generate_func_decl_stmt(AST::FuncDeclStmt& fds);

    /**
     * @brief Method for generating LLVM IR code for 'return'
     *
     * This method generating LLVM IR code for 'return'
     *
     * @param rs Return statement
     */
    void generate_return_stmt(AST::ReturnStmt& rs);

    /**
     * @brief Method for generating LLVM IR code for expressions
     *
     * This method generating LLVM IR cide for passing expression. If passed expression is unsupported by current version of compiler, then throwing exception
     *
     * @param expr Expression for code generating
     */
    llvm::Value *generate_expr(AST::Expr& expr);

    /**
     * @brief Method for generating LLVM IR code for literals
     *
     * This method generating LLVM IR code for literals and returns it. If type of passed literal is TYPE_STRING_LIT, then generating global string variable with value as literal value
     *
     * @param lit Literal for generating
     *
     * @return Generated LLVM value
     */
    llvm::Value *generate_literal_expr(AST::Literal& lit);

    /**
     * @brief Method for generating LLVM IR code for binary expressions
     *
     * This method generating LLVM IR code for binary expressions and returns it. If type of passed binary operator is unsupported by current version of compiler, then throwing exception
     *
     * @param be Binary expression for generating
     *
     * @return Generated LLVM value
     */
    llvm::Value *generate_binary_expr(AST::BinaryExpr& be);
    
    /**
     * @brief Method for generating LLVM IR code for unary expressions
     *
     * This method generating LLVM IR code for unary expressions and returns it. If type of passed unary operator is unsupported by current version of compiler, then throwing exception
     *
     * @param be Unary expression for generating
     *
     * @return Generated LLVM value
     */
    llvm::Value *generate_unary_expr(AST::UnaryExpr& ue);
    
    /**
     * @brief Method for generating LLVM IR code for variable expressions
     *
     * This method generating LLVM IR code for variable expressions and returns it
     *
     * @param ve Variable expression for generating
     *
     * @return Generated LLVM value
     */
    llvm::Value *generate_var_expr(AST::VarExpr& ve);

    /**
     * @brief Method for converting AST::Type to llvm::Type
     *
     * This method converting passed AST::Type to llvm::Type and returns it. If type of passed AST::Type is unsupported by current version of compiler, then throwing exception
     *
     * @param type AST::Type for converting
     *
     * @return Converted type to llvm::Type
     */
    llvm::Type *type_to_llvm(AST::Type type);
};