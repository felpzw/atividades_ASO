/**
 * Implemente aqui as funções dos sistema de arquivos que simula EXT3
 */

#include "fs.h"
#include <fstream>
#include <cmath>
#include <vector>

/**
 * @brief Inicializa um sistema de arquivos que simula EXT3
 * @param fsFileName nome do arquivo que contém sistema de arquivos que simula EXT3 (caminho do arquivo no sistema de arquivos local)
 * @param blockSize tamanho em bytes do bloco
 * @param numBlocks quantidade de blocos
 * @param numInodes quantidade de inodes
 */
void initFs(std::string fsFileName, int blockSize, int numBlocks, int numInodes)
{
    std::ofstream arq(fsFileName, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!arq) return;

    // cabecalho: tamanho do bloco, numero de blocos e numero de inodes
    char cabecalho[3] = {(char)blockSize, (char)numBlocks, (char)numInodes};
    arq.write(cabecalho, 3);

    // mapa de bits: somente o bloco 0 esta ocupado (diretorio raiz)
    int tamMapa = (int)std::ceil(numBlocks / 8.0);
    std::vector<char> mapa(tamMapa, 0);
    mapa[0] = 0x01;
    arq.write(mapa.data(), tamMapa);

    // vetor de inodes: inode 0 e o diretorio raiz "/", os demais ficam livres
    INODE raiz{};
    raiz.IS_USED = 0x01;
    raiz.IS_DIR = 0x01;
    raiz.NAME[0] = '/';
    raiz.SIZE = 0;
    arq.write((char *)&raiz, sizeof(INODE));

    INODE vazio{};
    for (int i = 1; i < numInodes; i++)
        arq.write((char *)&vazio, sizeof(INODE));

    // indice do inode do diretorio raiz
    char indiceRaiz = 0;
    arq.write(&indiceRaiz, 1);

    // vetor de blocos zerado
    std::vector<char> blocos(blockSize * numBlocks, 0);
    arq.write(blocos.data(), blocos.size());

    arq.close();
}

void addFile(std::string fsFileName, std::string filePath, std::string fileContent)
{
}

void addDir(std::string fsFileName, std::string dirPath)
{
}

void remove(std::string fsFileName, std::string path)
{
}

void move(std::string fsFileName, std::string oldPath, std::string newPath)
{
}
