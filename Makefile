# Compilador e flags
CXX = g++
CXXFLAGS = -O3 -std=c++17 -Wall -Wextra

# Nome do executável
TARGET1 = 2queima

# Objetos que são comuns aos dois projetos
COMMON_OBJS = Graph.o brkgaAPI/Population.o

# Objetos específicos da Versão 1
OBJS1 = brkga-2queima.o Decoder2Queima.o $(COMMON_OBJS)

# Regra principal: compila o executável
all: $(TARGET1)

# Regra para o executável 1
$(TARGET1): $(OBJS1)
	$(CXX) $(CXXFLAGS) -o $(TARGET1) $(OBJS1)

# Regra genérica para compilar qualquer .cpp em .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpeza completa
clean:
	rm -f *.o brkgaAPI/*.o $(TARGET1) results.csv