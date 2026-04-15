# Compilador e flags
CXX = g++
CXXFLAGS = -O3 -std=c++17 -Wall -Wextra

# Nomes dos executáveis
TARGET1 = 2queima
TARGET2 = 2queimaV2

# Objetos que são comuns aos dois projetos
COMMON_OBJS = Graph.o brkgaAPI/Population.o

# Objetos específicos da Versão 1
OBJS1 = brkga-2queima.o Decoder2Queima.o $(COMMON_OBJS)

# Objetos específicos da Versão 2
OBJS2 = brkga-2queimaV2.o Decoder2QueimaV2.o $(COMMON_OBJS)

# Regra principal: compila os dois executáveis
all: $(TARGET1) $(TARGET2)

# Regra para o executável 1
$(TARGET1): $(OBJS1)
	$(CXX) $(CXXFLAGS) -o $(TARGET1) $(OBJS1)

# Regra para o executável 2
$(TARGET2): $(OBJS2)
	$(CXX) $(CXXFLAGS) -o $(TARGET2) $(OBJS2)

# Regra genérica para compilar qualquer .cpp em .o
# O Makefile cuida de achar brkga-2queimaV2.cpp e transformar em .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpeza completa
clean:
	rm -f *.o brkgaAPI/*.o $(TARGET1) $(TARGET2) results.csv