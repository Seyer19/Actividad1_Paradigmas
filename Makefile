# Makefile del analizador lexico + sintactico
# Uso:
#   make          -> compila
#   make run      -> compila y corre los ejemplos internos
#   make ejemplo  -> compila y analiza ejemplos/ejemplo1.txt
#   make clean    -> borra el binario

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
TARGET   = compilador
SRC      = main.cpp
HEADERS  = Lexer.h Parser.h

$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

ejemplo: $(TARGET)
	./$(TARGET) ejemplos/ejemplo1.txt

clean:
	rm -f $(TARGET) $(TARGET).exe

.PHONY: run ejemplo clean
