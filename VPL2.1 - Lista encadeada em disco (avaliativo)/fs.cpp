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
}

#endif /* fs_h */
