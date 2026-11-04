#include "Decoder2QueimaV5.h"
#include "SimulacaoQueima.h"

#include <algorithm>

namespace {

struct EntradaReparo {
    int pontuacao;
    int gatilhos;
    int grau;
    double chave;
    int vertice;
    unsigned versao;
};

bool menor_prioridade(
    const EntradaReparo& primeira,
    const EntradaReparo& segunda) {

    if (primeira.pontuacao != segunda.pontuacao) {
        return primeira.pontuacao < segunda.pontuacao;
    }
    if (primeira.gatilhos != segunda.gatilhos) {
        return primeira.gatilhos < segunda.gatilhos;
    }
    if (primeira.grau != segunda.grau) {
        return primeira.grau < segunda.grau;
    }
    if (primeira.chave != segunda.chave) {
        return primeira.chave < segunda.chave;
    }

    return primeira.vertice > segunda.vertice;
}

struct MemoriaAuxiliar {
    EstadoQueima estado;
    std::vector<int> fila_cromossomo;
    std::vector<int> grau_relativo;
    std::vector<int> quantidade_gatilhos;
    std::vector<unsigned> versao_pontuacao;
    std::vector<EntradaReparo> melhores_candidatos;
    bool heuristicas_iniciadas = false;

    void reiniciar(
        const Graph& grafo,
        const std::vector<double>& cromossomo) {

        size_t numero_vertices = grafo.getOrder();
        estado.reiniciar(numero_vertices);

        preparar_fila_do_cromossomo(
            fila_cromossomo,
            cromossomo,
            numero_vertices,
            true);

        grau_relativo.resize(numero_vertices);
        quantidade_gatilhos.resize(numero_vertices);
        versao_pontuacao.resize(numero_vertices);
        melhores_candidatos.clear();
        if (melhores_candidatos.capacity() < numero_vertices) {
            melhores_candidatos.reserve(numero_vertices);
        }
        heuristicas_iniciadas = false;
    }
};

// Cada thread possui sua propria memoria para que as avaliacoes paralelas do
// BRKGA nao misturem seus dados.
thread_local MemoriaAuxiliar memoria;

void guardar_nova_pontuacao(
    MemoriaAuxiliar& dados,
    const std::vector<double>& cromossomo,
    int vertice) {

    if (dados.estado.queimado[vertice]) {
        return;
    }

    int grau = dados.grau_relativo[vertice];
    int gatilhos = dados.quantidade_gatilhos[vertice];
    dados.melhores_candidatos.push_back({
        std::max(grau, gatilhos),
        gatilhos,
        grau,
        cromossomo[vertice],
        vertice,
        dados.versao_pontuacao[vertice]});

    std::push_heap(
        dados.melhores_candidatos.begin(),
        dados.melhores_candidatos.end(),
        menor_prioridade);
}

void pontuacao_foi_alterada(
    MemoriaAuxiliar& dados,
    const std::vector<double>& cromossomo,
    int vertice) {

    dados.versao_pontuacao[vertice]++;
    if (dados.heuristicas_iniciadas) {
        guardar_nova_pontuacao(dados, cromossomo, vertice);
    }
}

// Um vertice com exatamente um vizinho queimado e um gatilho para cada um de
// seus vizinhos ainda nao queimados.
void alterar_gatilhos_criados_por(
    MemoriaAuxiliar& dados,
    const Graph& grafo,
    const std::vector<double>& cromossomo,
    int vertice,
    int alteracao) {

    for (size_t vizinho : grafo.getNeighbors(vertice)) {
        if (!dados.estado.queimado[vizinho]) {
            dados.quantidade_gatilhos[vizinho] += alteracao;
            pontuacao_foi_alterada(
                dados,
                cromossomo,
                static_cast<int>(vizinho));
        }
    }
}

void queimar_e_atualizar_heuristicas(
    MemoriaAuxiliar& dados,
    const Graph& grafo,
    const std::vector<double>& cromossomo,
    int vertice) {

    if (!dados.heuristicas_iniciadas) {
        queimar_vertice(grafo, vertice, dados.estado);
        return;
    }

    if (dados.estado.queimado[vertice]) {
        return;
    }

    dados.estado.queimado[vertice] = 1;

    // Se o proprio vertice era um gatilho, deixa de contar ao queimar.
    if (dados.estado.quantidade_vizinhos_queimados[vertice] == 1) {
        alterar_gatilhos_criados_por(
            dados, grafo, cromossomo, vertice, -1);
    }

    dados.estado.total_queimados++;
    dados.grau_relativo[vertice] = 0;
    dados.quantidade_gatilhos[vertice] = 0;
    dados.versao_pontuacao[vertice]++;

    for (size_t vizinho : grafo.getNeighbors(vertice)) {
        if (dados.estado.queimado[vizinho]) {
            continue;
        }

        dados.grau_relativo[vizinho]--;
        pontuacao_foi_alterada(
            dados,
            cromossomo,
            static_cast<int>(vizinho));

        int quantidade_anterior =
            dados.estado.quantidade_vizinhos_queimados[vizinho];
        dados.estado.quantidade_vizinhos_queimados[vizinho]++;

        if (quantidade_anterior == 0) {
            // O vizinho passou a ter exatamente um queimado ao lado.
            alterar_gatilhos_criados_por(
                dados,
                grafo,
                cromossomo,
                static_cast<int>(vizinho),
                1);
        } else if (quantidade_anterior == 1) {
            // Ao chegar a dois, ele deixa de ser gatilho e passa a queimar
            // por propagacao na proxima rodada.
            alterar_gatilhos_criados_por(
                dados,
                grafo,
                cromossomo,
                static_cast<int>(vizinho),
                -1);
            dados.estado.fila_propagacao_seguinte.push_back(
                static_cast<int>(vizinho));
        }
    }
}

void propagar_e_atualizar_heuristicas(
    MemoriaAuxiliar& dados,
    const Graph& grafo,
    const std::vector<double>& cromossomo) {
    for (int vertice : dados.estado.fila_propagacao_atual) {
        queimar_e_atualizar_heuristicas(
            dados, grafo, cromossomo, vertice);
    }

    dados.estado.fila_propagacao_atual.clear();
}

void iniciar_heuristicas(
    const Graph& grafo,
    MemoriaAuxiliar& dados,
    const std::vector<double>& cromossomo) {

    std::fill(
        dados.quantidade_gatilhos.begin(),
        dados.quantidade_gatilhos.end(),
        0);
    std::fill(
        dados.versao_pontuacao.begin(),
        dados.versao_pontuacao.end(),
        0);
    dados.melhores_candidatos.clear();

    for (size_t vertice = 0; vertice < grafo.getOrder(); ++vertice) {
        if (dados.estado.queimado[vertice]) {
            dados.grau_relativo[vertice] = 0;
        } else {
            dados.grau_relativo[vertice] = static_cast<int>(
                grafo.getVertexDegree(vertice)) -
                dados.estado.quantidade_vizinhos_queimados[vertice];
        }
    }

    for (size_t vertice = 0; vertice < grafo.getOrder(); ++vertice) {
        if (!dados.estado.queimado[vertice] &&
            dados.estado.quantidade_vizinhos_queimados[vertice] == 1) {
            alterar_gatilhos_criados_por(
                dados,
                grafo,
                cromossomo,
                static_cast<int>(vertice),
                1);
        }
    }

    for (size_t vertice = 0; vertice < grafo.getOrder(); ++vertice) {
        if (dados.estado.queimado[vertice]) {
            continue;
        }

        int grau = dados.grau_relativo[vertice];
        int gatilhos = dados.quantidade_gatilhos[vertice];
        dados.melhores_candidatos.push_back({
            std::max(grau, gatilhos),
            gatilhos,
            grau,
            cromossomo[vertice],
            static_cast<int>(vertice),
            dados.versao_pontuacao[vertice]});
    }

    std::make_heap(
        dados.melhores_candidatos.begin(),
        dados.melhores_candidatos.end(),
        menor_prioridade);
    dados.heuristicas_iniciadas = true;
}

int escolher_reparo_combinado(
    const Graph& grafo,
    MemoriaAuxiliar& dados,
    const std::vector<double>& cromossomo) {

    if (!dados.heuristicas_iniciadas) {
        iniciar_heuristicas(grafo, dados, cromossomo);
    }

    while (!dados.melhores_candidatos.empty()) {
        const EntradaReparo& melhor =
            dados.melhores_candidatos.front();

        bool entrada_valida =
            !dados.estado.queimado[melhor.vertice] &&
            melhor.versao ==
                dados.versao_pontuacao[melhor.vertice];

        if (entrada_valida) {
            return melhor.vertice;
        }

        std::pop_heap(
            dados.melhores_candidatos.begin(),
            dados.melhores_candidatos.end(),
            menor_prioridade);
        dados.melhores_candidatos.pop_back();
    }

    return -1;
}

double simular(
    const Graph& grafo,
    const std::vector<double>& cromossomo,
    std::vector<int>* sequencia_queima) {

    validar_cromossomo(grafo, cromossomo);
    MemoriaAuxiliar& dados = memoria;
    dados.reiniciar(grafo, cromossomo);

    if (sequencia_queima != nullptr) {
        sequencia_queima->clear();
        sequencia_queima->reserve(grafo.getOrder());
    }

    int rodadas = 0;

    while (!dados.estado.todos_queimados()) {
        rodadas++;

        propagar_e_atualizar_heuristicas(
            dados, grafo, cromossomo);

        if (dados.estado.todos_queimados()) {
            break;
        }

        int escolhido = proximo_vertice_da_fila(
            dados.fila_cromossomo,
            cromossomo,
            dados.estado.queimado);

        if (escolhido == -1) {
            escolhido = escolher_reparo_combinado(
                grafo, dados, cromossomo);
        }

        if (escolhido == -1) {
            break;
        }

        queimar_e_atualizar_heuristicas(
            dados, grafo, cromossomo, escolhido);

        if (sequencia_queima != nullptr) {
            sequencia_queima->push_back(escolhido);
        }

        terminar_rodada(dados.estado);
    }

    return rodadas;
}

}  // namespace

double Decodificador2Queima::decode(
    const std::vector<double>& cromossomo) const {
    return simular(grafo, cromossomo, nullptr);
}

std::vector<int> Decodificador2Queima::obter_sequencia_queima(
    const std::vector<double>& cromossomo) const {
    std::vector<int> sequencia;
    simular(grafo, cromossomo, &sequencia);
    return sequencia;
}
