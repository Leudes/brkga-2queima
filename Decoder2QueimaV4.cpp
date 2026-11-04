#include "Decoder2QueimaV4.h"
#include "SimulacaoQueima.h"

#include <algorithm>

namespace {

// A fila guarda uma copia da pontuacao no momento em que ela mudou. Copias
// antigas sao descartadas somente quando chegam ao topo da fila.
struct EntradaGatilho {
    int quantidade;
    double pontuacao;
    int vertice;
};

bool menor_prioridade(
    const EntradaGatilho& primeira,
    const EntradaGatilho& segunda) {

    if (primeira.pontuacao != segunda.pontuacao) {
        return primeira.pontuacao < segunda.pontuacao;
    }

    // O desempate pelo indice deixa o resultado deterministico caso duas
    // chaves do cromossomo sejam exatamente iguais.
    return primeira.vertice > segunda.vertice;
}

struct MemoriaAuxiliar {
    EstadoQueima estado;
    std::vector<int> fontes_do_cromossomo;
    std::vector<int> ordem_para_reparo;
    std::vector<int> vertices_com_um_queimado;
    std::vector<int> candidatos_empatados;
    std::vector<int> quantidade_gatilhos;
    std::vector<EntradaGatilho> melhores_gatilhos;
    bool gatilhos_iniciados = false;

