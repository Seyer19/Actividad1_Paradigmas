#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <map>
#include <regex>
#include <sstream>

// ============================================================
// TokenType + Token
// ------------------------------------------------------------
// Este es el "contrato" entre lexer y parser: ambos archivos
// (Lexer.h y Parser.h) deben coincidir en cómo se ve un Token.
// ============================================================
enum class TokenType {
    IDENTIFIER, NUMBER, STRING,
    PLUS, MINUS, MULTIPLY, DIVIDE, MODULO, POWER,
    ASSIGN, PLUS_ASSIGN, MINUS_ASSIGN,
    EQUAL, NOT_EQUAL, LESS_THAN, GREATER_THAN, LESS_EQUAL, GREATER_EQUAL,
    AND, OR, NOT,
    LPAREN, RPAREN, LBRACKET, RBRACKET, LBRACE, RBRACE,
    COMMA, SEMICOLON, DOT, COLON,
    IF, ELSE, ELIF, WHILE, FOR, DEF, CLASS, RETURN, PRINT,
    TRUE_LIT, FALSE_LIT, NONE_LIT, IN,
    NEWLINE, WHITESPACE, COMMENT, UNKNOWN,
    TOKEN_EOF   // se llama TOKEN_EOF y no EOF porque EOF ya es una
                // macro de <cstdio> (vale -1) y chocaría con ella.
};

inline std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::IDENTIFIER:    return "IDENTIFICADOR";
        case TokenType::NUMBER:        return "NUMERO";
        case TokenType::STRING:        return "CADENA";
        case TokenType::PLUS:          return "SUMA";
        case TokenType::MINUS:         return "RESTA";
        case TokenType::MULTIPLY:      return "MULTIPLICACION";
        case TokenType::DIVIDE:        return "DIVISION";
        case TokenType::MODULO:        return "MODULO";
        case TokenType::POWER:         return "POTENCIA";
        case TokenType::ASSIGN:        return "ASIGNACION";
        case TokenType::PLUS_ASSIGN:   return "SUMA_ASIGNACION";
        case TokenType::MINUS_ASSIGN:  return "RESTA_ASIGNACION";
        case TokenType::EQUAL:         return "IGUAL";
        case TokenType::NOT_EQUAL:     return "NO_IGUAL";
        case TokenType::LESS_THAN:     return "MENOR_QUE";
        case TokenType::GREATER_THAN:  return "MAYOR_QUE";
        case TokenType::LESS_EQUAL:    return "MENOR_IGUAL";
        case TokenType::GREATER_EQUAL: return "MAYOR_IGUAL";
        case TokenType::AND:           return "Y_LOGICO";
        case TokenType::OR:            return "O_LOGICO";
        case TokenType::NOT:           return "NO_LOGICO";
        case TokenType::LPAREN:        return "PARENTESIS_IZQ";
        case TokenType::RPAREN:        return "PARENTESIS_DER";
        case TokenType::LBRACKET:      return "CORCHETE_IZQ";
        case TokenType::RBRACKET:      return "CORCHETE_DER";
        case TokenType::LBRACE:        return "LLAVE_IZQ";
        case TokenType::RBRACE:        return "LLAVE_DER";
        case TokenType::COMMA:         return "COMA";
        case TokenType::SEMICOLON:     return "PUNTO_COMA";
        case TokenType::DOT:           return "PUNTO";
        case TokenType::COLON:         return "DOS_PUNTOS";
        case TokenType::IF:            return "SI";
        case TokenType::ELSE:          return "SINO";
        case TokenType::ELIF:          return "SINO_SI";
        case TokenType::WHILE:         return "MIENTRAS";
        case TokenType::FOR:           return "PARA";
        case TokenType::DEF:           return "FUNCION";
        case TokenType::CLASS:         return "CLASE";
        case TokenType::RETURN:        return "RETORNO";
        case TokenType::PRINT:         return "IMPRIMIR";
        case TokenType::TRUE_LIT:      return "VERDADERO";
        case TokenType::FALSE_LIT:     return "FALSO";
        case TokenType::NONE_LIT:      return "NULO";
        case TokenType::IN:            return "EN";
        case TokenType::NEWLINE:       return "NUEVA_LINEA";
        case TokenType::WHITESPACE:    return "ESPACIO";
        case TokenType::COMMENT:       return "COMENTARIO";
        case TokenType::UNKNOWN:       return "DESCONOCIDO";
        case TokenType::TOKEN_EOF:     return "FIN_DE_ARCHIVO";
    }
    return "DESCONOCIDO";
}

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;

    Token(TokenType t, std::string v, int l, int c)
        : type(t), value(std::move(v)), line(l), column(c) {}

    std::string toString() const {
        return "Token(" + tokenTypeToString(type) + ", '" + value + "', " +
               std::to_string(line) + ":" + std::to_string(column) + ")";
    }
};

