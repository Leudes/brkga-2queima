# Comparação rápida dos decoders

Compara V1, V2, V3 e V4 em 10 grafos, com 10 repetições por combinação e parâmetros reduzidos para uma avaliação rápida.

| Grafo | Vértices | Arestas | Versão | Execuções | Válidas | Fitness médio | Melhor fitness | Desvio | Tempo médio (s) |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| ciclo_n100.txt | 100 | 100 | V1_cromossomo | 10 | 10 | 56.000 | 55 | 0.816 | 0.023859 |
| ciclo_n100.txt | 100 | 100 | V2_grau_estatico | 10 | 10 | 55.600 | 54 | 1.075 | 0.014756 |
| ciclo_n100.txt | 100 | 100 | V3_grau_relativo | 10 | 10 | 52.600 | 52 | 0.516 | 0.027570 |
| ciclo_n100.txt | 100 | 100 | V4_gatilho | 10 | 10 | 51.000 | 51 | 0.000 | 0.013738 |
| cubic_100.txt | 100 | 150 | V1_cromossomo | 10 | 10 | 30.200 | 29 | 0.632 | 0.027848 |
| cubic_100.txt | 100 | 150 | V2_grau_estatico | 10 | 10 | 30.200 | 29 | 0.632 | 0.014904 |
| cubic_100.txt | 100 | 150 | V3_grau_relativo | 10 | 10 | 29.000 | 29 | 0.000 | 0.023092 |
| cubic_100.txt | 100 | 150 | V4_gatilho | 10 | 10 | 29.400 | 29 | 0.516 | 0.032626 |
| cycle1-order25.txt | 25 | 25 | V1_cromossomo | 10 | 10 | 14.100 | 14 | 0.316 | 0.004340 |
| cycle1-order25.txt | 25 | 25 | V2_grau_estatico | 10 | 10 | 14.200 | 14 | 0.422 | 0.002152 |
| cycle1-order25.txt | 25 | 25 | V3_grau_relativo | 10 | 10 | 14.000 | 14 | 0.000 | 0.003282 |
| cycle1-order25.txt | 25 | 25 | V4_gatilho | 10 | 10 | 14.000 | 14 | 0.000 | 0.004362 |
| cycle10-order250.txt | 250 | 250 | V1_cromossomo | 10 | 10 | 190.100 | 148 | 85.091 | 0.053768 |
| cycle10-order250.txt | 250 | 250 | V2_grau_estatico | 10 | 10 | 142.200 | 138 | 2.573 | 0.044817 |
| cycle10-order250.txt | 250 | 250 | V3_grau_relativo | 10 | 10 | 130.100 | 129 | 0.738 | 0.238424 |
| cycle10-order250.txt | 250 | 250 | V4_gatilho | 10 | 10 | 126.000 | 126 | 0.000 | 0.036695 |
| dwt__162.txt | 162 | 510 | V1_cromossomo | 10 | 10 | 12.000 | 12 | 0.000 | 0.044763 |
| dwt__162.txt | 162 | 510 | V2_grau_estatico | 10 | 10 | 12.400 | 12 | 0.516 | 0.022582 |
| dwt__162.txt | 162 | 510 | V3_grau_relativo | 10 | 10 | 12.800 | 12 | 0.422 | 0.027118 |
| dwt__162.txt | 162 | 510 | V4_gatilho | 10 | 10 | 11.900 | 11 | 0.316 | 0.029265 |
| grade_10x10.txt | 100 | 180 | V1_cromossomo | 10 | 10 | 16.900 | 16 | 0.568 | 0.020940 |
| grade_10x10.txt | 100 | 180 | V2_grau_estatico | 10 | 10 | 16.700 | 16 | 0.483 | 0.014687 |
| grade_10x10.txt | 100 | 180 | V3_grau_relativo | 10 | 10 | 18.200 | 17 | 0.789 | 0.022563 |
| grade_10x10.txt | 100 | 180 | V4_gatilho | 10 | 10 | 16.200 | 15 | 0.632 | 0.021989 |
| nos7.txt | 729 | 1944 | V1_cromossomo | 10 | 10 | 26.300 | 24 | 1.337 | 0.149720 |
| nos7.txt | 729 | 1944 | V2_grau_estatico | 10 | 10 | 26.000 | 25 | 0.816 | 0.171078 |
| nos7.txt | 729 | 1944 | V3_grau_relativo | 10 | 10 | 30.700 | 30 | 0.675 | 0.241242 |
| nos7.txt | 729 | 1944 | V4_gatilho | 10 | 10 | 25.900 | 25 | 0.876 | 0.199405 |
| random_denso_n50_p30.txt | 50 | 384 | V1_cromossomo | 10 | 10 | 4.000 | 4 | 0.000 | 0.013318 |
| random_denso_n50_p30.txt | 50 | 384 | V2_grau_estatico | 10 | 10 | 4.000 | 4 | 0.000 | 0.007411 |
| random_denso_n50_p30.txt | 50 | 384 | V3_grau_relativo | 10 | 10 | 4.000 | 4 | 0.000 | 0.006557 |
| random_denso_n50_p30.txt | 50 | 384 | V4_gatilho | 10 | 10 | 4.000 | 4 | 0.000 | 0.011569 |
| random_esparso_n200_d3.txt | 200 | 481 | V1_cromossomo | 10 | 10 | 14.700 | 14 | 0.675 | 0.065161 |
| random_esparso_n200_d3.txt | 200 | 481 | V2_grau_estatico | 10 | 10 | 14.400 | 13 | 0.699 | 0.043333 |
| random_esparso_n200_d3.txt | 200 | 481 | V3_grau_relativo | 10 | 10 | 18.000 | 18 | 0.000 | 0.022880 |
| random_esparso_n200_d3.txt | 200 | 481 | V4_gatilho | 10 | 10 | 14.000 | 13 | 0.471 | 0.049382 |
| steam3.txt | 80 | 424 | V1_cromossomo | 10 | 10 | 7.800 | 7 | 0.422 | 0.010919 |
| steam3.txt | 80 | 424 | V2_grau_estatico | 10 | 10 | 7.800 | 7 | 0.422 | 0.009917 |
| steam3.txt | 80 | 424 | V3_grau_relativo | 10 | 10 | 8.000 | 8 | 0.000 | 0.011903 |
| steam3.txt | 80 | 424 | V4_gatilho | 10 | 10 | 7.400 | 7 | 0.516 | 0.014776 |
