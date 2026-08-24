#include "Decoder2QueimaV2.h"
#include <algorithm>
#include <cstdint>
#include <numeric>

namespace {
    struct ThreadBuffers {
        std::vector<int> direct_burn;
        std::vector<uint8_t> burned;
        std::vector<int> burned_neighbors_count;
        std::vector<int> spread_queue;
        std::vector<int> next_spread_queue;
    };
    thread_local ThreadBuffers tls_buffers;
}

Decoder2Queima::Decoder2Queima(const Graph& graph) : g{graph} {
    int n = g.getOrder();
    vertices_sorted_by_degree.resize(n);
    
    std::iota(vertices_sorted_by_degree.begin(), vertices_sorted_by_degree.end(), 0);
    
    std::sort(vertices_sorted_by_degree.begin(), vertices_sorted_by_degree.end(),
        [&](int a, int b) { 
            return g.getVertexDegree(a) > g.getVertexDegree(b); 
        });
}

double Decoder2Queima::decode(const std::vector< double >& chromosome) const {
    const int n = g.getOrder();
    auto& bufs = tls_buffers;

    if (bufs.burned.size() != static_cast<size_t>(n)) {
        bufs.direct_burn.reserve(n);
        bufs.burned.resize(n);
        bufs.burned_neighbors_count.resize(n);
        bufs.spread_queue.reserve(n);
        bufs.next_spread_queue.reserve(n);
    }

    bufs.direct_burn.clear();
    for (int i = 0; i < n; ++i) {
        if (chromosome[i] >= 0.5) {
            bufs.direct_burn.push_back(i);
        }
    }

    size_t sort_limit = std::min(bufs.direct_burn.size(), static_cast<size_t>(256));
    std::partial_sort(bufs.direct_burn.begin(), bufs.direct_burn.begin() + sort_limit, bufs.direct_burn.end(),
        [&](int a, int b) { return chromosome[a] > chromosome[b]; });

    std::fill(bufs.burned.begin(), bufs.burned.end(), 0);
    std::fill(bufs.burned_neighbors_count.begin(), bufs.burned_neighbors_count.end(), 0);
    bufs.spread_queue.clear();
    bufs.next_spread_queue.clear();
    
    int total_burned = 0;
    int rounds = 0;
    size_t seq_idx = 0;
    size_t repair_idx = 0;

    while (total_burned < n) {
        bool changed = false;
        rounds++;

        if (!bufs.spread_queue.empty()) {
            for (int v : bufs.spread_queue) {
                if (!bufs.burned[v]) {
                    bufs.burned[v] = 1;
                    total_burned++;
                    changed = true;
                    
                    for (size_t neighbor : g.getNeighbors(v)) {
                        if (!bufs.burned[neighbor]) {
                            bufs.burned_neighbors_count[neighbor]++;
                            if (bufs.burned_neighbors_count[neighbor] == 2) {
                                bufs.next_spread_queue.push_back(neighbor);
                            }
                        }
                    }
                }
            }
            bufs.spread_queue.clear();
        }

        if (total_burned == n) break;

        while (seq_idx < bufs.direct_burn.size()) {
            if (seq_idx >= sort_limit && sort_limit < bufs.direct_burn.size()) {
                size_t next_limit = std::min(bufs.direct_burn.size(), sort_limit + 256);
                std::partial_sort(bufs.direct_burn.begin() + sort_limit, bufs.direct_burn.begin() + next_limit, bufs.direct_burn.end(),
                    [&](int a, int b) { return chromosome[a] > chromosome[b]; });
                sort_limit = next_limit;
            }

            int target = bufs.direct_burn[seq_idx++];
            if (!bufs.burned[target]) {
                bufs.burned[target] = 1;
                total_burned++;
                changed = true;

                for (size_t neighbor : g.getNeighbors(target)) {
                    if (!bufs.burned[neighbor]) {
                        bufs.burned_neighbors_count[neighbor]++;
                        if (bufs.burned_neighbors_count[neighbor] == 2) {
                            bufs.next_spread_queue.push_back(neighbor);
                        }
                    }
                }
                break;
            }
        }

        std::swap(bufs.spread_queue, bufs.next_spread_queue);

        if (!changed && bufs.spread_queue.empty()) {
            int best_v = -1;

            while (repair_idx < vertices_sorted_by_degree.size()) {
                int candidate = vertices_sorted_by_degree[repair_idx];
                
                if (!bufs.burned[candidate]) {
                    best_v = candidate;
                    break; 
                }
                
                repair_idx++;
            }

            if (best_v != -1) {
                bufs.burned[best_v] = 1;
                total_burned++;
                changed = true; 
                
                for (size_t neighbor : g.getNeighbors(best_v)) {
                    if (!bufs.burned[neighbor]) {
                        bufs.burned_neighbors_count[neighbor]++;
                        if (bufs.burned_neighbors_count[neighbor] == 2) {
                            bufs.next_spread_queue.push_back(neighbor);
                        }
                    }
                }
            }
        }
    }

    if (total_burned < n) {
        return rounds + (n - total_burned) * 100.0;
    }

    return rounds;
}

