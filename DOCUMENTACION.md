# Analizador Lexico + Sintactico en C++

Traduccion a C++ de los dos scripts de Python (`lexer.py` y `parser.py`).
Implementa las dos primeras fases de un compilador sobre un subconjunto
simplificado de Python.

---

## 1. Que hace el programa

```
   CODIGO FUENTE                "x = 5 + 3"
        |
        v
+---------------------------+
|  FASE 1                   |   LexicalAnalyzer  (Lexer.h)
|  Analisis LEXICO          |   ------------------------------
|  texto  ->  tokens        |   Recorre el texto con expresiones
+---------------------------+   regulares y produce una lista plana
        |                       de Tokens con tipo, lexema, linea y
        |                       columna.
        v
   LISTA DE TOKENS
   [IDENTIFICADOR 'x'] [ASIGNACION '='] [NUMERO '5'] [SUMA '+'] [NUMERO '3'] [EOF]
        |
        v
+---------------------------+
|  FASE 2                   |   SyntaxAnalyzer  (Parser.h)
|  Analisis SINTACTICO      |   ------------------------------
|  tokens  ->  AST          |   Parser descendente recursivo: una
+---------------------------+   funcion por regla de la gramatica.
        |
        v
   ARBOL DE SINTAXIS ABSTRACTA (AST)
   Program([
     Assignment(x = BinaryOp(Literal(5) + Literal(3)))
   ])
```

Las fases 3 y 4 de un compilador real (analisis semantico y generacion de
codigo) **no** forman parte de esta actividad.

### Precedencia de operadores implementada

De menor a mayor fuerza. Cada nivel es una funcion del parser.

| Nivel | Regla         | Operadores                          | Funcion            |
|-------|---------------|-------------------------------------|--------------------|
| 1     | `comparacion` | `== != < > <= >= and or`            | `parseComparison`  |
| 2     | `expresion`   | `+ -` (binarios)                    | `parseExpression`  |
| 3     | `termino`     | `* / %`                             | `parseTerm`        |
| 4     | `unario`      | `- + not` (prefijos)                | `parseUnary`       |
| 5     | `potencia`    | `**` (asociativo a la derecha)      | `parsePower`       |
| 6     | `factor`      | `( )`, literales, variables, llamadas | `parseFactor`    |

---

## 2. Como compilarlo y correrlo

### Opcion A: linea de comandos (la mas simple)

```bash
g++ -std=c++17 -Wall -Wextra -g main.cpp -o compilador
./compilador                          # corre los ejemplos internos
./compilador ejemplos/ejemplo1.txt    # analiza un archivo
```

En Windows el binario se llama `compilador.exe` y se ejecuta con
`.\compilador.exe`.

**Solo se compila `main.cpp`.** `Lexer.h` y `Parser.h` NO se compilan por
separado: son cabeceras y entran al programa por medio de `#include`.
Intentar `g++ Lexer.h` es uno de los errores mas comunes.

### Opcion B: Makefile

```bash
make          # compila
make run      # compila y corre los ejemplos internos
make ejemplo  # compila y analiza ejemplos/ejemplo1.txt
make clean    # borra el binario
```

### Opcion C: Visual Studio Code

1. Instalar la extension **C/C++** de Microsoft (`ms-vscode.cpptools`).
2. Instalar un compilador y dejarlo en el PATH:
   - **Windows:** MSYS2 -> `pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-gdb`
     y agregar `C:\msys64\ucrt64\bin` a la variable de entorno PATH.
   - **macOS:** `xcode-select --install`
   - **Linux:** `sudo apt install build-essential gdb`
3. Verificar en una terminal nueva: `g++ --version` debe responder algo.
4. Abrir en VS Code **la carpeta completa** del proyecto
   (`Archivo > Abrir carpeta...`), no el archivo suelto.
5. `Ctrl+Shift+B` para compilar, `F5` para depurar.

Las configuraciones de la carpeta `.vscode/` ya vienen listas:

