/**
 * @file parser.hpp
 *
 * @brief Header file for defining parser
 */

#pragma once
#include "../lexer/token.hpp"
#include "ast.hpp"

/**
 * @brief Parser class
 */
class Parser {
private:
    std::vector<Token> tokens;                                  /**< Tokens (from Lexer) */
    size_t tokens_count;                                        /**< Count of tokens */
    uint32_t pos;                                               /**< Current position in tokens */
    bool is_debug;                                              /**< Flag for debug exception */

public:
    Parser(std::vector<Token> t, bool id) : tokens(t), tokens_count(t.size()), pos(0), is_debug(id) {}

    /**
     * @brief Method for parsing tokens into AST tree
     *
     * This method parsing all tokens, creates AST tree and returns it
     *
     * @return AST tree
     */
    std::vector<AST::StmtPtr> parse();

    /**
     * @brief Method for resetting all parameters in parser
     */
    void reset();

private:
    /**
     * @brief Method for parsing only one statement
     *
     * This method parsing one statement and returns it
     *
     * @param from_for Flag that indicates that the method is being called to process the for loop. If true, then processing for the for loop, otherwise not
     *
     * @return Parsed statement
     */
    AST::StmtPtr parse_stmt(bool from_for = false);

    /**
     * @brief Method for parsing of variable declaration
     *
     * This method sets the syntax for defining a variable, creates the AST element of the VarDeclStmt and returns it
     *
     * @return VarDeclStmt
     */
    AST::StmtPtr parse_var_decl_stmt();

    /**
     * @brief Method for parsing of variable assignment
     *
     * This method sets the syntax for assignment a variable, creates the AST element of the VarAsgnStmt and returns it
     *
     * @return VarAsgnStmt
     */
    AST::StmtPtr parse_var_asgn_stmt();

    /**
     * @brief Method for parsing of functions declaration
     *
     * This method sets the syntax for defining a function, creates the AST element of the FuncDeclStmt and returns it
     *
     * @return FuncDeclStmt
     */
    AST::StmtPtr parse_func_decl_stmt();

    /**
     * @brief Method for parsing of functions calling
     *
     * This method sets the syntax for calling a function, creates the AST element of the FuncCallStmt and returns it
     *
     * @return FuncCallStmt
     */
    AST::StmtPtr parse_func_call_stmt();

    /**
     * @brief Method for parsing of function argument
     *
     * This method sets the syntax for defining a function argument, creates the AST element of the Argument and returns it
     *
     * @return Argument
     */
    AST::Argument parse_argument();

    /**
     * @brief Method for parsing of 'return'
     *
     * This method sets the syntax for 'return' statement, creates the AST element of the ReturnStmt and returns it
     *
     * @return ReturnStmt
     */
    AST::StmtPtr parse_return_stmt();

    /**
     * @brief Method for parsing of control flow operators (aka if-else)
     *
     * This method sets the syntax for control flow operators, creates the AST element of the IfElseStmt and returns it
     *
     * @return IfElseStmt
     */
    AST::StmtPtr parse_if_else_stmt();

    /**
     * @brief Method for parsing of while cycle
     *
     * This method sets the syntax for while cycle, creates the AST element of the WhileCycleStmt and returns it
     *
     * @return WhileCycleStmt
     */
    AST::StmtPtr parse_while_cycle_stmt();

    /**
     * @brief Method for parsing of do-while cycle
     *
     * This method sets the syntax for do-while cycle, creates the AST element of the DoWhileCycleStmt and returns it
     *
     * @return DoWhileCycleStmt
     */
    AST::StmtPtr parse_do_while_cycle_stmt();

    /**
     * @brief Method for parsing of for cycle
     *
     * This method sets the syntax for for cycle, creates the AST element of the ForCycleStmt and returns it
     *
     * @return ForCycleStmt
     */
    AST::StmtPtr parse_for_cycle_stmt();

    /**
     * @brief Method for parsing of break statement
     *
     * This method sets the syntax for break statement, creates the AST element of the BreakStmt and returns it
     *
     * @return BreakStmt
     */
    AST::StmtPtr parse_break_stmt();

    /**
     * @brief Method for parsing of continue statement
     *
     * This method sets the syntax for continue statement, creates the AST element of the ContinueStmt and returns it
     *
     * @return ContinueStmt
     */
    AST::StmtPtr parse_continue_stmt();

    /**
     * @brief Method for parsing of module definition
     *
     * This method sets the syntax for module definition, creates the AST element of the ModuleStmt and returns it
     *
     * @return ModuleStmt
     */
    AST::StmtPtr parse_module_stmt();

    /**
     * @brief Method for parsing of import the module
     *
     * This method sets the syntax for import the module, creates the AST element of the UseModuleStmt and returns it
     *
     * @return UseModuleStmt
     */
    AST::StmtPtr parse_use_module_stmt();
    
    /**
     * @brief Method for parsing expressions
     *
     * This method parsing expression, creates the AST element and returns it
     *
     * @return Parsed expression
     */
    AST::ExprPtr parse_expr();

    /**
     * @brief Method for parsing expression as 'logical and'
     *
     * This method parsing all expressions as 'logical and'. If type of current token isnt TOK_OP_L_AND (aka &&), then returns created expression
     *
     * @return Parsed expression as 'logical and'
     */
    AST::ExprPtr parse_l_and_expr();

    /**
     * @brief Method for parsing expression as 'logical or'
     *
     * This method parsing all expressions as 'logical or'. If type of current token isnt TOK_OP_L_OR (aka ||), then returns created expression
     *
     * @return Parsed expression as 'logical or'
     */
    AST::ExprPtr parse_l_or_expr();

