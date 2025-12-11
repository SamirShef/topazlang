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
    std::string libs_path;                                                      /**< Absolute path to the Topaz libraries */
    std::string file_name;                                                      /**< Absolute path to the Topaz source code */
    std::vector<AST::StmtPtr>& stmts;                                           /**< AST Tree (statements from Parser) */
    bool is_debug;                                                              /**< Flag for debug exception */
    llvm::LLVMContext context;                                                  /**< LLVM Context */
    llvm::IRBuilder<> builder;                                                  /**< LLVM IR Builder */
    std::unique_ptr<llvm::Module> module;                                       /**< LLVM Module (module name is relative path to the Topaz source code) */
    std::stack<std::map<std::string, llvm::Value*>> variables;                  /**< View scope of the variables table */
    std::map<std::string, std::vector<llvm::Function*>> functions;              /**< Functions table */
    std::stack<llvm::Type*> functions_ret_types;                                /**< Stack of functions return types */
    std::stack<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>> loop_blocks;    /**< Stack of branches into cycles. First for 'break', second for 'continue' */

    /**
     * @brief Structure of part of path to object
     */
    struct PathPart {
        std::string name;                                                       /**< Name of part */

        /**
         * @brief Object from path (module or class)
         */
        enum Object {
            OBJ_MODULE,
            OBJ_CLASS,
        } object;
    };
    std::stack<PathPart> current_path;                                          /**< Stack to current path to some object */

public:
    CodeGenerator(std::vector<AST::StmtPtr>& s, std::string lp, std::string fn, bool id) : context(), builder(context), module(std::make_unique<llvm::Module>(fn, context)), stmts(s), libs_path(lp), file_name(fn), is_debug(id) {
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
     * @brief Method for generating LLVM IR code for function calling
     *
     * This method generating LLVM IR code for function calling
     *
     * @param fds Function calling statement
     */
    void generate_func_call_stmt(AST::FuncCallStmt& fcs);

    /**
     * @brief Method for generating LLVM IR code for 'return'
     *
     * This method generating LLVM IR code for 'return'
     *
     * @param rs Return statement
     */
    void generate_return_stmt(AST::ReturnStmt& rs);

    /**
     * @brief Method for generating LLVM IR code for control flow operators
     *
     * This method generating LLVM IR code for control flow operators
     *
     * @param ies Control flow operator
     */
    void generate_if_else_stmt(AST::IfElseStmt& ies);

    /**
     * @brief Method for generating LLVM IR code for while cycle
     *
     * This method generating LLVM IR code for while cycle
     *
     * @param wcs While cycle
     */
    void generate_while_cycle_stmt(AST::WhileCycleStmt& wcs);

    /**
     * @brief Method for generating LLVM IR code for do-while cycle
     *
     * This method generating LLVM IR code for do-while cycle
     *
     * @param dwcs Do-while cycle
     */
    void generate_do_while_cycle_stmt(AST::DoWhileCycleStmt& dwcs);

    /**
     * @brief Method for generating LLVM IR code for for cycle
     *
     * This method generating LLVM IR code for for cycle
     *
     * @param fcs For cycle
     */
    void generate_for_cycle_stmt(AST::ForCycleStmt& fcs);

    /**
     * @brief Method for generating LLVM IR code for break statement
     *
     * This method generating LLVM IR code for break statement
     *
     * @param bs Break statement
     */
    void generate_break_stmt(AST::BreakStmt& bs);

    /**
     * @brief Method for generating LLVM IR code for continue statement
     *
     * This method generating LLVM IR code for continue statement
     *
     * @param cs Continue statement
     */
    void generate_continue_stmt(AST::ContinueStmt& cs);

    /**
     * @brief Method for generating LLVM IR code for module definition
     *
     * This method generating LLVM IR code for module definition
     *
     * @param ms Module definition statement
     */
    void generate_module_stmt(AST::ModuleStmt& ms);

    /**
     * @brief Method for generating LLVM IR code for import the module
     *
     * This method generating LLVM IR code for import the module
     *
     * @param ums Importing module
     */
    void generate_use_module_stmt(AST::UseModuleStmt& ums);

    /**
     * @brief Method for generating LLVM IR code for extern calls
     *
     * This method generating LLVM IR code for extern calls
     *
     * @param es Extern calls
     */
    void generate_extern_stmt(AST::ExternStmt& es);

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
     * @brief Method for generating LLVM IR code for function calling expressions
     *
     * This method generating LLVM IR code for function calling expressions and returns it
     *
     * @param fce Function calling expression for generating
     *
     * @return Generated LLVM value
     */
    llvm::Value *generate_func_call_expr(AST::FuncCallExpr& fce);

    /**
     * @brief Method for generating LLVM IR code for chain of objects expression
     *
     * This method generating LLVM IR code for all expressions in path from chain
     *
     * @param co Chain of objects expression for generating
     *
     * @return Generated LLVM value
     */
    llvm::Value *generate_obj_chain_expr(AST::ChainObjects& co);

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

    /**
     * @brief Method for getting comon type between two types
     *
     * This method getting common type between two passed types and returns it. If common type does not exist, then throwing exception
     *
     * @param left Type to be implicitly cast
     * @param right Type to be implicitly cast to
     *
     * @return Common type between two passed types
     */
    llvm::Type *get_common_type(llvm::Type *left, llvm::Type *right);

    /**
     * @brief Method for implicitly cast value to expected type
     *
     * This method implicitly cast passed value to passed expected type and returns casted value. If cast is imposible, then returns nullptr
     *
     * @param val Value for casting
     * @param expected_type Type to be converted to
     *
     * @return Casted value
     */
    llvm::Value *implicitly_cast(llvm::Value *val, llvm::Type *expected_type);

    /**
     * @brief Method for getting mangled name
     *
     * This method returns mangled name based passed object name and CodeGenerator::current_path.
     * Names of parts of path separated '-' (if current part of path is module) or '#' (if current part of path is class)
     *
     * @param base_name Based name for mangling
     *
     * @return Mangled name
     */
    std::string get_mangled_name(std::string base_name);

    /**
     * @brief Method for getting resolved name by mangled name
     *
     * This method returns resolved name by passed mangled name
     *
     * @param mangled_name Mangled name for resolving
     *
     * @return Vector to PathPart
     */
    std::vector<PathPart> get_resolved_name(std::string mangled_name);
};