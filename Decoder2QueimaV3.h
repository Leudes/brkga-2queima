#ifndef DECODER_2QUEIMA_V3_H
#define DECODER_2QUEIMA_V3_H

#include "Graph.h"
#include <vector>

// V3: usa primeiro os vertices com chave maior ou igual a 0,5. O grau
// relativo somente repara a solucao quando essas fontes acabam.
class Decodificador2Queima {
public:
    explicit Decodificador2Queima(const Graph& grafo_recebido)
        : grafo{grafo_recebido} {}
    ~Decodificador2Queima() = default;

    double decode(const std::vector<double>& cromossomo) const;
    std::vector<int> obter_sequencia_queima(
        const std::vector<double>& cromossomo) const;

private:
    const Graph& grafo;
};

#endif