    /**
     * @brief Method for parsing expression as 'equality'
     *
     * This method parsing all expressions as 'equality'. If type of current token isnt TOK_OP_EQ_EQ (aka ==) or TOK_OP_NOT_EQ_EQ (aka !=), then returns created expression
     *
     * @return Parsed expression as 'equality'
     */
    AST::ExprPtr parse_equality_expr();

    /**
     * @brief Method for parsing expression as 'comparation'
     *
     * This method parsing all expressions as 'comparation'. If type of current token isnt TOK_OP_GT (aka >) or TOK_OP_GT_EQ (aka >=) or TOK_OP_LS (aka <) or TOK_OP_LS_EQ (aka <=), then returns created expression
     *
     * @return Parsed expression as 'comparation'
     */
    AST::ExprPtr parse_comparation_expr();

    /**
     * @brief Method for parsing expression as 'additive'
     *
     * This method parsing all expressions as 'additive'. If type of current token isnt TOK_OP_PLUS (aka +) or TOK_OP_MINUS (aka -), then returns created expression
     *
     * @return Parsed expression as 'additive'
     */
    AST::ExprPtr parse_additive_expr();

    /**
     * @brief Method for parsing expression as 'multiplicative'
     *
     * This method parsing all expressions as 'multiplicative'. If type of current token isnt TOK_OP_MULT (aka *) or TOK_OP_DIV (aka /) or TOK_OP_MODULO (aka %), then returns created expression
     *
     * @return Parsed expression as 'multiplicative'
     */
    AST::ExprPtr parse_multiplicative_expr();

    /**
     * @brief Method for parsing expression as 'unary'
     *
     * This method parsing all expressions as 'unary'. If type of current token isnt TOK_OP_L_NOT (aka !) or TOK_OP_MINUS (aka -), then returns created expression
     *
     * @return Parsed expression as 'unary'
     */
    AST::ExprPtr parse_unary_expr();

    /**
     * @brief Method for parsing expression as 'primary'
     *
     * This method parsing all expressions as 'primary'. If type of current token isnt literal or identifier, then throwing exception
     *
     * @return Parsed expression as 'primary'
     */
    AST::ExprPtr parse_primary_expr();

    /**
     * @brief Method for parsing expresion as chain of objects
     *
     * This method parsing all expressions as chain of objects. Elements of chain separated of '.' (for example: obj1.obj2)
     *
     * @return Parsed expression as chain of objects
     */
    AST::ExprPtr parse_obj_chain_expr();

    /**
     * @brief Method for getting token from tokens by parser pos and passed offset
     *
     * This method getting getting token from tokens by parser pos and passed offset
     * If pos + offset out of bounds of range tokens, then throwing exception
     *
     * @param rpos Offset
     *
     * @return Token from tokens
     */
    Token peek(int32_t rpos = 0) const;

    /**
     * @brief Method for skipping the current token if its type is equal to the passed one
     *
     * If type of current token is equal to the passed type, then this token will be skipped and returns 'true'. Otherwise returns 'false'
     *
     * @param type Type of token wich need to skip
     *
     * @return 'true' if passed type is equal to type of current token and 'false' otherwise
     */
    bool match(TokenType type);

    /**
     * @brief Method for verifying the type of the current token
     *
     * This method checks whether the type of the current token is equal to the passed type.
     * If equal, it skips and returns it, otherwise it throws an exception with the passed message
     *
     * @param type Type of token for verification
     * @param err_msg Error message if verification is fault
     * @param line Line coordinate. For exception
     *
     * @return Verificated token
     */
    Token consume(TokenType type, std::string err_msg, uint32_t line);

    /**
     * @brief Method for verifyng the Topaz type by current tokens
     *
     * This method checks the current keywords to collect the final Topaz type and returns it.
     * If the tokens are in the wrong sequence or are not part of the type, an exception is thrown
     *
     * @return Topaz type
     */
    AST::Type consume_type();

    /**
     * @brief Method for skipping semicolon in the end of statement
     *
     * This method checks the current token and if it is the semicolon, then skipping it, otherwise throwing exception
     */
    void consume_semicolon();

    /**
     * @brief Method for convert type of token to type of Topaz value
     *
     * This method converts the passed token type to the Topaz value type. If the conversion fails, an exception is thrown
     *
     * @param type Type of token for convertion
     *
     * @return Converted type of Topaz value
     */
    AST::TypeValue ttype_to_tvalue(TokenType type);

    /**
     * @brief Method for checking whether the passed token is a compound assignment operator
     *
     * This method checks whether the passed token is a compound assignment operator. If yes, it returns true, otherwise it returns false
     *
     * @param token Token for checking
     *
     * @return True if token is compound assignment operator, and false otherwise
     */
    bool is_compound_asgn_operator(Token token);
    
    /**
     * @brief Method for creating compound assignment operator and returns expression of assignment
     *
     * This method checking current token and if it is compound assignment operator, then generating expression for assignment.
     * For example 'a += 1' is equal 'a = a + 1'
     * If current token isnt compound assignment operator, then throwing exception
     *
     * @param var_name Name of variable for assignment
     *
     * @return Assingment expression
     */
    AST::ExprPtr create_compound_asgn_operator(std::string var_name);

    /**
     * @brief Method for creating increment/decrement operator and returns expression of assignment
     *
     * This method checking current token and if it is increment/decrement operator, then generating expression of assignment.
     * For example 'a++' is equal 'a = a + 1'
     * If current token isnt increment/decrement operator, then throwing exception
     *
     * @param var_name Name of variable for assignment
     *
     * @return Increment/Decrement expression
     */
    AST::ExprPtr create_inc_dec_operator(std::string var_name);
};