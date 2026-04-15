#include "Decoder2QueimaV2.h"

Decoder2Queima::Decoder2Queima(const Graph& graph) : g{graph} {
    int n = g.getOrder();
    vertices_sorted_by_degree.resize(n);
    
    // Preenche com 0, 1, 2, ..., n-1
    std::iota(vertices_sorted_by_degree.begin(), vertices_sorted_by_degree.end(), 0);
    
    // Ordena os vértices com base no grau (Decrescente: do maior pro menor)
    std::sort(vertices_sorted_by_degree.begin(), vertices_sorted_by_degree.end(),
        [&](int a, int b) { 
            return g.getVertexDegree(a) > g.getVertexDegree(b); 
        });
}

double Decoder2Queima::decode(const std::vector< double >& chromosome) const {
    const int n = g.getOrder();

    // vector dos vertices que serão queimados por queima direta
    std::vector<int> direct_burn;
    direct_burn.reserve(n);

    // selecionando os vertices que serão queimados por queima direta
    for (int i = 0; i < n; ++i) {
        if (chromosome[i] >= 0.5) {
            direct_burn.push_back(i);
        }
    }

    // dando um sort no vector (não acho que seja necessário mas organiza)
    std::sort(direct_burn.begin(), direct_burn.end(),
        [&](int a, int b) { return chromosome[a] > chromosome[b]; });


    // setando variáveis e vectors
    std::vector<bool> burned(n, false);
    std::vector<int> burned_neighbors_count(n, 0);
    std::vector<int> spread_queue;
    std::vector<int> next_spread_queue;
    
    int total_burned = 0;
    int rounds = 0;
    size_t seq_idx = 0;
    size_t repair_idx = 0;

    spread_queue.reserve(n);
    next_spread_queue.reserve(n);

    // algoritmo da queima
    while (total_burned < n) {
        bool changed = false;
        rounds++;

        // queima indireta
        if (!spread_queue.empty()) {
            for (int v : spread_queue) {
                if (!burned[v]) {
                    burned[v] = true;
                    total_burned++;
                    changed = true;
                    
                    for (size_t neighbor : g.getNeighbors(v)) {
                        if (!burned[neighbor]) {
                            burned_neighbors_count[neighbor]++;
                            if (burned_neighbors_count[neighbor] == 2) {
                                next_spread_queue.push_back(neighbor);
                            }
                        }
                    }
                }
            }
            spread_queue.clear();
        }

        if (total_burned == n) break;

        // queima direta
        while (seq_idx < direct_burn.size()) {
            int target = direct_burn[seq_idx++];
            if (!burned[target]) {
                burned[target] = true;
                total_burned++;
                changed = true;

                for (size_t neighbor : g.getNeighbors(target)) {
                    if (!burned[neighbor]) {
                        burned_neighbors_count[neighbor]++;
                        if (burned_neighbors_count[neighbor] == 2) {
                            next_spread_queue.push_back(neighbor);
                        }
                    }
                }
                break;
            }
        }

        std::swap(spread_queue, next_spread_queue);

        if (!changed && spread_queue.empty()) {
            
            int best_v = -1;

            while (repair_idx < vertices_sorted_by_degree.size()) {
                int candidate = vertices_sorted_by_degree[repair_idx];
                
                if (!burned[candidate]) {
                    best_v = candidate;
                    break; 
                }
                
                repair_idx++;
            }


            if (best_v != -1) {
                burned[best_v] = true;
                total_burned++;
                changed = true; 
                
                for (size_t neighbor : g.getNeighbors(best_v)) {
                    if (!burned[neighbor]) {
                        burned_neighbors_count[neighbor]++;
                        if (burned_neighbors_count[neighbor] == 2) {
                            next_spread_queue.push_back(neighbor);
                        }
                    }
                }
            }
        }
    }

    return rounds;
}


std::vector<int> Decoder2Queima::get_burn_sequence(const std::vector< double >& chromosome) const {
    const int n = g.getOrder();
    std::vector<int> direct_burn;
    direct_burn.reserve(n);

    for (int i = 0; i < n; ++i) {
        if (chromosome[i] >= 0.5) {
            direct_burn.push_back(i);
        }
    }

    std::sort(direct_burn.begin(), direct_burn.end(),
        [&](int a, int b) { return chromosome[a] > chromosome[b]; });

    std::vector<bool> burned(n, false);
    std::vector<int> burned_neighbors_count(n, 0);
    std::vector<int> spread_queue;
    std::vector<int> next_spread_queue;
    
    std::vector<int> sequence; 
    
    spread_queue.reserve(n);
    next_spread_queue.reserve(n);

    int total_burned = 0;
    size_t seq_idx = 0;
    size_t repair_idx = 0;

    while (total_burned < n) {
        bool changed = false;

        if (!spread_queue.empty()) {
            for (int v : spread_queue) {
                if (!burned[v]) {
                    burned[v] = true;
                    total_burned++;
                    changed = true;
                    
                    for (size_t neighbor : g.getNeighbors(v)) {
                        if (!burned[neighbor]) {
                            burned_neighbors_count[neighbor]++;
                            if (burned_neighbors_count[neighbor] == 2) {
                                next_spread_queue.push_back(neighbor);
                            }
                        }
                    }
                }
            }
            spread_queue.clear();
        }

        if (total_burned == n) break;

        while (seq_idx < direct_burn.size()) {
            int target = direct_burn[seq_idx++];
            if (!burned[target]) {
                burned[target] = true;
                total_burned++;
                changed = true;
                
                // É AQUI QUE ANOTAMOS A SEQUÊNCIA!
                sequence.push_back(target); 

                for (size_t neighbor : g.getNeighbors(target)) {
                    if (!burned[neighbor]) {
                        burned_neighbors_count[neighbor]++;
                        if (burned_neighbors_count[neighbor] == 2) {
                            next_spread_queue.push_back(neighbor);
                        }
                    }
                }
                break;
            }
        }

        std::swap(spread_queue, next_spread_queue);

        if (!changed && spread_queue.empty()) {
            
            int best_v = -1;

            while (repair_idx < vertices_sorted_by_degree.size()) {
                int candidate = vertices_sorted_by_degree[repair_idx];
                
                if (!burned[candidate]) {
                    best_v = candidate;
                    break; 
                }
                
                repair_idx++;
            }


            if (best_v != -1) {
                burned[best_v] = true;
                total_burned++;
                changed = true; 

                sequence.push_back(best_v); 
                
                for (size_t neighbor : g.getNeighbors(best_v)) {
                    if (!burned[neighbor]) {
                        burned_neighbors_count[neighbor]++;
                        if (burned_neighbors_count[neighbor] == 2) {
                            next_spread_queue.push_back(neighbor);
                        }
                    }
                }
            }
        }
    }

    return sequence;
}