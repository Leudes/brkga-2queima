#include "Graph.h"

Graph::Graph(const std::string& filename) {
    std::ifstream file(filename);
    
    if (!file) {
        throw std::runtime_error("Error opening the file!");
    }
    
    size_t source {0}, destination {0};
    std::string line {};
    
    std::vector<std::pair<size_t, size_t>> edges;
    size_t maxVertex = 0;
    bool hasEdges = false;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '%') continue;
        std::stringstream ssEdges(line);
        if (ssEdges >> source >> destination) {
            if (source != destination) {
                edges.push_back({source, destination});
                if (!hasEdges || source > maxVertex) maxVertex = source;
                if (!hasEdges || destination > maxVertex) maxVertex = destination;
                hasEdges = true;
            }
        }
    }
    file.close();

    if (hasEdges) {
        adjList.resize(maxVertex + 1);
        for (const auto& [u, v] : edges) {
            adjList[u].push_back(v);
            adjList[v].push_back(u);
        }
        for (size_t i = 0; i <= maxVertex; ++i) {
            std::sort(adjList[i].begin(), adjList[i].end());
            adjList[i].erase(std::unique(adjList[i].begin(), adjList[i].end()), adjList[i].end());
        }
    }
}

void Graph::addVertex(size_t source) {
    if (source >= adjList.size()) {
        adjList.resize(source + 1);
    }
}

void Graph::addEdge(size_t source, size_t destination) {
    if (source == destination) { return; }

    if (source >= adjList.size()) adjList.resize(source + 1);
    if (destination >= adjList.size()) adjList.resize(destination + 1);

    if (!edgeExists(source, destination)) {
        adjList[source].push_back(destination);
        adjList[destination].push_back(source);
        std::sort(adjList[source].begin(), adjList[source].end());
        std::sort(adjList[destination].begin(), adjList[destination].end());
    }
}

size_t Graph::getOrder() const noexcept { return adjList.size(); }

size_t Graph::getSize() const noexcept { 
    size_t count = 0;
    for (const auto& neighbors : adjList) {
        count += neighbors.size();
    }
    // Each edge is counted two times 
    return count / 2;
}

size_t Graph::getVertexDegree(size_t vertex) const {
    if (!vertexExists(vertex)) {
        std::string str = "Vertex " + std::to_string(vertex) + " does not exists in the graph";
        throw std::out_of_range(str);
    }
    return adjList[vertex].size();
}

size_t Graph::getMinDegree() const { 
    if (adjList.empty()) {
        throw std::runtime_error("error: empty graph does not have minimum degree");
    }
	
    size_t minDegree = std::numeric_limits<size_t>::max();
    for (const auto& neighbors : adjList) {
        if (neighbors.size() < minDegree) {
            minDegree = neighbors.size();
        }
    }
            
    return minDegree;
}

size_t Graph::getMaxDegree() const { 
    if (adjList.empty()) {
        throw std::runtime_error("error: empty graph does not have minimum degree");
    }
	
    size_t maxDegree = 0;
    for (const auto& neighbors : adjList) {
        if (neighbors.size() > maxDegree) {
            maxDegree = neighbors.size();
        }
    }
            
    return maxDegree;
}

const std::vector<size_t>& Graph::getNeighbors(size_t vertex) const { return this->adjList.at(vertex); }

bool Graph::vertexExists(size_t vertex) const { return vertex < adjList.size(); }

bool Graph::edgeExists(size_t u, size_t v) const {
    if (!vertexExists(u) || !vertexExists(v)) {
        return false;
    }
    return std::binary_search(adjList[u].begin(), adjList[u].end(), v);
}

void Graph::deleteVertex(size_t vertex) {
    if (!vertexExists(vertex)) {
        throw std::runtime_error("Vertex does not exist (function deleteVertex)");
    }
    for (size_t neighbor : adjList[vertex]) {
        if (neighbor < adjList.size()) {
            auto& nvec = adjList[neighbor];
            nvec.erase(std::remove(nvec.begin(), nvec.end(), vertex), nvec.end());
        }
    }
    adjList[vertex].clear();
}

float Graph::getDensity() const noexcept {
    if (getOrder() <= 1) return 0.0f;
    return static_cast<float>(getSize() * 2) / (getOrder() * (getOrder() - 1)); 
}

std::unordered_set<size_t> Graph::getIsolatedVertices() const {
    std::unordered_set<size_t> vertices;
    for (size_t i = 0; i < adjList.size(); ++i) {
        if (adjList[i].empty()) {
            vertices.insert(i);
        }
    }
    return vertices;
}

std::unordered_set<size_t> Graph::getVertices() const {
    std::unordered_set<size_t> vertices;
    for (size_t i = 0; i < adjList.size(); ++i) {
        vertices.insert(i);
    }
    return vertices;
}

std::ostream& operator<< (std::ostream& os, const Graph& graph) {
    for (size_t vertex = 0; vertex < graph.adjList.size(); ++vertex) {
        os << vertex << " ----> ";
        for (const auto& neighbor : graph.adjList[vertex]) { 
            os << neighbor << " ";  
        }
        os << '\n';
    }
    return os;
}

size_t Graph::chooseRandomVertex() const {
    if (adjList.empty()) {
        throw std::runtime_error("Graph is empty, cannot choose a vertex.");
    }
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> distrib(0, adjList.size() - 1);
    return distrib(gen);
}