| Archivo                  | Para que sirve                                        |
|--------------------------|-------------------------------------------------------|
| `tasks.json`             | Tarea de compilacion con `Ctrl+Shift+B`               |
| `launch.json`            | Depuracion con `F5` (puntos de interrupcion, variables) |
| `c_cpp_properties.json`  | IntelliSense: evita los subrayados rojos falsos       |

---

## 3. Correcciones aplicadas respecto a la version que entregaste

| # | Problema                                                                 | Efecto que causaba                                                                 | Correccion |
|---|--------------------------------------------------------------------------|------------------------------------------------------------------------------------|------------|
| 1 | `std::getline` no eliminaba el `\r` de los archivos guardados en Windows | Cada linea de un archivo CRLF terminaba en un token `DESCONOCIDO` y el parser fallaba | Funcion `splitLines()` que descarta `\r` |
| 2 | El menos unario no existia en la gramatica                               | `x = -5` daba "Factor inesperado: RESTA"                                            | Regla `unario` + nodo `UnaryOperation` |
| 3 | El token `POTENCIA` se generaba pero el parser nunca lo consumia         | `x = 2 ** 3` daba "Sentencia inesperada comenzando con POTENCIA"                    | Regla `potencia`, asociativa a la derecha |
| 4 | Faltaba `elif`, que si existe como token                                 | Un `elif` era rechazado por el parser                                               | `parseIfChain()` recursiva |
| 5 | Faltaba el metodo `classify()` que si estaba en el Python                | La traduccion estaba incompleta respecto al original                                | `LexicalAnalyzer::classify()` |
| 6 | El token `FIN_DE_ARCHIVO` reportaba `lineNum - 1`                        | Numero de linea incorrecto en errores al final del archivo                           | Se usa `lines.size()` |
| 7 | Un bloque sin `}` producia un mensaje confuso                            | Decia "Sentencia inesperada comenzando con FIN_DE_ARCHIVO"                          | Guarda explicita: "Falta la llave de cierre" |
| 8 | Comentarios con acentos en el codigo fuente                              | Advertencia C4819 y caracteres corruptos en MSVC sin BOM                            | Comentarios sin acentos |
| 9 | `fread` sobre un archivo vacio                                           | Lectura de `&outContent[0]` con tamano 0                                            | Guarda `size > 0` |

**Importante:** el codigo que entregaste **si compilaba**. Se verifico con
`g++ -std=c++17 -Wall -Wextra` sin un solo error ni advertencia. Si no lo
podias correr en VS Code, el problema era el entorno (compilador no instalado
o no configurado), no el programa.

---

## 4. Limitaciones conocidas

Estas limitaciones tambien existen en la version original en Python. Se dejan
tal cual porque salen del alcance de la actividad, pero conviene saberlas por
si el profesor pregunta:

- El AST se construye pero **no se evalua**: no hay interprete ni tabla de simbolos.
- No se soportan `def`, `class`, `for`, `return` ni `in`, aunque sus tokens si existen.
- `+=` y `-=` se reconocen como tokens pero no tienen regla en la gramatica.
- Los bloques usan llaves `{ }`, no indentacion como Python real.
- Una expresion suelta (`x + 1` sin asignar) no es una sentencia valida.
- Las cadenas no admiten secuencias de escape (`\"` dentro de comillas).

---

## 5. Estructura del proyecto

```
compilador/
├── Lexer.h                       Fase 1: TokenType, Token, LexicalAnalyzer
├── Parser.h                      Fase 2: nodos del AST y SyntaxAnalyzer
├── main.cpp                      Punto de entrada (unico archivo que se compila)
├── Makefile                      Atajos de compilacion
├── README.md                     Este archivo
├── .vscode/
│   ├── tasks.json
│   ├── launch.json
│   └── c_cpp_properties.json
└── ejemplos/
    ├── ejemplo1.txt              Programa valido y completo
    └── ejemplo2_con_error.txt    Programa con error de sintaxis a proposito
```
