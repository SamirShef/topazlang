/**
 * @file semantic.hpp
 *
 * @brief Header file for defining semantic analyzer
 */

#include "../parser/ast.hpp"
#include <filesystem>
#include <vector>
#include <memory>
#include <stack>
#include <map>

class SemanticAnalyzer {
private:
    std::string libs_path;                                                      /**< Absolute path to the Topaz libraries */
    std::string file_name;                                                      /**< Absolute path to the Topaz source code */
    std::string path_to_current_dir;                                            /**< Absolute path to the directory which contains current Topaz source code file */
    std::vector<AST::StmtPtr>& stmts;                                           /**< AST Tree (statements from Parser) */
    bool is_debug;                                                              /**< Flag for debug exception */
    bool in_unsafe;                                                             /**< Flag that indicates whether we are inside an unsafe context */

    /**
     * @brief Current space (in global, in module or in function)
     */
    enum Space {
        SPACE_GLOBAL,
        SPACE_MODULE,
        SPACE_FUNCTION,
    } current_space;

    std::map<AST::TypeValue, std::vector<AST::TypeValue>> implicitly_cast_allowed_types {
        {AST::TYPE_BOOL, {}},
        {AST::TYPE_CHAR, {AST::TYPE_SHORT, AST::TYPE_INT, AST::TYPE_LONG, AST::TYPE_FLOAT, AST::TYPE_DOUBLE}},
        {AST::TYPE_SHORT, {AST::TYPE_INT, AST::TYPE_LONG, AST::TYPE_FLOAT, AST::TYPE_DOUBLE}},
        {AST::TYPE_INT, {AST::TYPE_LONG, AST::TYPE_FLOAT, AST::TYPE_DOUBLE}},
        {AST::TYPE_LONG, {AST::TYPE_FLOAT, AST::TYPE_DOUBLE}},
        {AST::TYPE_FLOAT, {AST::TYPE_DOUBLE}}
    };                                                                          /**< Type table for implicit casting */

    /**
     * @brief Structure of value
     */
    struct Value {
        AST::Type type;                                                         /**< Type of value */
        AST::Value value;                                                       /**< Primitive value */
        bool is_value_unknown;                                                  /**< Is the value the unknown at the compilation time (for example I/O value) */
        bool is_literal;                                                        /**< Is the value the result of an operation on literals */

        Value(AST::Type t, AST::Value v, bool vu, bool il) : type(t), value(v), is_value_unknown(vu), is_literal(il) {}
    };

    std::stack<std::map<std::string, Value>> variables;                         /**< View scope of the variables table */

    /**
     * @brief Structure of information about function
     */
    struct FunctionInfo {
        AST::Type ret_type;                                                     /**< Function return type */
        std::vector<AST::Argument> args;                                        /**< Function arguments */
        std::vector<AST::StmtPtr> block;                                        /**< Function block */

        FunctionInfo(AST::Type rt, std::vector<AST::Argument> a, std::vector<AST::StmtPtr> b) : ret_type(rt), args(std::move(a)), block(std::move(b)) {}
    };
    std::map<std::string, std::vector<std::shared_ptr<FunctionInfo>>> functions;/**< Functions table (name, candidates) */
    std::stack<AST::Type> functions_ret_types;                                  /**< Stack of functions return types */
    unsigned depth_of_loops;                                                    /**< Depth of loops */

public:
    /**
     * @brief Structure of information about module
     */
    struct ModuleInfo {
        std::map<std::string, std::pair<AST::AccessModifier, ModuleInfo*>> modules;     /**< Submodules into module */
        std::map<std::string, std::pair<AST::AccessModifier, std::string>> functions;   /**< Functions table in module */
    };
private:
    std::map<std::string, ModuleInfo*> modules;                                 /**< Modules table */
    std::vector<std::string> names_of_imported_modules;                         /**< Names of already imported modules */

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
    SemanticAnalyzer(std::vector<AST::StmtPtr>& s, std::string lp, std::string fn, bool id) : stmts(s), libs_path(lp), file_name(fn), depth_of_loops(0), is_debug(id) {
        std::filesystem::path file_path = std::filesystem::absolute(fn);
        path_to_current_dir = file_path.parent_path().string();
        variables.push({});
    }

