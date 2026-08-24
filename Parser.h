#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <initializer_list>
#include "Lexer.h"   // Token / TokenType

// ============================================================
// Nodos del AST
// ------------------------------------------------------------
// Usamos std::shared_ptr en vez de punteros crudos para que no
// tengas que preocuparte por 'delete' ni por memory leaks al
// construir el árbol.
// ============================================================
//ajustes al parser
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual std::string toString() const = 0;
};

class Expression : public ASTNode {};
class Statement : public ASTNode {};

using ExprPtr = std::shared_ptr<Expression>;
using StmtPtr = std::shared_ptr<Statement>;

class BinaryOperation : public Expression {
public:
    ExprPtr left;
    std::string op;
    ExprPtr right;

    BinaryOperation(ExprPtr l, std::string o, ExprPtr r)
        : left(std::move(l)), op(std::move(o)), right(std::move(r)) {}

    std::string toString() const override {
        return "BinaryOp(" + left->toString() + " " + op + " " + right->toString() + ")";
    }
};

class Literal : public Expression {
public:
    enum class Kind { NUMBER, STRING, BOOL_TRUE, BOOL_FALSE };

    std::string rawValue;
    Kind kind;

    Literal(std::string v, Kind k) : rawValue(std::move(v)), kind(k) {}

    std::string toString() const override {
        return "Literal(" + rawValue + ")";
    }
};

class Identifier : public Expression {
public:
    std::string name;
    explicit Identifier(std::string n) : name(std::move(n)) {}

    std::string toString() const override {
        return "Identifier(" + name + ")";
    }
};

class FunctionCall : public Expression {
public:
    std::string name;
    std::vector<ExprPtr> arguments;

    FunctionCall(std::string n, std::vector<ExprPtr> args)
        : name(std::move(n)), arguments(std::move(args)) {}

    std::string toString() const override {
        std::ostringstream oss;
        oss << "FunctionCall(" << name << "(";
        for (size_t i = 0; i < arguments.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << arguments[i]->toString();
        }
        oss << "))";
        return oss.str();
    }
};

class Assignment : public Statement {
public:
    std::string target;
    ExprPtr value;

    Assignment(std::string t, ExprPtr v) : target(std::move(t)), value(std::move(v)) {}

    std::string toString() const override {
        return "Assignment(" + target + " = " + value->toString() + ")";
    }
};

class IfStatement : public Statement {
public:
    ExprPtr condition;
    std::vector<StmtPtr> thenBody;
    std::vector<StmtPtr> elseBody;

    IfStatement(ExprPtr cond, std::vector<StmtPtr> thenB, std::vector<StmtPtr> elseB)
        : condition(std::move(cond)), thenBody(std::move(thenB)), elseBody(std::move(elseB)) {}

    std::string toString() const override {
        std::ostringstream oss;
        oss << "If(" << condition->toString() << ") then [";
        for (size_t i = 0; i < thenBody.size(); ++i) {
            if (i > 0) oss << "; ";
            oss << thenBody[i]->toString();
        }
        oss << "] else [";
        for (size_t i = 0; i < elseBody.size(); ++i) {
            if (i > 0) oss << "; ";
            oss << elseBody[i]->toString();
        }
        oss << "]";
        return oss.str();
    }
};

class WhileStatement : public Statement {
public:
    ExprPtr condition;
    std::vector<StmtPtr> body;

    WhileStatement(ExprPtr cond, std::vector<StmtPtr> b)
        : condition(std::move(cond)), body(std::move(b)) {}

    std::string toString() const override {
        std::ostringstream oss;
        oss << "While(" << condition->toString() << ") [";
        for (size_t i = 0; i < body.size(); ++i) {
            if (i > 0) oss << "; ";
            oss << body[i]->toString();
        }
        oss << "]";
        return oss.str();
    }
};

class PrintStatement : public Statement {
public:
    ExprPtr expression;
    explicit PrintStatement(ExprPtr e) : expression(std::move(e)) {}

    std::string toString() const override {
        return "Print(" + expression->toString() + ")";
    }
};

class Program : public ASTNode {
public:
    std::vector<StmtPtr> statements;
    explicit Program(std::vector<StmtPtr> stmts) : statements(std::move(stmts)) {}

