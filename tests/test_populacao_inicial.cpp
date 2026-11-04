#include "Decoder2Queima.h"
#include "Graph.h"
#include "brkgaAPI/BRKGA.h"
#include "brkgaAPI/MTRand.h"
#include "HeuristicaInicial.h"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <vector>

bool populacoes_iguais(
    const BRKGA<Decodificador2Queima, MTRand>& primeira,
    const BRKGA<Decodificador2Queima, MTRand>& segunda) {

  for (unsigned indice_populacao = 0;
       indice_populacao < primeira.getK();
       indice_populacao++) {
    const Population& populacao_a =
        primeira.getPopulation(indice_populacao);
    const Population& populacao_b =
        segunda.getPopulation(indice_populacao);

    for (unsigned individuo = 0;
         individuo < populacao_a.getP();
         individuo++) {
      if (populacao_a.getFitness(individuo) !=
              populacao_b.getFitness(individuo) ||
          populacao_a.getChromosome(individuo) !=
              populacao_b.getChromosome(individuo)) {
        return false;
      }
    }
  }

  return true;
}

unsigned contar_cromossomo(
    const BRKGA<Decodificador2Queima, MTRand>& algoritmo,
    const std::vector<double>& procurado) {

  unsigned quantidade = 0;

  for (unsigned indice_populacao = 0;
       indice_populacao < algoritmo.getK();
       indice_populacao++) {
    const Population& populacao =
        algoritmo.getPopulation(indice_populacao);

    for (unsigned individuo = 0;
         individuo < populacao.getP();
         individuo++) {
      if (populacao.getChromosome(individuo) == procurado) {
        quantidade++;
      }
    }
  }

  return quantidade;
}

int main() {
  Graph grafo;
  const unsigned numero_vertices = 12;

  for (unsigned vertice = 0; vertice < numero_vertices; vertice++) {
    grafo.addEdge(vertice, (vertice + 1) % numero_vertices);
  }

  Decodificador2Queima decodificador(grafo);

  // A API antiga continua produzindo a mesma populacao quando a nova funcao
  // nao e chamada.
  MTRand gerador_a(12345);
  MTRand gerador_b(12345);
  BRKGA<Decodificador2Queima, MTRand> algoritmo_a(
      numero_vertices, 20, 0.20, 0.10, 0.70,
      decodificador, gerador_a, 2, 1);
  BRKGA<Decodificador2Queima, MTRand> algoritmo_b(
      numero_vertices, 20, 0.20, 0.10, 0.70,
      decodificador, gerador_b, 2, 1);
  assert(populacoes_iguais(algoritmo_a, algoritmo_b));

  SolucaoInicial solucao =
      construir_por_grau_relativo_e_gatilho(grafo);
  assert(avaliar_solucao(grafo, solucao.fontes_de_queima) ==
         solucao.numero_de_rodadas);

  std::vector<double> cromossomo = criar_cromossomo_da_solucao(
      numero_vertices, solucao.fontes_de_queima);
  double fitness_inicial = decodificador.decode(cromossomo);
  assert(fitness_inicial == solucao.numero_de_rodadas);

  algoritmo_a.setInitialPopulation(cromossomo);
  assert(contar_cromossomo(algoritmo_a, cromossomo) == 1);
  assert(algoritmo_a.getBestFitness() <= fitness_inicial);

  algoritmo_a.evolve();
  assert(algoritmo_a.getBestFitness() <= fitness_inicial);

  bool tamanho_invalido_foi_rejeitado = false;
  try {
    algoritmo_a.setInitialPopulation(std::vector<double>(3, 0.5));
  } catch (const std::range_error&) {
    tamanho_invalido_foi_rejeitado = true;
  }
  assert(tamanho_invalido_foi_rejeitado);

  std::cout << "Teste da populacao inicial: OK\n";
  return 0;
}