    /**
     * @brief Method for analyze all statements
     *
     * This method analyze all statements to semantic errors. If have error, then throwing exception
     */
    void analyze();

    /**
     * @brief Method for getting modules from semantic
     *
     * @return Table of modules
     */
    std::map<std::string, ModuleInfo*> get_modules() const {
        return modules;
    }

    /**
     * @brief Method for getting functions from semantic
     *
     * @return Table of functions
     */
    std::map<std::string, std::vector<std::shared_ptr<FunctionInfo>>> get_functions() const {
        return functions;
    }

private:
    /**
     * @brief Method for analyze one statement
     *
     * This method analyze one statement to semantic errors. If have error, then throwing exception
     *
     * @param stmt Statement for analyzing
     */
    void analyze_stmt(AST::Stmt& stmt);

    /**
     * @brief Method for analyze variable declaration
     *
     * This method analyze variable declaration
     *
     * @param vds Variable declaration statement for analyzing
     * @param is_func_arg Flag that indicates that a variable is an argument to a function
     */
    void analyze_var_decl_stmt(AST::VarDeclStmt& vds, bool is_func_arg = false);

    /**
     * @brief Method for analyze variable assignment
     *
     * This method analyze variable assignment
     *
     * @param vas Variable assignment statement for analyzing
     */
    void analyze_var_asgn_stmt(AST::VarAsgnStmt& vas);

    /**
     * @brief Method for analyze function declaration
     *
     * This method analyze function declaration
     *
     * @param fds Function declaration statement for analyzing
     */
    void analyze_func_decl_stmt(AST::FuncDeclStmt& fds);

    /**
     * @brief Method for analyze function calling
     *
     * This method analyze function calling
     *
     * @param fds Function calling statement for analyzing
     */
    void analyze_func_call_stmt(AST::FuncCallStmt& fcs);

    /**
     * @brief Method for analyze 'return' statement
     *
     * This method analyze 'return' statement
     *
     * @param rs 'return' statement for analyzing
     */
    void analyze_return_stmt(AST::ReturnStmt& rs);

    /**
     * @brief Method for analyze control flow operators
     *
     * This method analyze control flow operators. If condition for control flow operator 'if' is not a bool type, then throwing exception
     *
     * @param ies Control flow operator
     */
    void analyze_if_else_stmt(AST::IfElseStmt& ies);

    /**
     * @brief Method for analyze while cycle
     *
     * This method analyze while cycle. If condition for while cycle is not a bool type, then throwing exception
     *
     * @param wcs While cycle
     */
    void analyze_while_cycle_stmt(AST::WhileCycleStmt& wcs);

    /**
     * @brief Method for analyze do-while cycle
     *
     * This method analyze do-while cycle. If condition for do-while cycle is not a bool type, then throwing exception
     *
     * @param dwcs Do-while cycle
     */
    void analyze_do_while_cycle_stmt(AST::DoWhileCycleStmt& dwcs);

    /**
     * @brief Method for analyze for cycle
     *
     * This method analyze for cycle.
     * If condition for for cycle is not a bool type, then throwing exception.
     * If indexator statement is not a variable declaration/assignment, then throwing exception.
     *
     * @param fcs For cycle
     */
    void analyze_for_cycle_stmt(AST::ForCycleStmt& fcs);

    /**
     * @brief Method for analyze break statement
     *
     * This method analyze break statement. If break statement isn't in cycle, then throwing exception
     *
     * @param bs Break statement
     */
    void analyze_break_stmt(AST::BreakStmt& bs);

    /**
     * @brief Method for analyze continue statement
     *
     * This method analyze continue statement. If continue statement isn't in cycle, then throwing exception
     *
     * @param cs Continue statement
     */
    void analyze_continue_stmt(AST::ContinueStmt& cs);

    /**
     * @brief Method for analyze module definition
     *
     * This method analyze module definition
     *
     * @param ms Module definition statement
     */
    void analyze_module_stmt(AST::ModuleStmt& ms);

