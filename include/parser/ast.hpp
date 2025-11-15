/**
 * @file ast.hpp
 *
 * @brief Header file for defining AST tree elements
 */

#pragma once
#include "../lexer/token.hpp"
#include <cstdint>
#include <sstream>
#include <uchar.h>
#include <utility>
#include <variant>
#include <string>
#include <memory>
#include <cmath>
#include <vector>

namespace AST {
    /**
     * @brief Type values enum
     */
    enum TypeValue {
        TYPE_CHAR,                               /**< 'char' type */
        TYPE_SHORT,                              /**< 'short' type */
        TYPE_INT,                                /**< 'int' type */
        TYPE_LONG,                               /**< 'long' type */
        TYPE_FLOAT,                              /**< 'float' type */
        TYPE_DOUBLE,                             /**< 'double' type */
        TYPE_BOOL,                               /**< 'bool' type */
        TYPE_STRING_LIT,                         /**< String literal type */
        TYPE_TRAIT,                              /**< Trait type */
        TYPE_CLASS,                              /**< Class type */
    };
    
    /**
     * @brief Structure for describing the type
     */
    struct Type {
        TypeValue type;                         /**< Enum type */
        std::string name;                       /**< String name of type */
        bool is_const;                          /**< Flag 'is constant type' */
        bool is_ptr;                            /**< Flag 'is raw pointer type' */
        bool is_nullable;                       /**< Flag 'is nullable type' */
        
        Type(TypeValue t, std::string n, bool ic = false, bool ip = false, bool in = false) : type(t), name(n), is_const(ic), is_ptr(ip), is_nullable(in) {}

        /**
         * @brief Method for convert type to string
         *
         * This method converting AST::Type to std::string. For example constant nullable int in Topaz will be converted as 'const int?'
         *
         * @return The type converted to string
         */
        std::string to_str() {
            std::stringstream ss;
            ss << (is_const ? "const " : "") << (is_ptr ? "*" : "") << name << (is_nullable ? "? " : "");
            return ss.str();
        }
    };

    /**
     * @brief Structure for describing the value
     */
    struct Value {
        std::variant<char8_t, int16_t, int32_t, int64_t, float_t, double_t, bool, std::string> value;               /**< Value as variant between char, short, int, long, float, double, bool and string */

        Value(char8_t v)            : value(v) {}
        Value(int16_t v)            : value(v) {}
        Value(int32_t v)            : value(v) {}
        Value(int64_t v)            : value(v) {}
        Value(float_t v)            : value(v) {}
        Value(double_t v)           : value(v) {}
        Value(bool v)               : value(v) {}
        Value(std::string v)        : value(v) {}
    };

    struct Argument {
        std::string name;
        Type type;

        Argument(std::string n, Type t) : name(n), type(t) {}
    };
    
    /**
     * @brief Base class of statement
     */
    class Stmt {
    public:
        uint32_t line;                      /**< Line coordinate */

        Stmt(uint32_t l) : line(l) {}
        virtual ~Stmt() = default;
    };

    /**
     * @brief Base class of expression
     */
    class Expr {
    public:
        uint32_t line;                      /**< Line coordinate */

        Expr(uint32_t l) : line(l) {}
        virtual ~Expr() = default;
    };

    using StmtPtr = std::unique_ptr<Stmt>;
    using ExprPtr = std::unique_ptr<Expr>;

    // EXPRESSIONS

    /**
     * @brief Base class of literal
     */
    class Literal : public Expr {
    public:
        Type type;                          /**< Type of literal */
        Value value;                        /**< Value of literal */

        Literal(Type t, Value v, uint32_t l) : type(t), value(v), Expr(l) {}
        ~Literal() override = default;
    };

    /**
     * @brief Character literal
     */
    class CharacterLiteral : public Literal {
    public:
        CharacterLiteral(char8_t v, uint32_t l) : Literal(Type(TYPE_CHAR, "char"), Value(v), l) {}
        ~CharacterLiteral() override = default;
    };

    /**
     * @brief Short literal
     */
    class ShortLiteral : public Literal {
    public:
        ShortLiteral(int16_t v, uint32_t l) : Literal(Type(TYPE_SHORT, "short"), Value(v), l) {}
        ~ShortLiteral() override = default;
    };

