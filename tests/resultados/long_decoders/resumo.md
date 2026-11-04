# Comparação longa dos decoders

Compara V1, V2, V3 e V4 em 10 grafos, com 30 repetições por combinação, usando parâmetros mais longos que o teste rápido.

| Grafo | Vértices | Arestas | Versão | Execuções | Válidas | Fitness médio | Melhor fitness | Desvio | Tempo médio (s) |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| ciclo_n100.txt | 100 | 100 | V1_cromossomo | 30 | 30 | 54.100 | 53 | 0.759 | 0.160793 |
| ciclo_n100.txt | 100 | 100 | V2_grau_estatico | 30 | 30 | 54.033 | 52 | 0.890 | 0.144570 |
| ciclo_n100.txt | 100 | 100 | V3_grau_relativo | 30 | 30 | 51.733 | 51 | 0.450 | 0.304180 |
| ciclo_n100.txt | 100 | 100 | V4_gatilho | 30 | 30 | 51.000 | 51 | 0.000 | 0.165013 |
| cubic_100.txt | 100 | 150 | V1_cromossomo | 30 | 30 | 29.367 | 29 | 0.490 | 0.148613 |
| cubic_100.txt | 100 | 150 | V2_grau_estatico | 30 | 30 | 29.200 | 29 | 0.407 | 0.161275 |
| cubic_100.txt | 100 | 150 | V3_grau_relativo | 30 | 30 | 28.967 | 28 | 0.183 | 0.255926 |
| cubic_100.txt | 100 | 150 | V4_gatilho | 30 | 30 | 28.967 | 28 | 0.183 | 0.358103 |
| cycle1-order25.txt | 25 | 25 | V1_cromossomo | 30 | 30 | 14.000 | 14 | 0.000 | 0.025646 |
| cycle1-order25.txt | 25 | 25 | V2_grau_estatico | 30 | 30 | 14.000 | 14 | 0.000 | 0.028822 |
| cycle1-order25.txt | 25 | 25 | V3_grau_relativo | 30 | 30 | 14.000 | 14 | 0.000 | 0.041589 |
| cycle1-order25.txt | 25 | 25 | V4_gatilho | 30 | 30 | 14.000 | 14 | 0.000 | 0.045143 |
| cycle10-order250.txt | 250 | 250 | V1_cromossomo | 30 | 30 | 136.733 | 134 | 1.639 | 0.683642 |
| cycle10-order250.txt | 250 | 250 | V2_grau_estatico | 30 | 30 | 136.100 | 132 | 1.494 | 0.566552 |
| cycle10-order250.txt | 250 | 250 | V3_grau_relativo | 30 | 30 | 128.733 | 127 | 0.944 | 1.835146 |
| cycle10-order250.txt | 250 | 250 | V4_gatilho | 30 | 30 | 126.000 | 126 | 0.000 | 0.433571 |
| dwt__162.txt | 162 | 510 | V1_cromossomo | 30 | 30 | 11.433 | 11 | 0.504 | 0.249373 |
| dwt__162.txt | 162 | 510 | V2_grau_estatico | 30 | 30 | 11.500 | 11 | 0.509 | 0.243070 |
| dwt__162.txt | 162 | 510 | V3_grau_relativo | 30 | 30 | 12.200 | 12 | 0.407 | 0.336890 |
| dwt__162.txt | 162 | 510 | V4_gatilho | 30 | 30 | 11.133 | 11 | 0.346 | 0.333005 |
| grade_10x10.txt | 100 | 180 | V1_cromossomo | 30 | 30 | 15.800 | 15 | 0.484 | 0.140161 |
| grade_10x10.txt | 100 | 180 | V2_grau_estatico | 30 | 30 | 15.767 | 15 | 0.504 | 0.142363 |
| grade_10x10.txt | 100 | 180 | V3_grau_relativo | 30 | 30 | 17.700 | 17 | 0.466 | 0.210074 |
| grade_10x10.txt | 100 | 180 | V4_gatilho | 30 | 30 | 15.367 | 15 | 0.490 | 0.207306 |
| nos7.txt | 729 | 1944 | V1_cromossomo | 30 | 30 | 24.533 | 23 | 0.937 | 1.460258 |
| nos7.txt | 729 | 1944 | V2_grau_estatico | 30 | 30 | 24.067 | 23 | 0.785 | 1.538893 |
| nos7.txt | 729 | 1944 | V3_grau_relativo | 30 | 30 | 29.100 | 27 | 0.759 | 2.197776 |
| nos7.txt | 729 | 1944 | V4_gatilho | 30 | 30 | 23.900 | 23 | 0.662 | 1.860440 |
| random_denso_n50_p30.txt | 50 | 384 | V1_cromossomo | 30 | 30 | 4.000 | 4 | 0.000 | 0.088503 |
| random_denso_n50_p30.txt | 50 | 384 | V2_grau_estatico | 30 | 30 | 4.000 | 4 | 0.000 | 0.095999 |
| random_denso_n50_p30.txt | 50 | 384 | V3_grau_relativo | 30 | 30 | 4.000 | 4 | 0.000 | 0.069611 |
| random_denso_n50_p30.txt | 50 | 384 | V4_gatilho | 30 | 30 | 4.000 | 4 | 0.000 | 0.143506 |
| random_esparso_n200_d3.txt | 200 | 481 | V1_cromossomo | 30 | 30 | 13.600 | 13 | 0.498 | 0.343428 |
| random_esparso_n200_d3.txt | 200 | 481 | V2_grau_estatico | 30 | 30 | 13.367 | 13 | 0.490 | 0.353397 |
| random_esparso_n200_d3.txt | 200 | 481 | V3_grau_relativo | 30 | 30 | 18.000 | 18 | 0.000 | 0.297574 |
| random_esparso_n200_d3.txt | 200 | 481 | V4_gatilho | 30 | 30 | 13.200 | 13 | 0.407 | 0.422051 |
| steam3.txt | 80 | 424 | V1_cromossomo | 30 | 30 | 7.200 | 7 | 0.407 | 0.138969 |
| steam3.txt | 80 | 424 | V2_grau_estatico | 30 | 30 | 7.267 | 7 | 0.450 | 0.122140 |
| steam3.txt | 80 | 424 | V3_grau_relativo | 30 | 30 | 8.000 | 8 | 0.000 | 0.118968 |
| steam3.txt | 80 | 424 | V4_gatilho | 30 | 30 | 7.067 | 7 | 0.254 | 0.149380 |