    /**
     * @brief Method for analyze import the module
     *
     * This method analyze import the module. If module already imported, then throwing exception
     *
     * @param ums Importing module
     */
    void analyze_use_module_stmt(AST::UseModuleStmt& ums);

    /**
     * @brief Method for analyze unsafe context
     *
     * This method analyze unsafe context (pointers and extern calls)
     *
     * @param us Unsafe context block
     */
    void analyze_unsafe_stmt(AST::UnsafeStmt& us);
    
    /**
     * @brief Method for analyze expression
     *
     * This method analyze passed expression and returns value of it
     *
     * @param expr Expression for analyzing
     *
     * @return Value of passed expression
     */
    Value analyze_expr(AST::Expr& expr);
    
    /**
     * @brief Method for analyze literal
     *
     * This method analyze passed literal and returns value of it
     *
     * @param lit Literal for analyzing
     *
     * @return Value of passed literal
     */
    Value analyze_literal_expr(AST::Literal& lit);

    /**
     * @brief Method for analyze binary expression
     *
     * This method analyze passed binary expression and returns value of it
     *
     * @param be Binary expression for analyzing
     *
     * @return Value of passed binary expression
     */
    Value analyze_binary_expr(AST::BinaryExpr& be);

    /**
     * @brief Method for analyze unary expression
     *
     * This method analyze passed unary expression and returns value of it
     *
     * @param ue Unary expression for analyzing
     *
     * @return Value of passed unary expression
     */
    Value analyze_unary_expr(AST::UnaryExpr& ue);

    /**
     * @brief Method for analyze variable expression
     *
     * This method searching passed variable in view scope of variables table and returns value of it
     *
     * @param ve Variable expression for analyzing
     *
     * @return Value of passed variable (if have)
     */
    Value analyze_var_expr(AST::VarExpr& ve);

    /**
     * @brief Method for analyze function calling expression
     *
     * This method searching passed function in functions table and returns value of call this function
     *
     * @param fce Function calling expression for analyzing
     *
     * @return Value of passed function calling (if have)
     */
    Value analyze_func_call_expr(AST::FuncCallExpr& fce);

    /**
     * @brief Method for analyze chain of objects expression
     *
     * This method checking all expressions in path from chain
     *
     * @param co Chain of objects expression for evaluating
     *
     * @return Value of passed chain of objects
     */
    Value analyze_obj_chain_expr(AST::ChainObjects& co);

    /**
     * @brief Method for analyze object from chain of objects expression
     *
     * This method analyzing passed object expression by passed target value
     *
     * @param target Value of target object
     * @param obj Expression of object for analyzing
     *
     * @return Value of passed object expression
     */
    Value analyze_obj_from_chain(Value target, AST::Expr& obj);

    /**
     * @brief Method for evaluating and returning function returned value
     *
     * This method evaluating function returned value and returns it. If function dont have return statement, then throwing exception
     *
     * @param func Pointer to information about function
     * @param fce Function calling expression
     *
     * @return Evaluating function returned value
     */
    Value get_function_return_value(std::shared_ptr<FunctionInfo> func, AST::FuncCallExpr& fce);

    /**
     * @brief Method for evaluating and returning function returned value from control flow operators
     *
     * This method evaluating function returned value from control flow operators and returns it.
     * If in block of statements of control flow operators does not have a 'return' statement, then returns nullptr
     *
     * @param ies Control flow operators
     *
     * @return Evaluating function returned value or nullptr if does not have 'return'
     */
    Value *get_function_return_value_from_if_else(AST::IfElseStmt& ies);

    /**
     * @brief Method for evaluating and returning function returned value from while cycle
     *
     * This method evaluating function returned value from while cycle and returns it.
     * If in block of statements of while cycle does not have a 'return' statement, then returns nullptr
     *
     * @param wcs While cycle
     *
     * @return Evaluating function returned value or nullptr if does not have 'return'
     */
    Value *get_function_return_value_from_while_cycle(AST::WhileCycleStmt& wcs);

