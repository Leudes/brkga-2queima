#ifndef DECODER_2QUEIMA_H
#define DECODER_2QUEIMA_H

#include <vector>
#include <algorithm>
#include "Graph.h"

class Decoder2Queima {
public:
	Decoder2Queima(const Graph& graph) : g{graph} { }	
	~Decoder2Queima() = default;	        
    
    double decode(const std::vector< double >& chromosome) const;
    std::vector<int> get_burn_sequence(const std::vector< double >& chromosome) const;

private:
	const Graph& g;
};

#endif