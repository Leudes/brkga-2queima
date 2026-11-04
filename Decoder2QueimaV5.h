#ifndef DECODER_2QUEIMA_V5_H
#define DECODER_2QUEIMA_V5_H

#include "Graph.h"
#include <vector>

// V5: usa primeiro os vertices com chave maior ou igual a 0,5. Quando essas
// fontes acabam, combina grau relativo e gatilhos para reparar a solucao.
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
