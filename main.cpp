// ============================================================
// PROGRAMA PRINCIPAL
// ------------------------------------------------------------
// Uso:
//   ./compilador                    -> corre los ejemplos internos
//   ./compilador archivo.txt        -> analiza el archivo indicado
// ============================================================
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include "Lexer.h"
#include "Parser.h"

// Lee el contenido completo de un archivo usando la libreria
// estandar de C (stdio.h).
static bool readFileWithStdio(const std::string& path, std::string& outContent) {
    FILE* file = fopen(path.c_str(), "rb");
    if (file == nullptr) {
        return false;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size < 0) {
        fclose(file);
        return false;
    }

    outContent.resize(static_cast<size_t>(size));
    size_t readBytes = (size > 0)
        ? fread(&outContent[0], 1, static_cast<size_t>(size), file)
        : 0;
    fclose(file);

    outContent.resize(readBytes);
    return true;
}

// FASE 1: mostrar la lista de tokens.
static void printTokens(const std::vector<Token>& tokens) {
    printf("\n[FASE 1] Analisis lexico - tokens generados:\n");
    for (const auto& t : tokens) {
        printf("  %s\n", t.toString().c_str());
    }
}

// Resumen por categorias (equivalente a classify() en Python).
static void printClassification(const LexicalAnalyzer& lexer, const std::vector<Token>& tokens) {
    printf("\n[FASE 1b] Clasificacion por categoria:\n");
    std::map<std::string, std::vector<Token>> grupos = lexer.classify(tokens);
    for (const auto& par : grupos) {
        if (par.second.empty()) continue;
        printf("  %-22s: ", par.first.c_str());
        for (size_t i = 0; i < par.second.size(); ++i) {
            if (i > 0) printf(", ");
            printf("%s", par.second[i].value.c_str());
        }
        printf("\n");
    }
}

// FASE 2: construir y mostrar el AST.
static void processSource(const std::string& code) {
    LexicalAnalyzer lexer;
    std::vector<Token> tokens = lexer.tokenize(code);

    printTokens(tokens);
    printClassification(lexer, tokens);

    SyntaxAnalyzer parser(tokens);
    try {
        std::shared_ptr<Program> ast = parser.parse();
        printf("\n[FASE 2] Analisis sintactico - AST:\n%s\n", ast->toString().c_str());
    } catch (const ParserSyntaxError& e) {
        printf("\n[FASE 2] %s\n", e.what());
    }
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string path = argv[1];
        std::string source;

        if (!readFileWithStdio(path, source)) {
            fprintf(stderr, "No se pudo abrir el archivo: %s\n", path.c_str());
            return 1;
        }

        printf("============================================================\n");
        printf("Archivo: %s\n", path.c_str());
        printf("============================================================\n");
        processSource(source);
    } else {
        std::vector<std::string> ejemplos = {
            "x = 5 + 3",
            "resultado = (a + b) * 2",
            "if x > 0 { print(x) }",
            "contador = 0",
            "y = -4 + 2 ** 3",                                  // unario y potencia
            "if a > 1 { print(1) } elif a > 0 { print(0) } else { print(-1) }",
        };

        for (const auto& codigo : ejemplos) {
            printf("============================================================\n");
            printf("Codigo: %s\n", codigo.c_str());
            printf("============================================================\n");
            processSource(codigo);
            printf("\n");
        }

        printf("(No se paso ningun archivo como argumento; se corrieron los ejemplos internos.\n");
        printf(" Uso real: %s ruta/al/archivo.txt)\n", argv[0]);
    }

    return 0;
}
