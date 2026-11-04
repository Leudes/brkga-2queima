# Estimativa pessimista de tempo

Mede uma execução das seis versões nos grafos grandes `n4000_d24` e `psmigr_3` e projeta o tempo do experimento final completo.

| Grafo | Vértices | Arestas | Versão | Fitness | Tempo (s) | Estado |
|---|---:|---:|---:|---:|---:|---:|
| random__gilbert_n4000_d24_r1.txt | 4000 | 51971 | V1_cromossomo | 9 | 102.481 | ok |
| random__gilbert_n4000_d24_r1.txt | 4000 | 51971 | V2_grau_estatico | 9 | 87.3465 | ok |
| random__gilbert_n4000_d24_r1.txt | 4000 | 51971 | V3_reparo_grau_relativo | 9 | 24.552 | ok |
| random__gilbert_n4000_d24_r1.txt | 4000 | 51971 | V4_reparo_gatilho | 9 | 74.9736 | ok |
| random__gilbert_n4000_d24_r1.txt | 4000 | 51971 | V5_reparo_combinado | 9 | 100.778 | ok |
| random__gilbert_n4000_d24_r1.txt | 4000 | 51971 | V6_permutacao_pura | 9 | 89.0975 | ok |
| harwell_boeing__psmigr_3.txt | 3140 | 410781 | V1_cromossomo | 4 | 409.455 | ok |
| harwell_boeing__psmigr_3.txt | 3140 | 410781 | V2_grau_estatico | 4 | 362.854 | ok |
| harwell_boeing__psmigr_3.txt | 3140 | 410781 | V3_reparo_grau_relativo | 4 | 97.304 | ok |
| harwell_boeing__psmigr_3.txt | 3140 | 410781 | V4_reparo_gatilho | 4 | 255.998 | ok |
| harwell_boeing__psmigr_3.txt | 3140 | 410781 | V5_reparo_combinado | 4 | 398.621 | ok |
| harwell_boeing__psmigr_3.txt | 3140 | 410781 | V6_permutacao_pura | 4 | 365.178 | ok |

## Projeção

| Versão | Grafo de referência | Tempo medido (s) | Execuções | Horas | Dias |
|---|---:|---:|---:|---:|---:|
| V1_cromossomo | harwell_boeing__psmigr_3.txt | 410.892639 | 2625 | 299.609216 | 12.483717 |
| V2_grau_estatico | harwell_boeing__psmigr_3.txt | 364.178706 | 2625 | 265.546973 | 11.064457 |
| V3_reparo_grau_relativo | harwell_boeing__psmigr_3.txt | 98.852932 | 2625 | 72.080263 | 3.003344 |
| V4_reparo_gatilho | harwell_boeing__psmigr_3.txt | 257.099006 | 2625 | 187.468025 | 7.811168 |
| V5_reparo_combinado | harwell_boeing__psmigr_3.txt | 400.047286 | 2625 | 291.701146 | 12.154214 |
| V6_permutacao_pura | harwell_boeing__psmigr_3.txt | 366.488320 | 2625 | 267.231067 | 11.134628 |
| TOTAL_6_VERSOES | maior tempo de cada versao | 1897.558889 | 15750 | 1383.636690 | 57.651529 |
