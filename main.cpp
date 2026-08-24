#include <cstdio>
#include <string>
#include <vector>
#include "Lexer.h"
#include "Parser.h"

// Lee el contenido completo de un archivo usando la librería
// estándar de C (stdio.h).
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
    size_t readBytes = fread(&outContent[0], 1, static_cast<size_t>(size), file);
    fclose(file);

    outContent.resize(readBytes);
    return true;
}

static void printTokens(const std::vector<Token>& tokens) {
    printf("Tokens generados:\n");
    for (const auto& t : tokens) {
        printf("  %s\n", t.toString().c_str());
    }
}

static void processSource(const std::string& code) {
    LexicalAnalyzer lexer;
    std::vector<Token> tokens = lexer.tokenize(code);

    printTokens(tokens);

    SyntaxAnalyzer parser(tokens);
    try {
        std::shared_ptr<Program> ast = parser.parse();
        printf("\nAST:\n%s\n", ast->toString().c_str());
    } catch (const ParserSyntaxError& e) {
        printf("\n%s\n", e.what());
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

        printf("Archivo: %s\n", path.c_str());
        printf("========================================\n");
        processSource(source);
    } else {
        std::vector<std::string> ejemplos = {
            "x = 5 + 3",
            "resultado = (a + b) * 2",
            "if x > 0 { print(x) }",
            "contador = 0",
        };

        for (const auto& codigo : ejemplos) {
            printf("============================================================\n");
            printf("Codigo: %s\n", codigo.c_str());
            processSource(codigo);
        }

        printf("\n(No se paso ningun archivo .c como argumento, se corrieron ejemplos de prueba.\n");
        printf(" Uso real: %s ruta/al/archivo.c)\n", argv[0]);
    }

    return 0;
}