// ============================================================
// LexicalAnalyzer
// ------------------------------------------------------------
// Traducción directa del lexer en Python: usa expresiones
// regulares en el ORDEN correcto (lo más específico primero)
// para respetar la regla de "longest match".
// ============================================================
class LexicalAnalyzer {
public:
    LexicalAnalyzer() {
        // IMPORTANTE: el orden importa. Los patrones más específicos
        // van primero (p. ej. "==" antes que "=", decimales antes que
        // enteros, "**" antes que "*").
        patterns_ = {
            {std::regex(R"(#.*)"),                       TokenType::COMMENT},

            {std::regex(R"(\d+\.\d+)"),                  TokenType::NUMBER},
            {std::regex(R"(\d+)"),                       TokenType::NUMBER},

            {std::regex(R"("[^"]*")"),                   TokenType::STRING},
            {std::regex(R"('[^']*')"),                   TokenType::STRING},

            {std::regex(R"(\+=)"),                       TokenType::PLUS_ASSIGN},
            {std::regex(R"(-=)"),                        TokenType::MINUS_ASSIGN},

            {std::regex(R"(==)"),                        TokenType::EQUAL},
            {std::regex(R"(!=)"),                        TokenType::NOT_EQUAL},
            {std::regex(R"(<=)"),                        TokenType::LESS_EQUAL},
            {std::regex(R"(>=)"),                        TokenType::GREATER_EQUAL},
            {std::regex(R"(<)"),                         TokenType::LESS_THAN},
            {std::regex(R"(>)"),                         TokenType::GREATER_THAN},

            {std::regex(R"(\*\*)"),                      TokenType::POWER},
            {std::regex(R"(\+)"),                        TokenType::PLUS},
            {std::regex(R"(-)"),                         TokenType::MINUS},
            {std::regex(R"(\*)"),                        TokenType::MULTIPLY},
            {std::regex(R"(/)"),                         TokenType::DIVIDE},
            {std::regex(R"(%)"),                         TokenType::MODULO},
            {std::regex(R"(=)"),                         TokenType::ASSIGN},

            {std::regex(R"(\()"),                        TokenType::LPAREN},
            {std::regex(R"(\))"),                        TokenType::RPAREN},
            {std::regex(R"(\[)"),                        TokenType::LBRACKET},
            {std::regex(R"(\])"),                        TokenType::RBRACKET},
            {std::regex(R"(\{)"),                        TokenType::LBRACE},
            {std::regex(R"(\})"),                        TokenType::RBRACE},

            {std::regex(R"(,)"),                         TokenType::COMMA},
            {std::regex(R"(;)"),                         TokenType::SEMICOLON},
            {std::regex(R"(\.)"),                        TokenType::DOT},
            {std::regex(R"(:)"),                         TokenType::COLON},

            {std::regex(R"([a-zA-Z_][a-zA-Z0-9_]*)"),    TokenType::IDENTIFIER},

            {std::regex(R"([ \t]+)"),                    TokenType::WHITESPACE},
        };

        keywords_ = {
            {"if", TokenType::IF},       {"elif", TokenType::ELIF},   {"else", TokenType::ELSE},
            {"while", TokenType::WHILE}, {"for", TokenType::FOR},     {"def", TokenType::DEF},
            {"class", TokenType::CLASS}, {"return", TokenType::RETURN}, {"print", TokenType::PRINT},
            {"True", TokenType::TRUE_LIT}, {"False", TokenType::FALSE_LIT}, {"None", TokenType::NONE_LIT},
            {"in", TokenType::IN}, {"and", TokenType::AND}, {"or", TokenType::OR}, {"not", TokenType::NOT},
        };
    }

    // Convierte el código fuente en una lista de tokens.
    std::vector<Token> tokenize(const std::string& code, bool includeWhitespace = false) const {
        std::vector<Token> tokens;

        std::vector<std::string> lines;
        std::stringstream ss(code);
        std::string line;
        while (std::getline(ss, line)) {
            lines.push_back(line);
        }
        if (code.empty()) {
            lines.push_back("");
        }

        int lineNum = 1;
        for (const auto& currentLine : lines) {
            size_t pos = 0;
            int column = 1;

            while (pos < currentLine.size()) {
                bool matched = false;

                for (const auto& pattern : patterns_) {
                    std::smatch match;
                    auto begin = currentLine.cbegin() + static_cast<long>(pos);
                    auto end = currentLine.cend();

                    // match_continuous obliga a que el match empiece
                    // EXACTAMENTE en 'pos' (equivalente a re.match en Python).
                    if (std::regex_search(begin, end, match, pattern.regex,
                                           std::regex_constants::match_continuous)) {
                        std::string value = match.str(0);
                        TokenType type = pattern.type;

                        if (type == TokenType::IDENTIFIER) {
                            auto it = keywords_.find(value);
                            if (it != keywords_.end()) {
                                type = it->second;
                            }
                        }

                        if (includeWhitespace || (type != TokenType::WHITESPACE)) {
                            tokens.emplace_back(type, value, lineNum, column);
                        }

                        pos += value.size();
                        column += static_cast<int>(value.size());
                        matched = true;
                        break;
                    }
                }

                if (!matched) {
                    tokens.emplace_back(TokenType::UNKNOWN, std::string(1, currentLine[pos]), lineNum, column);
                    pos += 1;
                    column += 1;
                }
            }
            lineNum++;
        }

        tokens.emplace_back(TokenType::TOKEN_EOF, "", lineNum - 1, 1);
        return tokens;
    }

private:
    struct Pattern {
        std::regex regex;
        TokenType type;
    };

    std::vector<Pattern> patterns_;
    std::map<std::string, TokenType> keywords_;
};

#endif // LEXER_H