    void reiniciar(
        const Graph& grafo,
        const std::vector<double>& cromossomo) {

        size_t numero_vertices = grafo.getOrder();
        estado.reiniciar(numero_vertices);

        preparar_ordem_do_cromossomo(
            ordem_para_reparo,
            cromossomo,
            numero_vertices,
            false);

        // A ordem ja esta decrescente. Portanto, as chaves maiores ou iguais
        // a 0,5 formam o inicio dessa lista.
        fontes_do_cromossomo.clear();
        fontes_do_cromossomo.reserve(numero_vertices);

        for (int vertice : ordem_para_reparo) {
            if (cromossomo[vertice] < 0.5) {
                break;
            }

            fontes_do_cromossomo.push_back(vertice);
        }

        quantidade_gatilhos.resize(numero_vertices);
        vertices_com_um_queimado.clear();
        vertices_com_um_queimado.reserve(numero_vertices);
        candidatos_empatados.clear();
        candidatos_empatados.reserve(numero_vertices);
        melhores_gatilhos.clear();

        if (melhores_gatilhos.capacity() < numero_vertices) {
            melhores_gatilhos.reserve(numero_vertices);
        }

        gatilhos_iniciados = false;
    }
};

// Cada thread possui sua propria memoria para que as avaliacoes paralelas do
// BRKGA nao misturem seus dados.
thread_local MemoriaAuxiliar memoria;

void guardar_nova_pontuacao(
    MemoriaAuxiliar& dados,
    const std::vector<double>& cromossomo,
    int vertice) {

    int quantidade = dados.quantidade_gatilhos[vertice];
    if (quantidade <= 0 || dados.estado.queimado[vertice]) {
        return;
    }

    dados.melhores_gatilhos.push_back({
        quantidade,
        quantidade + cromossomo[vertice],
        vertice});

    std::push_heap(
        dados.melhores_gatilhos.begin(),
        dados.melhores_gatilhos.end(),
        menor_prioridade);
}

// Um vertice com exatamente um vizinho queimado funciona como gatilho para
// cada um de seus vizinhos ainda nao queimados.
void alterar_gatilhos_criados_por(
    MemoriaAuxiliar& dados,
    const Graph& grafo,
    const std::vector<double>& cromossomo,
    int vertice,
    int alteracao) {

    for (size_t vizinho : grafo.getNeighbors(vertice)) {
        if (dados.estado.queimado[vizinho]) {
            continue;
        }

        dados.quantidade_gatilhos[vizinho] += alteracao;

        if (dados.gatilhos_iniciados) {
            guardar_nova_pontuacao(
                dados,
                cromossomo,
                static_cast<int>(vizinho));
        }
    }
}

void queimar_e_atualizar_gatilhos(
    MemoriaAuxiliar& dados,
    const Graph& grafo,
    const std::vector<double>& cromossomo,
    int vertice) {

    // Antes do primeiro reparo nao ha motivo para manter os gatilhos. Nesse
    // trecho usamos a simulacao comum, que e mais barata.
    if (!dados.gatilhos_iniciados) {
        queimar_vertice(
            grafo,
            vertice,
            dados.estado,
            &dados.vertices_com_um_queimado);
        return;
    }

    if (dados.estado.queimado[vertice]) {
        return;
    }

    dados.estado.queimado[vertice] = 1;

    // Se o proprio vertice era um gatilho, ele deixa de contribuir ao queimar.
    if (dados.estado.quantidade_vizinhos_queimados[vertice] == 1) {
        alterar_gatilhos_criados_por(
            dados,
            grafo,
            cromossomo,
            vertice,
            -1);
    }

    dados.estado.total_queimados++;
    dados.quantidade_gatilhos[vertice] = 0;

    for (size_t vizinho : grafo.getNeighbors(vertice)) {
        if (dados.estado.queimado[vizinho]) {
            continue;
        }

        int quantidade_anterior =
            dados.estado.quantidade_vizinhos_queimados[vizinho];
        dados.estado.quantidade_vizinhos_queimados[vizinho]++;

        if (quantidade_anterior == 0) {
            // O vizinho passou a ter exatamente um queimado ao lado.
            dados.vertices_com_um_queimado.push_back(
                static_cast<int>(vizinho));
            alterar_gatilhos_criados_por(
                dados,
                grafo,
                cromossomo,
                static_cast<int>(vizinho),
                1);
        } else if (quantidade_anterior == 1) {
            // Com dois vizinhos queimados, ele deixa de ser gatilho e entra
            // na fila de propagacao da proxima rodada.
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

void propagar_e_atualizar_gatilhos(
    MemoriaAuxiliar& dados,
    const Graph& grafo,
    const std::vector<double>& cromossomo) {

    for (int vertice : dados.estado.fila_propagacao_atual) {
        queimar_e_atualizar_gatilhos(
            dados,
            grafo,
            cromossomo,
            vertice);
    }

    dados.estado.fila_propagacao_atual.clear();
}

void iniciar_gatilhos(
    MemoriaAuxiliar& dados,
    const Graph& grafo,
    const std::vector<double>& cromossomo) {

    std::fill(
        dados.quantidade_gatilhos.begin(),
        dados.quantidade_gatilhos.end(),
        0);
    dados.melhores_gatilhos.clear();

    // Constroi os valores uma unica vez com base no estado atual da queima.
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

    // Durante a construcao acima nao inserimos cada mudanca separadamente na
    // fila. Agora colocamos somente a pontuacao final de cada candidato.
    for (size_t vertice = 0; vertice < grafo.getOrder(); ++vertice) {
        if (!dados.estado.queimado[vertice] &&
            dados.quantidade_gatilhos[vertice] > 0) {
            int quantidade = dados.quantidade_gatilhos[vertice];
            dados.melhores_gatilhos.push_back({
                quantidade,
                quantidade + cromossomo[vertice],
                static_cast<int>(vertice)});
        }
    }

    std::make_heap(
        dados.melhores_gatilhos.begin(),
        dados.melhores_gatilhos.end(),
        menor_prioridade);

    dados.gatilhos_iniciados = true;
}

int escolher_reparo_por_gatilho(
    const Graph& grafo,
    MemoriaAuxiliar& dados,
    const std::vector<double>& cromossomo) {
    if (!dados.gatilhos_iniciados) {
        iniciar_gatilhos(dados, grafo, cromossomo);
    }

    // Descarta entradas antigas ate encontrar uma pontuacao que ainda
    // corresponde ao estado atual.
    while (!dados.melhores_gatilhos.empty()) {
        const EntradaGatilho& melhor = dados.melhores_gatilhos.front();

        bool entrada_valida =
            !dados.estado.queimado[melhor.vertice] &&
            melhor.quantidade ==
                dados.quantidade_gatilhos[melhor.vertice];

        if (entrada_valida) {
            break;
        }

        // A quantidade mudou depois que esta entrada foi inserida.
        std::pop_heap(
            dados.melhores_gatilhos.begin(),
            dados.melhores_gatilhos.end(),
            menor_prioridade);
        dados.melhores_gatilhos.pop_back();
    }

    if (dados.melhores_gatilhos.empty()) {
        return -1;
    }

    double melhor_pontuacao =
        dados.melhores_gatilhos.front().pontuacao;
    std::vector<int>& empatados = dados.candidatos_empatados;
    empatados.clear();

    // Retira temporariamente todas as entradas validas com a melhor
    // pontuacao. Normalmente existe apenas uma. Esse trecho tambem elimina
    // copias antigas que tenham chegado ao topo da fila.
    while (!dados.melhores_gatilhos.empty()) {
        const EntradaGatilho& entrada =
            dados.melhores_gatilhos.front();

        bool entrada_valida =
            !dados.estado.queimado[entrada.vertice] &&
            entrada.quantidade ==
                dados.quantidade_gatilhos[entrada.vertice];

        if (!entrada_valida) {
            std::pop_heap(
                dados.melhores_gatilhos.begin(),
                dados.melhores_gatilhos.end(),
                menor_prioridade);
            dados.melhores_gatilhos.pop_back();
            continue;
        }

        if (entrada.pontuacao != melhor_pontuacao) {
            break;
        }

        int vertice = entrada.vertice;
        std::pop_heap(
            dados.melhores_gatilhos.begin(),
            dados.melhores_gatilhos.end(),
            menor_prioridade);
        dados.melhores_gatilhos.pop_back();

        if (std::find(empatados.begin(), empatados.end(), vertice) ==
            empatados.end()) {
            empatados.push_back(vertice);
        }
    }

    // Recoloca uma copia atual de cada candidato retirado.
    for (int vertice : empatados) {
        guardar_nova_pontuacao(dados, cromossomo, vertice);
    }

    if (empatados.size() == 1) {
        return empatados.front();
    }

    // Em um empate exato, reproduz a ordem usada pela V4 anterior: primeiro
    // o gatilho que entrou antes na fronteira e depois a ordem dos vizinhos.
    for (int gatilho : dados.vertices_com_um_queimado) {
        if (dados.estado.queimado[gatilho] ||
            dados.estado.quantidade_vizinhos_queimados[gatilho] != 1) {
            continue;
        }

        for (size_t candidato : grafo.getNeighbors(gatilho)) {
            if (dados.estado.queimado[candidato]) {
                continue;
            }

            double pontuacao =
                dados.quantidade_gatilhos[candidato] +
                cromossomo[candidato];

            if (pontuacao == melhor_pontuacao) {
                return static_cast<int>(candidato);
            }
        }
    }

    return empatados.front();
}

double simular(
    const Graph& grafo,
    const std::vector<double>& cromossomo,
    std::vector<int>* sequencia_queima) {

    validar_cromossomo(grafo, cromossomo);

    // Guardar a referencia evita procurar a variavel thread_local em todas as
    // operacoes internas da simulacao.
    MemoriaAuxiliar& dados = memoria;
    dados.reiniciar(grafo, cromossomo);

    if (sequencia_queima != nullptr) {
        sequencia_queima->clear();
        sequencia_queima->reserve(grafo.getOrder());
    }

    size_t posicao_cromossomo = 0;
    size_t posicao_reparo = 0;
    int rodadas = 0;

    while (!dados.estado.todos_queimados()) {
        rodadas++;

        propagar_e_atualizar_gatilhos(
            dados,
            grafo,
            cromossomo);

        if (dados.estado.todos_queimados()) {
            break;
        }

        int escolhido = proximo_vertice_nao_queimado(
            dados.fontes_do_cromossomo,
            posicao_cromossomo,
            dados.estado.queimado);

        if (escolhido == -1) {
            escolhido = escolher_reparo_por_gatilho(
                grafo,
                dados,
                cromossomo);
        }

        // Se nao houver gatilho aplicavel, usa a maior chave nao queimada
        // para que o reparo sempre produza uma solucao completa.
        if (escolhido == -1) {
            escolhido = proximo_vertice_nao_queimado(
                dados.ordem_para_reparo,
                posicao_reparo,
                dados.estado.queimado);
        }

        if (escolhido == -1) {
            break;
        }

        queimar_e_atualizar_gatilhos(
            dados,
            grafo,
            cromossomo,
            escolhido);

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
