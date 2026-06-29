#ifndef fs_h
#define fs_h
#include <string>
#include <fstream>

/**
 * @param arquivoDaLista nome do arquivo em disco que contem a lista encadeada
 * @param novoNome nome a ser adicionado apos depoisDesteNome
 * @param depoisDesteNome um nome presente na lista
 */
void adiciona(std::string arquivoDaLista, std::string novoNome, std::string depoisDesteNome)
{
    const int TAM_NO = 28;
    const int TAM_NOME = 20;

    // abre o arquivo e conta os blocos
    std::fstream arq(arquivoDaLista, std::ios::in | std::ios::out | std::ios::binary);
    if (!arq) return;
    arq.seekg(0, std::ios::end);
    long tamArquivo = arq.tellg();
    int numBlocos = (tamArquivo - 4) / TAM_NO;

    // procura um bloco livre
    int offsetLivre = -1;
    for (int i = 0; i < numBlocos; i++) {
        int offset = 4 + i * TAM_NO;
        int uso = 0;
        arq.seekg(offset, std::ios::beg);
        arq.read((char *)&uso, 4);
        if (uso == 0) { offsetLivre = offset; break; }
    }
    if (offsetLivre < 0) return;
}

#endif /* fs_h */
