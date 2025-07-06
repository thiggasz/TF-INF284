import numpy as np
from scipy.stats import wilcoxon

'''
Instâncias,Greedy,,,,,Iterated Greddy,,,,,Artificial Bee Colony,,,,
Custo,IT_1,117,115,114,124,127,107,105,99,94,102,105,100,108,102,98
IT_2,220,223,229,214,227,192,186,194,173,196,197,193,189,196,193
IT_3,303,280,288,282,285,269,275,268,268,265,256,259,253,265,257
'''

greedy_data = {
    'IT_1': [117, 115, 114, 124, 127],
    'IT_2': [220, 223, 229, 214, 227],
    'IT_3': [303, 280, 288, 282, 285]
}

ig_data = {
    'IT_1': [107, 105, 99, 94, 102],
    'IT_2': [192, 186, 194, 173, 196],
    'IT_3': [269, 275, 268, 268, 265]
}

abc_data = {
    'IT_1': [105, 100, 108, 102, 98],
    'IT_2': [197, 193, 189, 196, 193],
    'IT_3': [256, 259, 253, 265, 257]
}

def wilcoxon_test(data1, data2):
    for key in data1:
        diffs = np.array(data1[key]) - np.array(data2[key])
        print(f"Diferença: {diffs}")

        if np.all(diffs == 0):
            print(f"{key}: todas as diferenças são 0 → p-valor = 1.0 (sem variação)\n")
            continue

        stat, p = wilcoxon(data1[key], data2[key], zero_method='wilcox')
        print(f"{key}: estatística={stat:.4f}, p-valor={p:.4f}\n")


print("Comparação entre Greedy e Iterated Greedy usando o teste de Wilcoxon:\n")
wilcoxon_test(greedy_data, ig_data)

print("Comparação entre Greedy e Artificial Bee Colony usando o teste de Wilcoxon:\n")
wilcoxon_test(greedy_data, abc_data)

print("Comparação entre Iterated Greedy e Artificial Bee Colony usando o teste de Wilcoxon:\n")
wilcoxon_test(ig_data, abc_data)
