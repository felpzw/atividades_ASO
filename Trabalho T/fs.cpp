/**
 * Implemente aqui as funções dos sistema de arquivos que simula EXT3
 */

#include "fs.h"
#include <fstream>
#include <cmath>
#include <vector>

// offsets de cada regiao do sistema de arquivos
struct Geometria {
    int tamBloco;
    int numBlocos;
    int numInodes;
    int offsetMapa;
    int offsetInodes;
    int offsetRaiz;
    int offsetBlocos;
};

// le o cabecalho e calcula onde comeca cada regiao
static Geometria leGeometria(std::fstream &arq)
{
    Geometria g;
    unsigned char cabecalho[3];
    arq.seekg(0, std::ios::beg);
    arq.read((char *)cabecalho, 3);
    g.tamBloco = cabecalho[0];
    g.numBlocos = cabecalho[1];
    g.numInodes = cabecalho[2];
    g.offsetMapa = 3;
    g.offsetInodes = g.offsetMapa + (int)std::ceil(g.numBlocos / 8.0);
    g.offsetRaiz = g.offsetInodes + g.numInodes * (int)sizeof(INODE);
    g.offsetBlocos = g.offsetRaiz + 1;
    return g;
}

static INODE leInode(std::fstream &arq, const Geometria &g, int indice)
{
    INODE ino;
    arq.seekg(g.offsetInodes + indice * (int)sizeof(INODE), std::ios::beg);
    arq.read((char *)&ino, sizeof(INODE));
    return ino;
}

static void gravaInode(std::fstream &arq, const Geometria &g, int indice, const INODE &ino)
{
    arq.seekp(g.offsetInodes + indice * (int)sizeof(INODE), std::ios::beg);
    arq.write((char *)&ino, sizeof(INODE));
}

// marca um bloco como ocupado (1) ou livre (0) no mapa de bits
static void marcaBloco(std::fstream &arq, const Geometria &g, int bloco, int ocupado)
{
    unsigned char byteMapa;
    arq.seekg(g.offsetMapa + bloco / 8, std::ios::beg);
    arq.read((char *)&byteMapa, 1);
    if (ocupado)
        byteMapa |= (1 << (bloco % 8));
    else
        byteMapa &= ~(1 << (bloco % 8));
    arq.seekp(g.offsetMapa + bloco / 8, std::ios::beg);
    arq.write((char *)&byteMapa, 1);
}

// procura o primeiro bloco livre no mapa de bits e ja o marca como ocupado
static int alocaBloco(std::fstream &arq, const Geometria &g)
{
    for (int i = 0; i < g.numBlocos; i++) {
        unsigned char byteMapa;
        arq.seekg(g.offsetMapa + i / 8, std::ios::beg);
        arq.read((char *)&byteMapa, 1);
        if (!(byteMapa & (1 << (i % 8)))) {
            marcaBloco(arq, g, i, 1);
            return i;
        }
    }
    return -1;
}

// procura o primeiro inode livre (IS_USED = 0)
static int procuraInodeLivre(std::fstream &arq, const Geometria &g)
{
    for (int i = 0; i < g.numInodes; i++) {
        if (leInode(arq, g, i).IS_USED == 0)
            return i;
    }
    return -1;
}

// quantidade de blocos usados por um inode (diretorio sempre ocupa ao menos 1)
static int blocosUsados(const Geometria &g, const INODE &ino)
{
    int blocos = (int)std::ceil((unsigned char)ino.SIZE / (double)g.tamBloco);
    if (ino.IS_DIR && blocos == 0) blocos = 1;
    return blocos;
}

// le o n-esimo byte da area de dados de um inode (seguindo os blocos diretos)
static unsigned char leByteDoInode(std::fstream &arq, const Geometria &g, const INODE &ino, int n)
{
    int bloco = ino.DIRECT_BLOCKS[n / g.tamBloco];
    unsigned char byteLido;
    arq.seekg(g.offsetBlocos + bloco * g.tamBloco + n % g.tamBloco, std::ios::beg);
    arq.read((char *)&byteLido, 1);
    return byteLido;
}

