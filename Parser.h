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
// Nodos del AST (Arbol de Sintaxis Abstracta)
// ------------------------------------------------------------
// Se usa std::shared_ptr en vez de punteros crudos para no tener
// que preocuparse por 'delete' ni por fugas de memoria.
// ============================================================

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual std::string toString() const = 0;
};

class Expression : public ASTNode {};   // produce un valor
class Statement  : public ASTNode {};   // produce un efecto

using ExprPtr = std::shared_ptr<Expression>;
using StmtPtr = std::shared_ptr<Statement>;

// izquierda OPERADOR derecha, p. ej. a + b
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

// OPERADOR operando, p. ej. -5 o "not x"
class UnaryOperation : public Expression {
public:
    std::string op;
    ExprPtr operand;

    UnaryOperation(std::string o, ExprPtr e)
        : op(std::move(o)), operand(std::move(e)) {}

    std::string toString() const override {
        return "UnaryOp(" + op + " " + operand->toString() + ")";
    }
};

// Valor literal: numero, cadena o booleano.
class Literal : public Expression {
public:
    enum class Kind { NUMBER, STRING, BOOL_TRUE, BOOL_FALSE, NONE_VALUE };

    std::string rawValue;   // texto tal cual aparece en el codigo
    Kind kind;

    Literal(std::string v, Kind k) : rawValue(std::move(v)), kind(k) {}

    // Conversion a numero (solo tiene sentido si kind == NUMBER).
    double asNumber() const { return std::stod(rawValue); }
    bool isInteger() const { return rawValue.find('.') == std::string::npos; }

    std::string toString() const override {
        return "Literal(" + rawValue + ")";
    }
};

// Referencia a una variable.
class Identifier : public Expression {
public:
    std::string name;
    explicit Identifier(std::string n) : name(std::move(n)) {}

    std::string toString() const override {
        return "Identifier(" + name + ")";
    }
};

// Llamada a funcion: nombre(args...)
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

// variable = expresion
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

// Un programa completo: lista de sentencias.
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

// Error de sintaxis con informacion de posicion.
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
// SyntaxAnalyzer (parser descendente recursivo)
// ------------------------------------------------------------
// GRAMATICA (de menor a mayor precedencia):
//
//   programa    -> sentencia*
//   sentencia   -> asignacion | if | while | print
//   bloque      -> '{' sentencia* '}'
//   comparacion -> expresion ((== != < > <= >= and or) expresion)*
//   expresion   -> termino ((+ -) termino)*
//   termino     -> unario ((* / %) unario)*
//   unario      -> (- + not) unario | potencia
//   potencia    -> factor ('**' unario)?          [asociativo a la derecha]
//   factor      -> '(' comparacion ')' | IDENT llamada? | literal
// ============================================================
class SyntaxAnalyzer {
public:
    explicit SyntaxAnalyzer(std::vector<Token> tokens) {
        // Se filtran los tokens irrelevantes para la gramatica.
        tokens_.reserve(tokens.size());
        for (auto& t : tokens) {
            if (t.type != TokenType::COMMENT &&
                t.type != TokenType::WHITESPACE &&
                t.type != TokenType::NEWLINE) {
                tokens_.push_back(std::move(t));
            }
        }
    }

    // Punto de entrada: programa -> sentencia*
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

    // ---------- utilidades de navegacion ----------

    const Token* currentToken() const {
        if (position_ < tokens_.size()) return &tokens_[position_];
        return nullptr;
    }

    const Token* peekToken(int offset = 1) const {
        size_t idx = position_ + static_cast<size_t>(offset);
        if (idx < tokens_.size()) return &tokens_[idx];
        return nullptr;
    }

    bool atEnd() const {
        const Token* t = currentToken();
        return t == nullptr || t->type == TokenType::TOKEN_EOF;
    }

    // Consume el token actual solo si es del tipo esperado; si no, error.
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

    // Avanza una posicion y devuelve el lexema del token que se dejo atras.
    std::string advanceAndGetLexeme() {
        std::string lexeme = currentToken()->value;
        position_ += 1;
        return lexeme;
    }

    // ---------- sentencias ----------

    StmtPtr parseStatement() {
        // Se necesita 1 token de anticipacion para distinguir
        // "x = ..." (asignacion) de "x + 1" (expresion suelta).
        if (match({TokenType::IDENTIFIER}) && peekToken() != nullptr &&
            peekToken()->type == TokenType::ASSIGN) {
            return parseAssignment();
        }
        if (match({TokenType::IF}))    return parseIfChain(TokenType::IF);
        if (match({TokenType::WHILE})) return parseWhileStatement();
        if (match({TokenType::PRINT})) return parsePrintStatement();

        const Token* token = currentToken();
        throw ParserSyntaxError(
            "Sentencia inesperada comenzando con " + (token ? tokenTypeToString(token->type) : std::string("EOF")),
            token);
    }