    std::string toString() const override {
        std::ostringstream oss;
        oss << "Program([\n";
        for (size_t i = 0; i < statements.size(); ++i) {
            oss << "  " << statements[i]->toString();
            if (i + 1 < statements.size()) oss << ",\n";
        }
        oss << "\n])";
        return oss.str();
    }
};

// Error de sintaxis con información de posición.
class ParserSyntaxError : public std::runtime_error {
public:
    ParserSyntaxError(const std::string& message, const Token* token)
        : std::runtime_error(formatMessage(message, token)) {}

private:
    static std::string formatMessage(const std::string& message, const Token* token) {
        if (token != nullptr) {
            return "Error de sintaxis en linea " + std::to_string(token->line) +
                   ", columna " + std::to_string(token->column) + ": " + message;
        }
        return "Error de sintaxis: " + message;
    }
};

// ============================================================
// SyntaxAnalyzer (Parser descendente recursivo)
// ------------------------------------------------------------
//   programa    -> sentencia*
//   sentencia   -> asignacion | if | while | print
//   bloque      -> '{' sentencia* '}'
//   comparacion -> expresion ((==|!=|<|>|<=|>=|and|or) expresion)*
//   expresion   -> termino ((+|-) termino)*
//   termino     -> factor ((*|/|%) factor)*
//   factor      -> '(' comparacion ')' | IDENT llamada? | literal
// ============================================================
class SyntaxAnalyzer {
public:
    explicit SyntaxAnalyzer(std::vector<Token> tokens) {
        // Filtramos tokens irrelevantes para la gramática.
        tokens_.reserve(tokens.size());
        for (auto& t : tokens) {
            if (t.type != TokenType::COMMENT &&
                t.type != TokenType::WHITESPACE &&
                t.type != TokenType::NEWLINE) {
                tokens_.push_back(std::move(t));
            }
        }
    }

    std::shared_ptr<Program> parse() {
        std::vector<StmtPtr> statements;
        while (currentToken() != nullptr && currentToken()->type != TokenType::TOKEN_EOF) {
            statements.push_back(parseStatement());
        }
        return std::make_shared<Program>(std::move(statements));
    }

private:
    std::vector<Token> tokens_;
    size_t position_ = 0;

    const Token* currentToken() const {
        if (position_ < tokens_.size()) return &tokens_[position_];
        return nullptr;
    }

    const Token* peekToken(int offset = 1) const {
        size_t idx = position_ + static_cast<size_t>(offset);
        if (idx < tokens_.size()) return &tokens_[idx];
        return nullptr;
    }

    Token consume(TokenType expected) {
        const Token* token = currentToken();
        if (token == nullptr || token->type == TokenType::TOKEN_EOF) {
            throw ParserSyntaxError(
                "Se esperaba " + tokenTypeToString(expected) + " pero se llego al final del codigo",
                token);
        }
        if (token->type != expected) {
            throw ParserSyntaxError(
                "Se esperaba " + tokenTypeToString(expected) + ", se encontro " + tokenTypeToString(token->type),
                token);
        }
        Token result = *token;
        position_ += 1;
        return result;
    }

    bool match(std::initializer_list<TokenType> types) const {
        const Token* token = currentToken();
        if (token == nullptr) return false;
        for (TokenType t : types) {
            if (token->type == t) return true;
        }
        return false;
    }

    StmtPtr parseStatement() {
        if (match({TokenType::IDENTIFIER}) && peekToken() != nullptr &&
            peekToken()->type == TokenType::ASSIGN) {
            return parseAssignment();
        }
        if (match({TokenType::IF})) return parseIfStatement();
        if (match({TokenType::WHILE})) return parseWhileStatement();
        if (match({TokenType::PRINT})) return parsePrintStatement();

        const Token* token = currentToken();
        throw ParserSyntaxError(
            "Sentencia inesperada comenzando con " + (token ? tokenTypeToString(token->type) : "EOF"),
            token);
    }

    StmtPtr parseAssignment() {
        Token identifier = consume(TokenType::IDENTIFIER);
        consume(TokenType::ASSIGN);
        ExprPtr value = parseComparison();
        return std::make_shared<Assignment>(identifier.value, value);
    }

    std::vector<StmtPtr> parseBlock() {
        consume(TokenType::LBRACE);
        std::vector<StmtPtr> statements;
        while (!match({TokenType::RBRACE})) {
            statements.push_back(parseStatement());
        }
        consume(TokenType::RBRACE);
        return statements;
    }