// grava o n-esimo byte da area de dados de um inode
static void gravaByteDoInode(std::fstream &arq, const Geometria &g, const INODE &ino, int n, unsigned char valor)
{
    int bloco = ino.DIRECT_BLOCKS[n / g.tamBloco];
    arq.seekp(g.offsetBlocos + bloco * g.tamBloco + n % g.tamBloco, std::ios::beg);
    arq.write((char *)&valor, 1);
}

// separa um caminho "/a/b/c" nos componentes {"a", "b", "c"}
static std::vector<std::string> divideCaminho(const std::string &caminho)
{
    std::vector<std::string> partes;
    std::string parte;
    for (char c : caminho) {
        if (c == '/') {
            if (!parte.empty()) partes.push_back(parte);
            parte.clear();
        } else {
            parte += c;
        }
    }
    if (!parte.empty()) partes.push_back(parte);
    return partes;
}

// nome de um inode como string (NAME tem no maximo 10 bytes)
static std::string nomeDoInode(const INODE &ino)
{
    std::string nome;
    for (int i = 0; i < 10 && ino.NAME[i] != '\0'; i++)
        nome += ino.NAME[i];
    return nome;
}

// procura o inode de um caminho partindo da raiz; devolve -1 se nao achar.
// se idxPai for passado, recebe o indice do inode do diretorio pai
static int procuraInode(std::fstream &arq, const Geometria &g, const std::string &caminho, int *idxPai = nullptr)
{
    std::vector<std::string> partes = divideCaminho(caminho);
    int atual = 0; // raiz
    if (idxPai) *idxPai = -1;

    for (const std::string &parte : partes) {
        INODE dir = leInode(arq, g, atual);
        int filhoEncontrado = -1;
        for (int i = 0; i < (unsigned char)dir.SIZE; i++) {
            int filho = leByteDoInode(arq, g, dir, i);
            if (nomeDoInode(leInode(arq, g, filho)) == parte) {
                filhoEncontrado = filho;
                break;
            }
        }
        if (filhoEncontrado < 0) return -1;
        if (idxPai) *idxPai = atual;
        atual = filhoEncontrado;
    }
    return atual;
}

// registra um filho no diretorio pai: incrementa SIZE e grava o indice do filho
// no primeiro byte livre dos blocos do pai (alocando um novo bloco se preciso)
static void adicionaFilho(std::fstream &arq, const Geometria &g, int idxPai, int idxFilho)
{
    INODE pai = leInode(arq, g, idxPai);
    int posicao = (unsigned char)pai.SIZE;
    int capacidade = blocosUsados(g, pai) * g.tamBloco;
    if (posicao == capacidade)
        pai.DIRECT_BLOCKS[posicao / g.tamBloco] = alocaBloco(arq, g);
    gravaByteDoInode(arq, g, pai, posicao, (unsigned char)idxFilho);
    pai.SIZE++;
    gravaInode(arq, g, idxPai, pai);
}

// monta um inode novo com nome preenchido e demais campos zerados
static INODE montaInode(const std::string &nome, int isDir, int tamanho)
{
    INODE ino{};
    ino.IS_USED = 0x01;
    ino.IS_DIR = (unsigned char)isDir;
    ino.SIZE = (char)tamanho;
    for (int i = 0; i < 10 && i < (int)nome.size(); i++)
        ino.NAME[i] = nome[i];
    return ino;
}

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
    std::fstream arq(fsFileName, std::ios::in | std::ios::out | std::ios::binary);
    if (!arq) return;
    Geometria g = leGeometria(arq);

    // separa o diretorio pai do nome do arquivo
    std::vector<std::string> partes = divideCaminho(filePath);
    std::string nome = partes.back();
    std::string caminhoPai = filePath.substr(0, filePath.size() - nome.size());
    int idxPai = procuraInode(arq, g, caminhoPai);
    if (idxPai < 0) return;

    int idxNovo = procuraInodeLivre(arq, g);
    if (idxNovo < 0) return;

    // aloca os blocos necessarios e grava o conteudo neles
    INODE novo = montaInode(nome, 0, fileContent.size());
    int numBlocosArquivo = (int)std::ceil(fileContent.size() / (double)g.tamBloco);
    for (int i = 0; i < numBlocosArquivo; i++)
        novo.DIRECT_BLOCKS[i] = alocaBloco(arq, g);
    for (int i = 0; i < (int)fileContent.size(); i++)
        gravaByteDoInode(arq, g, novo, i, fileContent[i]);

    gravaInode(arq, g, idxNovo, novo);
    adicionaFilho(arq, g, idxPai, idxNovo);
    arq.close();
}