    /**
     * @brief Int literal
     */
    class IntLiteral : public Literal {
    public:
        IntLiteral(int32_t v, uint32_t l) : Literal(Type(TYPE_INT, "int"), Value(v), l) {}
        ~IntLiteral() override = default;
    };

    /**
     * @brief Long literal
     */
    class LongLiteral : public Literal {
    public:
        LongLiteral(int64_t v, uint32_t l) : Literal(Type(TYPE_LONG, "long"), Value(v), l) {}
        ~LongLiteral() override = default;
    };

    /**
     * @brief Float literal
     */
    class FloatLiteral : public Literal {
    public:
        FloatLiteral(float_t v, uint32_t l) : Literal(Type(TYPE_FLOAT, "float"), Value(v), l) {}
        ~FloatLiteral() override = default;
    };

    /**
     * @brief Double literal
     */
    class DoubleLiteral : public Literal {
    public:
        DoubleLiteral(double_t v, uint32_t l) : Literal(Type(TYPE_DOUBLE, "double"), Value(v), l) {}
        ~DoubleLiteral() override = default;
    };

    /**
     * @brief Boolean literal
     */
    class BoolLiteral : public Literal {
    public:
        BoolLiteral(bool v, uint32_t l) : Literal(Type(TYPE_BOOL, "bool"), Value(v), l) {}
        ~BoolLiteral() override = default;
    };

    /**
     * @brief String literal
     */
    class StringLiteral : public Literal {
    public:
        StringLiteral(std::string v, uint32_t l) : Literal(Type(TYPE_STRING_LIT, "string"), Value(v), l) {}
        ~StringLiteral() override = default;
    };

    /**
     * @brief Binary expression container
     *
     * Is the container of binary expression (aka <left_operand> <operator> <right_operand>)
     */
    class BinaryExpr : public Expr {
    public:
        TokenType op;                                           /**< Binary operator (+, -, *, /, &&, ||, !=, ==, >, >=, <, <=) */
        ExprPtr left_expr;                                      /**< Expression of left operand */
        ExprPtr right_expr;                                     /**< Expression of right operand */

        BinaryExpr(TokenType o, ExprPtr le, ExprPtr re, uint32_t l) : op(o), left_expr(std::move(le)), right_expr(std::move(re)), Expr(l) {}
        ~BinaryExpr() override = default;
    };

    /**
     * @brief Unary expression container
     *
     * Is the container of unary expression (aka <operator> <operand>)
     */
    class UnaryExpr : public Expr {
    public:
        TokenType op;                                           /**< Unary operator (-, !) */
        ExprPtr expr;                                           /**< Expression of operand */

        UnaryExpr(TokenType o, ExprPtr e, uint32_t l) : op(o), expr(std::move(e)), Expr(l) {}
        ~UnaryExpr() override = default;
    };

    class VarExpr : public Expr {
    public:
        std::string name;

        VarExpr(std::string n, uint32_t l) : name(n), Expr(l) {}
        ~VarExpr() override = default;
    };

    // STATEMENTS

    /**
     * @brief Statement of variable declaration
     */
    class VarDeclStmt : public Stmt {
    public:
        Type type;                                              /**< Variable type */
        ExprPtr expr;                                           /**< Variable initialization expression (maybe nullptr) */
        std::string name;                                       /**< Variable name */

        VarDeclStmt(Type t, ExprPtr e, std::string n, uint32_t l) : type(t), expr(std::move(e)), name(n), Stmt(l) {}
        ~VarDeclStmt() override = default;
    };

    /**
     * @brief Statement of functions declaration
     */
    class FuncDeclStmt : public Stmt {
    public:
        std::string name;                                       /**< Function name */
        std::vector<Argument> args;                             /**< Functions arguments */
        Type ret_type;                                          /**< Function return type */
        std::vector<StmtPtr> block;                             /**< Function block */

        FuncDeclStmt(std::string n, std::vector<Argument> a, Type rt, std::vector<StmtPtr> b, uint32_t l) : name(n), args(std::move(a)), ret_type(rt), block(std::move(b)), Stmt(l) {}
        ~FuncDeclStmt() override = default;
    };

    /**
     * @brief Statement of 'return'
     */
    class ReturnStmt : public Stmt {
    public:
        ExprPtr expr;                                           /**< Returned expression */

        ReturnStmt(ExprPtr e, uint32_t l) : expr(std::move(e)), Stmt(l) {}
        ~ReturnStmt() override = default;
    };
}