    /**
     * @brief Method for evaluating and returning function returned value from do-while cycle
     *
     * This method evaluating function returned value from do-while cycle and returns it.
     * If in block of statements of do-while cycle does not have a 'return' statement, then returns nullptr
     *
     * @param dwcs Do-while cycle
     *
     * @return Evaluating function returned value or nullptr if does not have 'return'
     */
    Value *get_function_return_value_from_do_while_cycle(AST::DoWhileCycleStmt& dwcs);

    /**
     * @brief Method for evaluating and returning function returned value from for cycle
     *
     * This method evaluating function returned value from for cycle and returns it.
     * If in block of statements of for cycle does not have a 'return' statement, then returns nullptr
     *
     * @param fcs For cycle
     *
     * @return Evaluating function returned value or nullptr if does not have 'return'
     */
    Value *get_function_return_value_from_for_cycle(AST::ForCycleStmt& fcs);

    /**
     * @brief Method for getting default value by type
     *
     * This method getting default value by passed type and returns it. If cannot generating value, then throwing exception
     *
     * @param type Type by which the default value is generated
     * @param line Line coordinate in Topaz source code (for exception)
     *
     * @return Default value by passed type
     */
    AST::Value get_default_val_by_type(AST::Type type, uint32_t line);

    /**
     * @brief Method for getting value of variable from view scope of variables table
     *
     * This method getting value of variable from view scope of variables table and returns it. If variable not found, then returning null
     *
     * @param name Name of variable
     *
     * @return Value of variable
     */
    std::unique_ptr<Value> get_variable_value(std::string name);

    /**
     * @brief Method for getting function candidates from functions table
     *
     * This method getting function candidates from functions table and returns it. If function not found, then returning empty vector
     *
     * @param name Name of function
     *
     * @return Candidates
     */
    std::vector<std::shared_ptr<FunctionInfo>> get_function_candidates(std::string name);

    /**
     * @brief Method for determining whether two types have a common type
     *
     * This method searches the table of types that can be implicitly cast to the type that needs to be converted to the type that is required.
     * If a match is found, then true is returned, otherwise false.
     *
     * @param left Type to be implicitly cast
     * @param right Type to be implicitly cast to
     *
     * @return true if there is a common type for the two passed types, otherwise false
     */
    bool has_common_type(AST::Type left, AST::Type right);
    
    /**
     * @brief Method for getting common type between two types
     *
     * This method getting common type between two passed types and returns it. If common type does not exist, then throwing exception
     *
     * @param left Type to be implicitly cast
     * @param right Type to be implicitly cast to
     * @param line Line coordinate in Topaz source code (for exception)
     *
     * @return Common type between two passed types
     */
    AST::Type get_common_type(AST::Type left, AST::Type right, uint32_t line);

    /**
     * @brief Method for getting implicitly casted value between two values
     *
     * This method getting implicitly casted value between two passed values and returns it. If common type does not exist, then throwing exception
     *
     * @param val Value to be implicitly cast
     * @param type Type to be implicitly cast to
     * @param line Line coordinate in Topaz source code (for exception)
     *
     * @return Implicitly casted value
     */
    Value implicitly_cast(Value val, AST::Type type, uint32_t line);

    /**
     * @brief Method for evaluating binary operations on two values from std::variant
     *
     * This method evaluating binary operations on two values from std::variant and returns result
     *
     * @param left Value of left operand
     * @param right Value of right operand
     * @param op Type of binary operator
     * @param line Line coordinate in Topaz source code (for exception)
     *
     * @return Evaluating value
     */
    double binary_two_variants(Value left, Value right, TokenType op, uint32_t line);

    /**
     * @brief Method for evaluating unary operations on two values from std::variant
     *
     * This method evaluating unary operations on two values from std::variant and returns result
     *
     * @param left Value of left operand
     * @param right Value of right operand
     * @param op Type of unary operator
     * @param line Line coordinate in Topaz source code (for exception)
     *
     * @return Evaluating value
     */
    double unary_two_variants(Value value, TokenType op, uint32_t line);

    /**
     * @brief Method for getting mangled name
     *
     * This method returns mangled name based passed object name and SemanticAnalyzer::current_path.
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