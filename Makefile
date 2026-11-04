# Compilador e flags
CXX = g++
CXXFLAGS = -O3 -std=c++17 -fopenmp -march=native -mtune=native -flto -DNDEBUG -Wall -Wextra -MMD -MP -I.
TEST_CXXFLAGS = $(filter-out -DNDEBUG,$(CXXFLAGS))

# Nomes dos executáveis
TARGET1 = 2queima
TARGET2 = 2queimaV2
TARGET3 = 2queimaV3
TARGET4 = 2queimaV4
TARGET5 = 2queimaV5
TARGET6 = 2queimaV6
TARGET_V1_INICIAL = 2queimaV1Inicial
TARGET_TESTE_INICIAL = test_populacao_inicial
TARGET_TESTE_V4 = test_decoder_v4
TARGET_TESTE_V5 = test_decoder_v5
TARGET_TESTE_V6 = test_decoder_v6

# Objetos que são comuns às seis versões
COMMON_OBJS = Graph.o brkgaAPI/Population.o

# Objetos específicos da Versão 1
OBJS1 = brkga-2queima.o Decoder2Queima.o $(COMMON_OBJS)

# Objetos específicos da Versão 2
OBJS2 = brkga-2queimaV2.o Decoder2QueimaV2.o $(COMMON_OBJS)

# Objetos específicos da Versão 3 (grau relativo)
OBJS3 = brkga-2queimaV3.o Decoder2QueimaV3.o $(COMMON_OBJS)

# Objetos específicos da Versão 4 (heurística de gatilho)
OBJS4 = brkga-2queimaV4.o Decoder2QueimaV4.o $(COMMON_OBJS)

# Objetos específicos da Versão 5 (grau relativo e gatilho)
OBJS5 = brkga-2queimaV5.o Decoder2QueimaV5.o $(COMMON_OBJS)

# Objetos específicos da Versão 6 (permutação pura)
OBJS6 = brkga-2queimaV6.o Decoder2QueimaV6.o $(COMMON_OBJS)

# Versao experimental da V1 com uma solucao heuristica na populacao inicial
OBJS_V1_INICIAL = brkga-2queimaV1Inicial.o Decoder2Queima.o \
	HeuristicaInicialGVNS.o $(COMMON_OBJS)

# Dependências de cabeçalhos geradas automaticamente pelo compilador
DEPS = $(sort $(OBJS1:.o=.d) $(OBJS2:.o=.d) $(OBJS3:.o=.d) $(OBJS4:.o=.d) $(OBJS5:.o=.d) $(OBJS6:.o=.d) $(OBJS_V1_INICIAL:.o=.d) tests/test_populacao_inicial.d tests/test_decoder_v4.d tests/test_decoder_v5.d tests/test_decoder_v6.d)

# Regra principal: compila os seis executáveis
all: $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4) $(TARGET5) $(TARGET6)

# Este alvo e separado de "all" porque e uma variante da V1 com populacao inicial.
inicial: $(TARGET_V1_INICIAL)

teste-inicial: $(TARGET_TESTE_INICIAL)
	./$(TARGET_TESTE_INICIAL)

teste-v4: $(TARGET_TESTE_V4)
	./$(TARGET_TESTE_V4)

teste-v5: $(TARGET_TESTE_V5)
	./$(TARGET_TESTE_V5)

teste-v6: $(TARGET_TESTE_V6)
	./$(TARGET_TESTE_V6)

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

# Regra para o executável 5
$(TARGET5): $(OBJS5)
	$(CXX) $(CXXFLAGS) -o $(TARGET5) $(OBJS5)

# Regra para o executável 6
$(TARGET6): $(OBJS6)
	$(CXX) $(CXXFLAGS) -o $(TARGET6) $(OBJS6)

$(TARGET_V1_INICIAL): $(OBJS_V1_INICIAL)
	$(CXX) $(CXXFLAGS) -o $(TARGET_V1_INICIAL) $(OBJS_V1_INICIAL)

$(TARGET_TESTE_INICIAL): tests/test_populacao_inicial.o Decoder2Queima.o \
		HeuristicaInicialGVNS.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET_TESTE_INICIAL) $^

$(TARGET_TESTE_V4): tests/test_decoder_v4.o Decoder2QueimaV4.o Graph.o
	$(CXX) $(CXXFLAGS) -o $(TARGET_TESTE_V4) $^

$(TARGET_TESTE_V5): tests/test_decoder_v5.o Decoder2QueimaV5.o Graph.o
	$(CXX) $(CXXFLAGS) -o $(TARGET_TESTE_V5) $^

$(TARGET_TESTE_V6): tests/test_decoder_v6.o Decoder2QueimaV6.o Graph.o
	$(CXX) $(CXXFLAGS) -o $(TARGET_TESTE_V6) $^

tests/test_populacao_inicial.o: tests/test_populacao_inicial.cpp
	$(CXX) $(TEST_CXXFLAGS) -Iheuristica_inicial -c $< -o $@

tests/test_decoder_v4.o: tests/test_decoder_v4.cpp
	$(CXX) $(TEST_CXXFLAGS) -c $< -o $@

tests/test_decoder_v5.o: tests/test_decoder_v5.cpp
	$(CXX) $(TEST_CXXFLAGS) -c $< -o $@

tests/test_decoder_v6.o: tests/test_decoder_v6.cpp
	$(CXX) $(TEST_CXXFLAGS) -c $< -o $@

HeuristicaInicialGVNS.o: heuristica_inicial/HeuristicaInicial.cpp \
		heuristica_inicial/HeuristicaInicial.h SimulacaoQueima.h Graph.h
	$(CXX) $(CXXFLAGS) -I. -Iheuristica_inicial -c $< -o $@

# Regra genérica para compilar qualquer .cpp em .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpeza completa
clean:
	rm -f *.o *.d tests/*.o tests/*.d brkgaAPI/*.o brkgaAPI/*.d $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4) $(TARGET5) $(TARGET6) $(TARGET_V1_INICIAL) $(TARGET_TESTE_INICIAL) $(TARGET_TESTE_V4) $(TARGET_TESTE_V5) $(TARGET_TESTE_V6) results.csv

-include $(DEPS)