    StmtPtr parseAssignment() {
        Token identifier = consume(TokenType::IDENTIFIER);
        consume(TokenType::ASSIGN);
        ExprPtr value = parseComparison();
        return std::make_shared<Assignment>(identifier.value, value);
    }

    // bloque -> LLAVE_IZQ sentencia* LLAVE_DER
    std::vector<StmtPtr> parseBlock() {
        consume(TokenType::LBRACE);
        std::vector<StmtPtr> statements;
        while (!match({TokenType::RBRACE})) {
            // Sin esta guarda, un bloque sin cerrar produce un mensaje
            // de error confuso en vez de decir que falta la llave.
            if (atEnd()) {
                throw ParserSyntaxError("Falta la llave de cierre '}' del bloque", currentToken());
            }
            statements.push_back(parseStatement());
        }
        consume(TokenType::RBRACE);
        return statements;
    }

    // Maneja la cadena if / elif / elif / else de forma recursiva:
    // cada 'elif' se representa como un IfStatement dentro del else del anterior.
    StmtPtr parseIfChain(TokenType openingKeyword) {
        consume(openingKeyword);
        ExprPtr condition = parseComparison();
        std::vector<StmtPtr> thenBody = parseBlock();

        std::vector<StmtPtr> elseBody;
        if (match({TokenType::ELIF})) {
            elseBody.push_back(parseIfChain(TokenType::ELIF));
        } else if (match({TokenType::ELSE})) {
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

    // ---------- expresiones (por niveles de precedencia) ----------

    // comparacion -> expresion ((== != < > <= >= and or) expresion)*
    ExprPtr parseComparison() {
        ExprPtr left = parseExpression();
        while (match({TokenType::EQUAL, TokenType::NOT_EQUAL, TokenType::LESS_THAN,
                      TokenType::GREATER_THAN, TokenType::LESS_EQUAL, TokenType::GREATER_EQUAL,
                      TokenType::AND, TokenType::OR})) {
            std::string op = advanceAndGetLexeme();
            left = std::make_shared<BinaryOperation>(left, op, parseExpression());
        }
        return left;
    }

    // expresion -> termino ((+ -) termino)*
    ExprPtr parseExpression() {
        ExprPtr left = parseTerm();
        while (match({TokenType::PLUS, TokenType::MINUS})) {
            std::string op = advanceAndGetLexeme();
            left = std::make_shared<BinaryOperation>(left, op, parseTerm());
        }
        return left;
    }

    // termino -> unario ((* / %) unario)*
    ExprPtr parseTerm() {
        ExprPtr left = parseUnary();
        while (match({TokenType::MULTIPLY, TokenType::DIVIDE, TokenType::MODULO})) {
            std::string op = advanceAndGetLexeme();
            left = std::make_shared<BinaryOperation>(left, op, parseUnary());
        }
        return left;
    }

    // unario -> (- + not) unario | potencia
    // Permite escribir "x = -5" y "if not bandera { ... }".
    ExprPtr parseUnary() {
        if (match({TokenType::MINUS, TokenType::PLUS, TokenType::NOT})) {
            std::string op = advanceAndGetLexeme();
            return std::make_shared<UnaryOperation>(op, parseUnary());
        }
        return parsePower();
    }

    // potencia -> factor ('**' unario)?
    // Es asociativo a la DERECHA: 2 ** 3 ** 2 equivale a 2 ** (3 ** 2).
    ExprPtr parsePower() {
        ExprPtr base = parseFactor();
        if (match({TokenType::POWER})) {
            std::string op = advanceAndGetLexeme();
            return std::make_shared<BinaryOperation>(base, op, parseUnary());
        }
        return base;
    }

    // factor -> '(' comparacion ')' | IDENT llamada? | literal
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
            // Se quitan las comillas de apertura y cierre.
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

        if (match({TokenType::NONE_LIT})) {
            consume(TokenType::NONE_LIT);
            return std::make_shared<Literal>("None", Literal::Kind::NONE_VALUE);
        }

        const Token* token = currentToken();
        throw ParserSyntaxError(
            "Factor inesperado: " + (token ? tokenTypeToString(token->type) : std::string("EOF")),
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