std::vector<int> Decoder2Queima::get_burn_sequence(const std::vector< double >& chromosome) const {
    const int n = g.getOrder();
    auto& bufs = tls_buffers;

    if (bufs.burned.size() != static_cast<size_t>(n)) {
        bufs.direct_burn.reserve(n);
        bufs.burned.resize(n);
        bufs.burned_neighbors_count.resize(n);
        bufs.spread_queue.reserve(n);
        bufs.next_spread_queue.reserve(n);
    }

    bufs.direct_burn.clear();
    for (int i = 0; i < n; ++i) {
        if (chromosome[i] >= 0.5) {
            bufs.direct_burn.push_back(i);
        }
    }

    size_t sort_limit = std::min(bufs.direct_burn.size(), static_cast<size_t>(256));
    std::partial_sort(bufs.direct_burn.begin(), bufs.direct_burn.begin() + sort_limit, bufs.direct_burn.end(),
        [&](int a, int b) { return chromosome[a] > chromosome[b]; });

    std::fill(bufs.burned.begin(), bufs.burned.end(), 0);
    std::fill(bufs.burned_neighbors_count.begin(), bufs.burned_neighbors_count.end(), 0);
    bufs.spread_queue.clear();
    bufs.next_spread_queue.clear();
    
    std::vector<int> sequence; 
    int total_burned = 0;
    size_t seq_idx = 0;
    size_t repair_idx = 0;

    while (total_burned < n) {
        bool changed = false;

        if (!bufs.spread_queue.empty()) {
            for (int v : bufs.spread_queue) {
                if (!bufs.burned[v]) {
                    bufs.burned[v] = 1;
                    total_burned++;
                    changed = true;
                    
                    for (size_t neighbor : g.getNeighbors(v)) {
                        if (!bufs.burned[neighbor]) {
                            bufs.burned_neighbors_count[neighbor]++;
                            if (bufs.burned_neighbors_count[neighbor] == 2) {
                                bufs.next_spread_queue.push_back(neighbor);
                            }
                        }
                    }
                }
            }
            bufs.spread_queue.clear();
        }

        if (total_burned == n) break;

        while (seq_idx < bufs.direct_burn.size()) {
            if (seq_idx >= sort_limit && sort_limit < bufs.direct_burn.size()) {
                size_t next_limit = std::min(bufs.direct_burn.size(), sort_limit + 256);
                std::partial_sort(bufs.direct_burn.begin() + sort_limit, bufs.direct_burn.begin() + next_limit, bufs.direct_burn.end(),
                    [&](int a, int b) { return chromosome[a] > chromosome[b]; });
                sort_limit = next_limit;
            }

            int target = bufs.direct_burn[seq_idx++];
            if (!bufs.burned[target]) {
                bufs.burned[target] = 1;
                total_burned++;
                changed = true;
                
                sequence.push_back(target); 

                for (size_t neighbor : g.getNeighbors(target)) {
                    if (!bufs.burned[neighbor]) {
                        bufs.burned_neighbors_count[neighbor]++;
                        if (bufs.burned_neighbors_count[neighbor] == 2) {
                            bufs.next_spread_queue.push_back(neighbor);
                        }
                    }
                }
                break;
            }
        }

        std::swap(bufs.spread_queue, bufs.next_spread_queue);

        if (!changed && bufs.spread_queue.empty()) {
            int best_v = -1;

            while (repair_idx < vertices_sorted_by_degree.size()) {
                int candidate = vertices_sorted_by_degree[repair_idx];
                
                if (!bufs.burned[candidate]) {
                    best_v = candidate;
                    break; 
                }
                
                repair_idx++;
            }

            if (best_v != -1) {
                bufs.burned[best_v] = 1;
                total_burned++;
                changed = true; 

                sequence.push_back(best_v); 
                
                for (size_t neighbor : g.getNeighbors(best_v)) {
                    if (!bufs.burned[neighbor]) {
                        bufs.burned_neighbors_count[neighbor]++;
                        if (bufs.burned_neighbors_count[neighbor] == 2) {
                            bufs.next_spread_queue.push_back(neighbor);
                        }
                    }
                }
            }
        }
    }

    return sequence;
}