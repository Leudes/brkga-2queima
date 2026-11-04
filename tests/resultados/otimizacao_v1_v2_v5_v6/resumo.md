# Otimização das versões V1, V2, V5 e V6

Compara o tempo antes e depois das otimizações de código, verificando também se o fitness permaneceu idêntico.

| Grafo | Vértices | Arestas | Versão | Repetições | Tempo anterior (s) | Tempo otimizado (s) | Redução (%) | Resultado idêntico |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| random__gilbert_n3000_d12_r1.txt | 3000 | 21034 | V1 | 3 | 0.663559 | 0.619849 | 6.6 | sim |
| random__gilbert_n3000_d12_r1.txt | 3000 | 21034 | V2 | 3 | 0.628814 | 0.534241 | 15.0 | sim |
| random__gilbert_n3000_d12_r1.txt | 3000 | 21034 | V5 | 3 | 0.580667 | 0.543102 | 6.5 | sim |
| random__gilbert_n3000_d12_r1.txt | 3000 | 21034 | V6 | 3 | 0.669542 | 0.545765 | 18.5 | sim |
| random__gilbert_n4000_d24_r1.txt | 4000 | 51971 | V5 | 3 | 1.258013 | 1.065062 | 15.3 | sim |
| snap__as19980416.txt | 3595 | 6458 | V5 | 3 | 3.206397 | 1.197287 | 62.7 | sim |
| harwell_boeing__psmigr_3.txt | 3140 | 410781 | V5 | 1 | 0.308695 | 0.266314 | 13.7 | sim |
