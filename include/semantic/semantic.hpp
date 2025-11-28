/**
 * @file semantic.hpp
 *
 * @brief Header file for defining semantic analyzer
 */

#include "../parser/ast.hpp"
#include <vector>
#include <memory>
#include <stack>
#include <map>

class SemanticAnalyzer {
private:
    std::string file_name;                                                      /**< Absolute path to the Topaz source code */
    std::vector<AST::StmtPtr>& stmts;                                           /**< AST Tree (statements from Parser) */

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
        bool is_literal;                                                        /**< Is the value the result of an operation on literals */

        Value(AST::Type t, AST::Value v, bool il) : type(t), value(v), is_literal(il) {}
    };

    std::stack<std::map<std::string, Value>> variables;                         /**< View scope of the variables table */

    /**
     * @brief Structure of information about function
     */
    struct FunctionInfo {
        AST::Type ret_type;                                                     /**< Function return type */
        std::vector<AST::Argument> args;                                        /**< Function arguments */
        std::vector<AST::StmtPtr> block;                                        /**< Function block */
    };
    std::map<std::string, std::unique_ptr<FunctionInfo>> functions;             /**< Functions table */
    std::stack<AST::Type> functions_ret_types;                                  /**< Stack of functions return types */
    unsigned depth_of_loops;                                                    /**< Depth of loops */

public:
    SemanticAnalyzer(std::vector<AST::StmtPtr>& s, std::string fn) : stmts(s), file_name(fn), depth_of_loops(0) {
        variables.push({});
    }

    /**
     * @brief Method for analyze all statements
     *
     * This method analyze all statements to semantic errors. If have error, then throwing exception
     */
    void analyze();

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
     */
    void analyze_var_decl_stmt(AST::VarDeclStmt& vds);

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
     * @brief Method for evaluating and returning function returned value
     *
     * This method evaluating function returned value and returns it. If function dont have return statement, then throwing exception
     *
     * @param func Pointer to information about function
     * @param fce Function calling expression
     *
     * @return Evaluating function returned value
     */
    Value get_function_return_value(FunctionInfo *func, AST::FuncCallExpr& fce);

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
     * @brief Method for getting info about function from functions table
     *
     * This method getting info about function from functions table and returns it. If function not found, then returning null
     *
     * @param name Name of function
     *
     * @return Info about function
     */
    FunctionInfo *get_function_info(std::string name);

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
     * @brief Method for getting comon type between two types
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
};