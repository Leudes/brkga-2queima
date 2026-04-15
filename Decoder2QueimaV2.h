#ifndef DECODER_2QUEIMA_H
#define DECODER_2QUEIMA_H

#include <vector>
#include <numeric>   // Necessário para o std::iota
#include <algorithm>
#include "Graph.h"

class Decoder2Queima {
public:
	// Alteramos o construtor para inicializar a lista pré-ordenada
	Decoder2Queima(const Graph& graph);	
	~Decoder2Queima() = default;	        
    
    double decode(const std::vector< double >& chromosome) const;
    std::vector<int> get_burn_sequence(const std::vector< double >& chromosome) const;

private:
	const Graph& g;
    // NOVA LISTA: Guarda os vértices ordenados do maior para o menor grau
    std::vector<int> vertices_sorted_by_degree; 
};

#endif