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

    // procura o no de referencia
    int offsetZ = -1;
    int proxDeZ = -1;
    for (int i = 0; i < numBlocos; i++) {
        int offset = 4 + i * TAM_NO;
        int uso = 0;
        char nome[TAM_NOME + 1] = {0};
        int prox = 0;
        arq.seekg(offset, std::ios::beg);
        arq.read((char *)&uso, 4);
        arq.read(nome, TAM_NOME);
        arq.read((char *)&prox, 4);
        if (uso == 1 && depoisDesteNome == std::string(nome)) {
            offsetZ = offset;
            proxDeZ = prox;
            break;
        }
    }
    if (offsetZ < 0) return;

    // grava o novo no no bloco livre, herdando o prox do no de referencia
    char nomeBuf[TAM_NOME] = {0};
    for (int i = 0; i < TAM_NOME && i < (int)novoNome.size(); i++)
        nomeBuf[i] = novoNome[i];
    int um = 1;
    arq.seekp(offsetLivre, std::ios::beg);
    arq.write((char *)&um, 4);
    arq.write(nomeBuf, TAM_NOME);
    arq.write((char *)&proxDeZ, 4);

    // liga o no de referencia ao novo no
    arq.seekp(offsetZ + 4 + TAM_NOME, std::ios::beg);
    arq.write((char *)&offsetLivre, 4);

    arq.close();
}

#endif /* fs_h */
