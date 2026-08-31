# Compilador e flags
CXX = g++
CXXFLAGS = -O3 -std=c++17 -fopenmp -march=native -mtune=native -Wall -Wextra -MMD -MP

# Nomes dos executáveis
TARGET1 = 2queima
TARGET2 = 2queimaV2
TARGET3 = 2queimaV3
TARGET4 = 2queimaV4

# Objetos que são comuns às quatro versões
COMMON_OBJS = Graph.o brkgaAPI/Population.o

# Objetos específicos da Versão 1
OBJS1 = brkga-2queima.o Decoder2Queima.o $(COMMON_OBJS)

# Objetos específicos da Versão 2
OBJS2 = brkga-2queimaV2.o Decoder2QueimaV2.o $(COMMON_OBJS)

# Objetos específicos da Versão 3 (grau relativo)
OBJS3 = brkga-2queimaV3.o Decoder2QueimaV3.o $(COMMON_OBJS)

# Objetos específicos da Versão 4 (heurística de gatilho)
OBJS4 = brkga-2queimaV4.o Decoder2QueimaV4.o $(COMMON_OBJS)

# Dependências de cabeçalhos geradas automaticamente pelo compilador
DEPS = $(sort $(OBJS1:.o=.d) $(OBJS2:.o=.d) $(OBJS3:.o=.d) $(OBJS4:.o=.d))

# Regra principal: compila os quatro executáveis
all: $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4)

# Regra para o executável 1
$(TARGET1): $(OBJS1)
	$(CXX) $(CXXFLAGS) -o $(TARGET1) $(OBJS1)

# Regra para o executável 2
$(TARGET2): $(OBJS2)
	$(CXX) $(CXXFLAGS) -o $(TARGET2) $(OBJS2)

# Regra para o executável 3
$(TARGET3): $(OBJS3)
	$(CXX) $(CXXFLAGS) -o $(TARGET3) $(OBJS3)

# Regra para o executável 4
$(TARGET4): $(OBJS4)
	$(CXX) $(CXXFLAGS) -o $(TARGET4) $(OBJS4)

# Regra genérica para compilar qualquer .cpp em .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpeza completa
clean:
	rm -f *.o *.d brkgaAPI/*.o brkgaAPI/*.d $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4) results.csv

-include $(DEPS)