    StmtPtr parseIfStatement() {
        consume(TokenType::IF);
        ExprPtr condition = parseComparison();
        std::vector<StmtPtr> thenBody = parseBlock();

        std::vector<StmtPtr> elseBody;
        if (match({TokenType::ELSE})) {
            consume(TokenType::ELSE);
            elseBody = parseBlock();
        }
        return std::make_shared<IfStatement>(condition, thenBody, elseBody);
    }

    StmtPtr parseWhileStatement() {
        consume(TokenType::WHILE);
        ExprPtr condition = parseComparison();
        std::vector<StmtPtr> body = parseBlock();
        return std::make_shared<WhileStatement>(condition, body);
    }

    StmtPtr parsePrintStatement() {
        consume(TokenType::PRINT);
        consume(TokenType::LPAREN);
        ExprPtr expression = parseComparison();
        consume(TokenType::RPAREN);
        return std::make_shared<PrintStatement>(expression);
    }

    ExprPtr parseComparison() {
        ExprPtr left = parseExpression();
        static const std::initializer_list<TokenType> comparisonOps = {
            TokenType::EQUAL, TokenType::NOT_EQUAL, TokenType::LESS_THAN,
            TokenType::GREATER_THAN, TokenType::LESS_EQUAL, TokenType::GREATER_EQUAL,
            TokenType::AND, TokenType::OR
        };
        while (match(comparisonOps)) {
            std::string op = currentToken()->value;
            position_ += 1;
            ExprPtr right = parseExpression();
            left = std::make_shared<BinaryOperation>(left, op, right);
        }
        return left;
    }

    ExprPtr parseExpression() {
        ExprPtr left = parseTerm();
        while (match({TokenType::PLUS, TokenType::MINUS})) {
            std::string op = currentToken()->value;
            position_ += 1;
            left = std::make_shared<BinaryOperation>(left, op, parseTerm());
        }
        return left;
    }

    ExprPtr parseTerm() {
        ExprPtr left = parseFactor();
        while (match({TokenType::MULTIPLY, TokenType::DIVIDE, TokenType::MODULO})) {
            std::string op = currentToken()->value;
            position_ += 1;
            left = std::make_shared<BinaryOperation>(left, op, parseFactor());
        }
        return left;
    }

    ExprPtr parseFactor() {
        if (match({TokenType::LPAREN})) {
            consume(TokenType::LPAREN);
            ExprPtr expr = parseComparison();
            consume(TokenType::RPAREN);
            return expr;
        }

        if (match({TokenType::IDENTIFIER})) {
            std::string name = consume(TokenType::IDENTIFIER).value;
            if (match({TokenType::LPAREN})) {
                return parseFunctionCall(name);
            }
            return std::make_shared<Identifier>(name);
        }

        if (match({TokenType::NUMBER})) {
            Token token = consume(TokenType::NUMBER);
            return std::make_shared<Literal>(token.value, Literal::Kind::NUMBER);
        }

        if (match({TokenType::STRING})) {
            Token token = consume(TokenType::STRING);
            std::string stripped = token.value.size() >= 2
                ? token.value.substr(1, token.value.size() - 2)
                : token.value;
            return std::make_shared<Literal>(stripped, Literal::Kind::STRING);
        }

        if (match({TokenType::TRUE_LIT})) {
            consume(TokenType::TRUE_LIT);
            return std::make_shared<Literal>("True", Literal::Kind::BOOL_TRUE);
        }

        if (match({TokenType::FALSE_LIT})) {
            consume(TokenType::FALSE_LIT);
            return std::make_shared<Literal>("False", Literal::Kind::BOOL_FALSE);
        }

        const Token* token = currentToken();
        throw ParserSyntaxError(
            "Factor inesperado: " + (token ? tokenTypeToString(token->type) : "EOF"),
            token);
    }

    ExprPtr parseFunctionCall(const std::string& functionName) {
        consume(TokenType::LPAREN);
        std::vector<ExprPtr> arguments;
        if (!match({TokenType::RPAREN})) {
            arguments.push_back(parseComparison());
            while (match({TokenType::COMMA})) {
                consume(TokenType::COMMA);
                arguments.push_back(parseComparison());
            }
        }
        consume(TokenType::RPAREN);
        return std::make_shared<FunctionCall>(functionName, arguments);
    }
};

#endif // PARSER_H