void addDir(std::string fsFileName, std::string dirPath)
{
    std::fstream arq(fsFileName, std::ios::in | std::ios::out | std::ios::binary);
    if (!arq) return;
    Geometria g = leGeometria(arq);

    // separa o diretorio pai do nome do novo diretorio
    std::vector<std::string> partes = divideCaminho(dirPath);
    std::string nome = partes.back();
    std::string caminhoPai = dirPath.substr(0, dirPath.size() - nome.size());
    int idxPai = procuraInode(arq, g, caminhoPai);
    if (idxPai < 0) return;

    int idxNovo = procuraInodeLivre(arq, g);
    if (idxNovo < 0) return;

    // todo diretorio recem criado recebe um bloco vazio, mesmo sem filhos
    INODE novo = montaInode(nome, 1, 0);
    novo.DIRECT_BLOCKS[0] = alocaBloco(arq, g);

    gravaInode(arq, g, idxNovo, novo);
    adicionaFilho(arq, g, idxPai, idxNovo);
    arq.close();
}

// retira um filho da lista do pai: desloca os indices seguintes para a esquerda,
// decrementa SIZE e libera no mapa de bits o bloco do pai que ficou vazio
static void removeFilho(std::fstream &arq, const Geometria &g, int idxPai, int idxFilho)
{
    INODE pai = leInode(arq, g, idxPai);
    int numFilhos = (unsigned char)pai.SIZE;

    int k = 0;
    while (k < numFilhos && leByteDoInode(arq, g, pai, k) != idxFilho)
        k++;
    for (int j = k; j < numFilhos - 1; j++)
        gravaByteDoInode(arq, g, pai, j, leByteDoInode(arq, g, pai, j + 1));

    int blocosAntes = blocosUsados(g, pai);
    pai.SIZE--;
    if (blocosUsados(g, pai) < blocosAntes)
        marcaBloco(arq, g, pai.DIRECT_BLOCKS[blocosAntes - 1], 0);
    gravaInode(arq, g, idxPai, pai);
}

// apaga um inode e seus descendentes: libera os blocos no mapa de bits e zera
// IS_USED, deixando os demais campos e o conteudo dos blocos como lixo
static void apagaRecursivo(std::fstream &arq, const Geometria &g, int indice)
{
    INODE ino = leInode(arq, g, indice);

    if (ino.IS_DIR) {
        std::vector<int> filhos;
        for (int i = 0; i < (unsigned char)ino.SIZE; i++)
            filhos.push_back(leByteDoInode(arq, g, ino, i));
        for (int filho : filhos)
            apagaRecursivo(arq, g, filho);
    }

    for (int i = 0; i < blocosUsados(g, ino); i++)
        marcaBloco(arq, g, ino.DIRECT_BLOCKS[i], 0);

    ino.IS_USED = 0x00;
    gravaInode(arq, g, indice, ino);
}

void remove(std::string fsFileName, std::string path)
{
    std::fstream arq(fsFileName, std::ios::in | std::ios::out | std::ios::binary);
    if (!arq) return;
    Geometria g = leGeometria(arq);

    int idxPai = -1;
    int indice = procuraInode(arq, g, path, &idxPai);
    if (indice <= 0) return; // nao achou ou tentou remover a raiz

    apagaRecursivo(arq, g, indice);
    removeFilho(arq, g, idxPai, indice);
    arq.close();
}

void move(std::string fsFileName, std::string oldPath, std::string newPath)
{
}
