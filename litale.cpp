#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <locale.h>
#include <ctype.h>


#define MAX_NOME 50
#define MAX_FABRICANTE 30
#define MAX_VALIDADE 11
#define MAX_FUNCIONALIDADE 50
#define TAM_REGISTRO 200
#define ORDEM 3
#define ARQUIVO_DADOS "dados.txt"
#define ARQUIVO_INDICE "indice.idx"
#define TAM_INDICE 30

//estruturas
typedef struct {
    char codigo[15];
    char nome[MAX_NOME];
    char fabricante[MAX_FABRICANTE];
    float preco;
    char validade[MAX_VALIDADE];
    char funcionalidade[MAX_FUNCIONALIDADE];
    char categoria[20];
} Produto;

typedef struct BTreeNode {
    int n;
    char chaves[2 * ORDEM - 1][15];
    int offsets[2 * ORDEM - 1];
    struct BTreeNode *filhos[2 * ORDEM];
    bool folha;
} BTreeNode;

typedef struct {
    BTreeNode *raiz;
    int grau;
} BTree;

//prototipos
void escreverProdutoArquivo(Produto *p);
int salvarProduto(Produto *p);
Produto lerProduto(int offset);
BTreeNode* criarNo(bool folha);
BTree* criarArvore();
void dividirFilho(BTreeNode *pai, int i, BTreeNode *y);
void inserirNaoCheio(BTreeNode *x, char *k, int offset);
void inserir(BTree *T, char *k, int offset);
void buscarPorFuncionalidade(BTree *arvore);
int buscarPorFuncionalidadeNo(BTreeNode *no, char *termo);
void exibirMenuPrincipal();
void cadastrarProduto(BTree *arvore);
void buscarPorCodigo(BTree *arvore);
void listarTodosProdutos();
void liberarNo(BTreeNode *no);
void liberarArvore(BTree *arvore);
void salvarIndice(char *codigo, int offset);
int buscarOffsetNoIndice(char *codigo);
void reconstruirIndice(BTree *arvore);

void escreverProdutoArquivo(Produto *p) {
    FILE *f = fopen(ARQUIVO_DADOS, "a");
    if (!f) {
        printf("Erro ao abrir arquivo para escrita!\n");
        return;
    }

    char nome_limpo[MAX_NOME];
    strcpy(nome_limpo, p->nome);
    char *pipe_pos;
    while ((pipe_pos = strchr(nome_limpo, '|')) != NULL) {
        *pipe_pos = '-';
    }
    
    fprintf(f, "%s|%s|%s|%.2f|%s|%s|%s\n", 
        p->codigo,
        nome_limpo,
        p->fabricante,
        p->preco,
        p->validade,
        p->funcionalidade,
        p->categoria);
    
    fclose(f);
}

int salvarProduto(Produto *p) {
    FILE *f = fopen(ARQUIVO_DADOS, "a");
    if (!f) return -1;
    
    fseek(f, 0, SEEK_END);
    int offset = ftell(f);
    
    escreverProdutoArquivo(p);
    fclose(f);

    salvarIndice(p->codigo, offset);
    
    return offset;
}

Produto lerProduto(int offset) {
    Produto p = {0};

    FILE *f = fopen(ARQUIVO_DADOS, "r");
    if (!f) return p;
    
    fseek(f, offset, SEEK_SET);
    char linha[TAM_REGISTRO];
    
    if (fgets(linha, sizeof(linha), f)) {
        linha[strcspn(linha, "\n")] = '\0';
        sscanf(linha, "%14[^|]|%50[^|]|%30[^|]|%f|%10[^|]|%50[^|]|%15[^\n]", 
            p.codigo, p.nome, p.fabricante, &p.preco, p.validade, p.funcionalidade, p.categoria);
    }
    
    fclose(f);
    return p;
}

void salvarIndice(char *codigo, int offset) {
    FILE *f = fopen(ARQUIVO_INDICE, "ab");
    if (!f) {
        printf("Erro ao abrir arquivo de índice para escrita!\n");
        return;
    }
    
    fwrite(codigo, sizeof(char), 15, f);
    fwrite(&offset, sizeof(int), 1, f);
    
    fclose(f);
}

int buscarOffsetNoIndice(char *codigo) {
    FILE *f = fopen(ARQUIVO_INDICE, "rb");
    if (!f) return -1;
    
    char codigoLido[15];
    int offset;
    
    while (fread(codigoLido, sizeof(char), 15, f) == 15) {
        if (fread(&offset, sizeof(int), 1, f) != 1) break;
        
        if (strcmp(codigoLido, codigo) == 0) {
            fclose(f);
            return offset;
        }
    }
    
    fclose(f);
    return -1;
}

void reconstruirIndice(BTree *arvore) {
    FILE *f = fopen(ARQUIVO_INDICE, "wb");
    if (!f) {
        printf("Erro ao criar arquivo de índice!\n");
        return;
    }
    
    BTreeNode *no = arvore->raiz;
    for (int i = 0; i < no->n; i++) {
        fwrite(no->chaves[i], sizeof(char), 15, f);
        fwrite(&no->offsets[i], sizeof(int), 1, f);
    }
    
    fclose(f);
}

void listarEmOrdem(BTreeNode *no, int *contador) {
    if (!no) return;

    for (int i = 0; i < no->n; i++) {
        if (!no->folha) {
            listarEmOrdem(no->filhos[i], contador);
        }

        Produto p = lerProduto(no->offsets[i]);
        (*contador)++;
        printf("+--------------------------------------------+\n");
        printf("| PRODUTO #%-3d %-28s |\n", *contador, p.categoria);
        printf("+--------------------------------------------+\n");

        printf("Código: %s\n", p.codigo);
        printf("Nome: %s\n", p.nome);
        printf("Fabricante: %s\n", p.fabricante);

        char preco_str[20];
        snprintf(preco_str, sizeof(preco_str), "R$%.2f", p.preco);
        printf("Preço: %s\n", preco_str);

        printf("Validade: %s\n", p.validade);
        printf("Funcionalidade: %s\n\n", p.funcionalidade);
    }

    if (!no->folha) {
        listarEmOrdem(no->filhos[no->n], contador);
    }
}
//arvore b
BTreeNode* criarNo(bool folha) {
    BTreeNode *n = (BTreeNode*)malloc(sizeof(BTreeNode));
    if (!n) return NULL;
    
    n->n = 0;
    n->folha = folha;
    
    for (int i = 0; i < 2 * ORDEM; i++) {
        n->filhos[i] = NULL;
    }
    
    for (int i = 0; i < 2 * ORDEM - 1; i++) {
        n->chaves[i][0] = '\0';
        n->offsets[i] = -1;
    }
    
    return n;
}

BTree* criarArvore() {
    BTree *arvore = (BTree*)malloc(sizeof(BTree));
    if (!arvore) return NULL;
    
    arvore->raiz = criarNo(true);
    arvore->grau = ORDEM;
    return arvore;
}

void dividirFilho(BTreeNode *pai, int i, BTreeNode *y) {
    BTreeNode *z = criarNo(y->folha);
    z->n = ORDEM - 1;
    
    for (int j = 0; j < ORDEM - 1; j++) {
        strcpy(z->chaves[j], y->chaves[j + ORDEM]);
        z->offsets[j] = y->offsets[j + ORDEM];
    }
    
    if (!y->folha) {
        for (int j = 0; j < ORDEM; j++) {
            z->filhos[j] = y->filhos[j + ORDEM];
        }
    }
    
    y->n = ORDEM - 1;
    
    for (int j = pai->n; j >= i + 1; j--) {
        pai->filhos[j + 1] = pai->filhos[j];
    }
    
    pai->filhos[i + 1] = z;
    
    for (int j = pai->n - 1; j >= i; j--) {
        strcpy(pai->chaves[j + 1], pai->chaves[j]);
        pai->offsets[j + 1] = pai->offsets[j];
    }
    
    strcpy(pai->chaves[i], y->chaves[ORDEM - 1]);
    pai->offsets[i] = y->offsets[ORDEM - 1];
    pai->n++;
}

void inserirNaoCheio(BTreeNode *x, char *k, int offset) {
    int i = x->n - 1;
    
    if (x->folha) {
        while (i >= 0 && strcmp(k, x->chaves[i]) < 0) {
            strcpy(x->chaves[i + 1], x->chaves[i]);
            x->offsets[i + 1] = x->offsets[i];
            i--;
        }
        strcpy(x->chaves[i + 1], k);
        x->offsets[i + 1] = offset;
        x->n++;
    } else {
        while (i >= 0 && strcmp(k, x->chaves[i]) < 0) i--;
        i++;
        
        if (x->filhos[i]->n == 2 * ORDEM - 1) {
            dividirFilho(x, i, x->filhos[i]);
            if (strcmp(k, x->chaves[i]) > 0) i++;
        }
        inserirNaoCheio(x->filhos[i], k, offset);
    }
}

void inserir(BTree *T, char *k, int offset) {
    BTreeNode *r = T->raiz;
    
    if (r->n == 2 * ORDEM - 1) {
        BTreeNode *s = criarNo(false);
        T->raiz = s;
        s->filhos[0] = r;
        dividirFilho(s, 0, r);
        inserirNaoCheio(s, k, offset);
    } else {
        inserirNaoCheio(r, k, offset);
    }
}

//busca
void normalizarString(char *str) {
    static const char *comAcento = "ÁáÀàÂâÃãÉéÈèÊêÍíÌìÎîÓóÒòÔôÕõÚúÙùÛûÇç";
    static const char *semAcento = "AaAaAaAaEeEeEeIiIiIiOoOoOoOoUuUuUuCc";
    
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
        
        const char *pos = strchr(comAcento, str[i]);
        if (pos) {
            int index = pos - comAcento;
            str[i] = semAcento[index];
        }
    }
}
int buscarPorFuncionalidadeNo(BTreeNode *no, char *termo) {
    int encontrados = 0;
    if (!no) return 0;

    char termoLower[MAX_FUNCIONALIDADE];
    strcpy(termoLower, termo);
    for(int i = 0; termoLower[i]; i++) {
        termoLower[i] = tolower(termoLower[i]);
    }

    for (int i = 0; i < no->n; i++) {
        Produto p = lerProduto(no->offsets[i]);

        char funcLower[MAX_FUNCIONALIDADE];
        strcpy(funcLower, p.funcionalidade);
        for(int j = 0; funcLower[j]; j++) {
            funcLower[j] = tolower(funcLower[j]);
        }

        int corresponde = 0;

        if (
            (strcmp(p.categoria, "medicamento") == 0) ||
            (strcmp(p.categoria, "cosmetico") == 0) ||
            (strcmp(p.categoria, "higiene") == 0) ||
            (strcmp(p.categoria, "suplemento") == 0)
        ) {
            if (strstr(funcLower, termoLower) != NULL) {
                corresponde = 1;
            }
        }

        if(corresponde) {
            encontrados++;
            printf("\n%s\n", p.nome);
            printf("Categoria: %s\n", p.categoria);
            printf("Funcionalidade: %s\n\n", p.funcionalidade);
        }
    }

    if(!no->folha) {
        for(int i = 0; i <= no->n; i++) {
            encontrados += buscarPorFuncionalidadeNo(no->filhos[i], termo);
        }
    }

    return encontrados;
}



void buscarPorFuncionalidade(BTree *arvore) {
    system("cls");
    printf("-------------------------------------------\n");
    printf("      BUSCA DE PRODUTOS POR FUNCIONALIDADE \n");
    printf("-------------------------------------------\n\n");
    printf("Digite o termo de busca (ex: dor) ou '0' para voltar : ");
    
    char termo[50];
    if (fgets(termo, 50, stdin)) {
        termo[strcspn(termo, "\n")] = '\0';
        
		if (strcmp(termo, "0") == 0 || strcmp(termo, "voltar") == 0) {
    printf("Busca cancelada. Retornando ao menu principal...\n");
    return;
}

        
        if (strlen(termo) > 0) {
            system("cls");
            printf("-------------------------------------------\n");
            printf("  PRODUTOS COM A FUNCIONALIDADE: \"%s\"  \n", termo);
            printf("-------------------------------------------\n\n");
            
            int encontrados = buscarPorFuncionalidadeNo(arvore->raiz, termo);
            
            printf("\n-------------------------------------------\n");
            printf("Total encontrado: %d produto(s)\n", encontrados);
            printf("-------------------------------------------\n");
        } else {
            printf("\nTermo de busca não pode ser vazio!\n");
        }
    }
    
    printf("\nPressione qualquer tecla para continuar...");
    getchar();
}
//menu funções
void exibirMenuPrincipal() {
    system("cls");
    printf("-------------------------------------------\n");
    printf("        SANTO REMÉDIO                      \n");
    printf("-------------------------------------------\n\n");
    printf("1. Cadastrar novo produto\n");
    printf("2. Buscar produto por código\n");
    printf("3. Buscar medicamentos por funcionalidade\n");
    printf("4. Listar todos os produtos\n");
    printf("5. Editar produto\n");
    printf("6. Remover produto\n");
    printf("7. Reconstruir índice\n");
    printf("8. Sair\n\n");
    printf("Escolha uma opção: ");
}


void cadastrarProduto(BTree *arvore) {
    Produto p;
    system("cls");
    printf("-------------------------------------------\n");
    printf("          CADASTRO DE NOVO PRODUTO         \n");
    printf("-------------------------------------------\n\n");

    int codigoExistente = 1;
    while (codigoExistente) {
        printf("Código (14 dígitos) ou '0' para voltar: ");
        scanf("%14s", p.codigo);
        getchar();
        
    if (strcmp(p.codigo, "0") == 0 || strcmpi(p.codigo, "voltar") == 0) {
        printf("Cadastro cancelado. Retornando ao menu principal...\n");
        return;
    } 
        
        BTreeNode *noAtual = arvore->raiz;
        codigoExistente = 0;
        
        while (noAtual != NULL && !codigoExistente) {
            int i = 0;
            while (i < noAtual->n && strcmp(p.codigo, noAtual->chaves[i]) > 0) {
                i++;
            }
            
            if (i < noAtual->n && strcmp(p.codigo, noAtual->chaves[i]) == 0) {
                codigoExistente = 1;
                printf("\nERRO: Já existe um produto com este código!\n");
                printf("Por favor, insira um código diferente.\n\n");
            }
            
            if (!codigoExistente) {
                if (noAtual->folha) {
                    noAtual = NULL;
                } else {
                    noAtual = noAtual->filhos[i];
                }
            }
        }
    }
    
    printf("Nome: ");
    fgets(p.nome, MAX_NOME, stdin);
    p.nome[strcspn(p.nome, "\n")] = '\0';
    
    printf("Fabricante: ");
    fgets(p.fabricante, MAX_FABRICANTE, stdin);
    p.fabricante[strcspn(p.fabricante, "\n")] = '\0';
    
    printf("Preço: R$");
    scanf("%f", &p.preco);
    
    printf("Validade (MM/AAAA): ");
    scanf("%10s", p.validade);
    
    printf("Funcionalidade: ");
    getchar();
    fgets(p.funcionalidade, MAX_FUNCIONALIDADE, stdin);
    p.funcionalidade[strcspn(p.funcionalidade, "\n")] = '\0';
    
    printf("Categoria (medicamento/higiene/cosmético/suplemento): ");
    fgets(p.categoria, 20, stdin);
    p.categoria[strcspn(p.categoria, "\n")] = '\0';
    
    int offset = salvarProduto(&p);
    if (offset != -1) {
        inserir(arvore, p.codigo, offset);
        printf("\nProduto cadastrado com sucesso!\n");
    } else {
        printf("\nErro ao cadastrar produto!\n");
    }
    system("pause");
}


void buscarPorCodigo(BTree *arvore) {
    system("cls");
    printf("-------------------------------------------\n");
    printf("           BUSCA DE PRODUTO POR CÓDIGO     \n");
    printf("-------------------------------------------\n\n");
    printf("Digite o código do produto (14 dígitos) ou '0' para voltar: ");
    
    char codigo[15];
    scanf("%14s", codigo);
    getchar();

    if (strcmp(codigo, "0") == 0 || strcmp(codigo, "voltar") == 0) {
        printf("Busca cancelada. Retornando ao menu principal...\n");
        return;
    }
  
    BTreeNode *noAtual = arvore->raiz;
    int encontrado = 0;
    
    while (noAtual != NULL && !encontrado) {
        int i = 0;
        while (i < noAtual->n && strcmp(codigo, noAtual->chaves[i]) > 0) {
            i++;
        }
        
        if (i < noAtual->n && strcmp(codigo, noAtual->chaves[i]) == 0) {
 
    Produto p = lerProduto(noAtual->offsets[i]);
    printf("\n%s\n", p.nome);
    printf("Código: %s\n", p.codigo);
    printf("Fabricante: %s\n", p.fabricante);
    printf("Preço: R$%.2f\n", p.preco);
    printf("Validade: %s\n", p.validade);
    printf("Funcionalidade: %s\n", p.funcionalidade);
    printf("Categoria: %s\n\n", p.categoria);
    encontrado = 1;
    }
        
        if (!encontrado) {
            if (noAtual->folha) {
                noAtual = NULL;
            } else {
                noAtual = noAtual->filhos[i];
            }
        }
    }
    
    if (!encontrado) {
        printf("\nProduto com o código '%s' não encontrado!\n", codigo);
    }

    printf("\nPressione qualquer tecla para continuar...");
    getchar();
}
int contarCaracteresVisiveis(const char *str) {
    int count = 0;
    while (*str) {
        if ((*str & 0xC0) != 0x80) { 
            count++;
        }
        str++;
    }
    return count;
}

void imprimirCampoAlinhado(const char *rotulo, const char *valor, int largura_total) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s: %s", rotulo, valor);
    
    int tamanho_rotulo = contarCaracteresVisiveis(rotulo) + 2; 
    int espaco_disponivel = largura_total - tamanho_rotulo - 4;
    
    printf("| %s: ", rotulo);
    
    int pos = 0;
    int len = contarCaracteresVisiveis(valor);
    
    while (pos < len) {
        int chunk = (len - pos) > espaco_disponivel ? espaco_disponivel : (len - pos);
        
        int bytes = 0;
        int chars = 0;
        while (chars < chunk && valor[bytes]) {
            if ((valor[bytes] & 0xC0) != 0x80) chars++;
            bytes++;
        }
        
        printf("%.*s", bytes, valor + pos);
        
        if (chars < espaco_disponivel) {
            printf("%*s", espaco_disponivel - chars, "");
        }
        
        pos += bytes;
        
        if (pos < len) {
            printf(" |\n| %*s", tamanho_rotulo, ""); 
        }
    }
    
    printf(" |\n");
}

//função que atualiza produto no arquivo
void atualizarProdutoNoArquivo(int offset, Produto *p) {
    FILE *f = fopen(ARQUIVO_DADOS, "r+");
    if (!f) {
        printf("Erro ao abrir arquivo para atualização!\n");
        return;
    }
    
    fseek(f, offset, SEEK_SET);
    char nome_limpo[MAX_NOME];
    strcpy(nome_limpo, p->nome);
    char *pipe_pos;
    while ((pipe_pos = strchr(nome_limpo, '|')) != NULL) {
        *pipe_pos = '-';
    }
    
    fprintf(f, "%s|%s|%s|%.2f|%s|%s|%s", 
        p->codigo,
        nome_limpo,
        p->fabricante,
        p->preco,
        p->validade,
        p->funcionalidade,
        p->categoria);
    
    long pos = ftell(f);
    fseek(f, 0, SEEK_END);
    long end = ftell(f);
    if (pos < end) {
        while (pos++ < end) {
            fputc(' ', f);
        }
    }
    
    fclose(f);
}

void editarProduto(BTree *arvore) {
    system("cls");
    printf("-------------------------------------------\n");
    printf("            EDITAR PRODUTO                 \n");
    printf("-------------------------------------------\n\n");
    printf("Digite o código do produto que deseja editar (14 dígitos) ou '0' para voltar: ");
    
    char codigo[15];
    scanf("%14s", codigo);
    getchar();
    
     if (strcmp(codigo, "0") == 0 || strcmp(codigo, "voltar") == 0) {
        printf("Edição cancelada. Retornando ao menu principal...\n");
        return;
    }
    
    BTreeNode *noAtual = arvore->raiz;
    int encontrado = 0;
    int offset = -1;
    
    while (noAtual != NULL && !encontrado) {
        int i = 0;
        while (i < noAtual->n && strcmp(codigo, noAtual->chaves[i]) > 0) {
            i++;
        }
    
        if (i < noAtual->n && strcmp(codigo, noAtual->chaves[i]) == 0) {
            offset = noAtual->offsets[i];
            encontrado = 1;
        }
        
        if (!encontrado) {
            if (noAtual->folha) {
                noAtual = NULL;
            } else {
                noAtual = noAtual->filhos[i];
            }
        }
    }
    
    if (!encontrado) {
        printf("\nProduto com o código '%s' não encontrado!\n", codigo);
        printf("\nPressione qualquer tecla para continuar...");
        getchar();
        return;
    }
    Produto p = lerProduto(offset);
    printf("\nDados atuais do produto:\n");
    printf("1. Nome: %s\n", p.nome);
    printf("2. Fabricante: %s\n", p.fabricante);
    printf("3. Preço: R$%.2f\n", p.preco);
    printf("4. Validade (MM/AAAA): %s\n", p.validade);
    printf("5. Funcionalidade: %s\n", p.funcionalidade);
    printf("6. Categoria: %s\n", p.categoria);
    printf("\nQual campo deseja editar? (1-6, 0 para cancelar): ");
    int opcao;
    scanf("%d", &opcao);
    getchar();
    
    if (opcao < 1 || opcao > 6) {
        printf("Edição cancelada.\n");
        printf("\nPressione qualquer tecla para continuar...");
        getchar();
        return;
    }
    switch(opcao) {
        case 1:
            printf("Novo nome: ");
            fgets(p.nome, MAX_NOME, stdin);
            p.nome[strcspn(p.nome, "\n")] = '\0';
            break;
        case 2:
            printf("Novo fabricante: ");
            fgets(p.fabricante, MAX_FABRICANTE, stdin);
            p.fabricante[strcspn(p.fabricante, "\n")] = '\0';
            break;
        case 3:
            printf("Novo preço: R$");
            scanf("%f", &p.preco);
            getchar();
            break;
        case 4:
            printf("Nova validade (MM/AAAA): ");
            scanf("%10s", p.validade);
            getchar();
            break;
        case 5:
            printf("Nova funcionalidade: ");
            fgets(p.funcionalidade, MAX_FUNCIONALIDADE, stdin);
            p.funcionalidade[strcspn(p.funcionalidade, "\n")] = '\0';
            break;
        case 6:
            printf("Nova categoria (medicamento/higiene/cosmético/suplemento): ");
            fgets(p.categoria, 20, stdin);
            p.categoria[strcspn(p.categoria, "\n")] = '\0';
            break;
        case 7:
              reconstruirIndice(arvore);
              printf("Índice reconstruído com sucesso!\n");
             system("pause");
             break;
        
    }
    atualizarProdutoNoArquivo(offset, &p);
    printf("\nProduto atualizado com sucesso!\n");
    printf("\nPressione qualquer tecla para continuar...");
    getchar();
}
void removerProduto(BTree *arvore) {
    system("cls");
    printf("-------------------------------------------\n");
    printf("            REMOÇÃO DE PRODUTO             \n");
    printf("-------------------------------------------\n\n");
    printf("Digite o código do produto a ser removido (14 dígitos) ou '0' para voltar: ");
    
    char codigo[15];
    scanf("%14s", codigo);
    getchar();

    if (strcmp(codigo, "0") == 0 || strcmp(codigo, "voltar") == 0) {
        printf("Operação cancelada.\n");
        return;
    }

    // Abrir os arquivos
    FILE *arquivoOriginal = fopen(ARQUIVO_DADOS, "r");
    FILE *arquivoTemp = fopen("temp.txt", "w");

    if (!arquivoOriginal || !arquivoTemp) {
        printf("Erro ao abrir arquivos!\n");
        if (arquivoOriginal) fclose(arquivoOriginal);
        if (arquivoTemp) fclose(arquivoTemp);
        return;
    }

    char linha[TAM_REGISTRO];
    int offsetAtual = 0;
    int encontrado = 0;

    while (fgets(linha, sizeof(linha), arquivoOriginal)) {
        char codigoAtual[15];
        sscanf(linha, "%14[^|]", codigoAtual);

        if (strcmp(codigoAtual, codigo) == 0) {
            encontrado = 1;

            Produto p;
            sscanf(linha, "%14[^|]|%50[^|]|%30[^|]|%f|%10[^|]|%50[^|]|%15[^\n]", 
                   p.codigo, p.nome, p.fabricante, &p.preco, p.validade, p.funcionalidade, p.categoria);
            strcpy(p.nome, "REMOVIDO");

            // Escreve no arquivo original a marcação como REMOVIDO
            FILE *f_marcar = fopen(ARQUIVO_DADOS, "r+");
            if (f_marcar) {
                fseek(f_marcar, offsetAtual, SEEK_SET);
                fprintf(f_marcar, "%s|%s|%s|%.2f|%s|%s|%s\n",
                        p.codigo, p.nome, p.fabricante, p.preco, p.validade, p.funcionalidade, p.categoria);
                fclose(f_marcar);
            }

            // Não copia para o arquivo temporário = REMOÇÃO FÍSICA
        } else {
            fputs(linha, arquivoTemp);
        }

        offsetAtual = ftell(arquivoOriginal);
    }

    fclose(arquivoOriginal);
    fclose(arquivoTemp);

    if (!encontrado) {
        remove("temp.txt");
        printf("\nProduto não encontrado!\n");
        printf("\nPressione qualquer tecla para continuar...");
        getchar();
        return;
    }

    printf("\nProduto encontrado e marcado como REMOVIDO.\n");
    printf("Deseja concluir a remoção física do sistema? (S/N): ");
    char confirmar;
    scanf("%c", &confirmar);
    getchar();

    if (toupper(confirmar) != 'S') {
        remove("temp.txt");
        printf("Remoção física cancelada. Apenas marcado como REMOVIDO.\n");
        return;
    }

    // Substituir o arquivo original pelo temporário
    remove(ARQUIVO_DADOS);
    rename("temp.txt", ARQUIVO_DADOS);

    // Recriar árvore B e índice
    liberarArvore(arvore);
    arvore = criarArvore();

    FILE *f = fopen(ARQUIVO_DADOS, "r");
    if (f) {
        int offset = 0;
        while (fgets(linha, sizeof(linha), f)) {
            Produto p;
            if (sscanf(linha, "%14[^|]|%50[^|]|%30[^|]|%f|%10[^|]|%50[^|]|%15[^\n]", 
                p.codigo, p.nome, p.fabricante, &p.preco, p.validade, p.funcionalidade, p.categoria) == 7) {
                inserir(arvore, p.codigo, offset);
            }
            offset = ftell(f);
        }
        fclose(f);
    }

    reconstruirIndice(arvore);

    printf("\nProduto removido fisicamente e marcado como REMOVIDO no histórico!\n");
    printf("\nPressione qualquer tecla para continuar...");
    getchar();
}

void listarTodosProdutos(BTree *arvore) {
    system("cls");
    printf("+--------------------------------------------+\n");
    printf("|         LISTA COMPLETA DE PRODUTOS         |\n");
    printf("+--------------------------------------------+\n\n");

    int contador = 0;
    listarEmOrdem(arvore->raiz, &contador);

    printf("+--------------------------------------------+\n");
    printf("| TOTAL DE PRODUTOS CADASTRADOS: %-12d |\n", contador);
    printf("+--------------------------------------------+\n");

    system("pause");
}


void liberarNo(BTreeNode *no) {
    if (!no) return;
    if (!no->folha) {
        for (int i = 0; i <= no->n; i++) {
            liberarNo(no->filhos[i]);
        }
    }
    free(no);
}

void liberarArvore(BTree *arvore) {
    if (arvore) {
        liberarNo(arvore->raiz);
        free(arvore);
    }
}
//main
int main() {
    setlocale(LC_ALL, "Portuguese");
    
      BTree *arvore = criarArvore();
    bool arquivoNovo = true;
    FILE *f = fopen(ARQUIVO_DADOS, "r");
    if (f) {
        fseek(f, 0, SEEK_END);
        if (ftell(f) > 0) {
            arquivoNovo = false;
        }
        fclose(f);
}
   
    FILE *f_indice = fopen(ARQUIVO_INDICE, "rb");
    if (!f_indice) {
        reconstruirIndice(arvore);
    } else {
        fclose(f_indice);
    }
    
          
     if (arquivoNovo) {
    Produto produtosExemplo[] = {
        {"12345678901234", "Dipirona", "Neo Quimica", 5.50, "12/2025", "Analgésico e antitérmico para dor e febre", "medicamento"},
        {"23456789012345", "Ibuprofeno", "EMS", 12.70, "05/2025", "Anti-inflamatório e analgésico para dor", "medicamento"},
        {"34567890123456", "Paracetamol", "Medley", 8.90, "02/2026", "Analgésico e antitérmico para dor leve", "medicamento"},
        {"45678901234567", "Omeprazol", "Eurofarma", 19.90, "12/2025", "Inibidor de bomba de prótons para azia", "medicamento"},
        {"56789012345678", "Amoxicilina", "Aché", 25.50, "08/2024", "Antibiótico para infecções bacterianas", "medicamento"},
        {"67890123456789", "Loratadina", "Medley", 14.30, "11/2024", "Anti-histamínico para alergias", "medicamento"},
        {"78901234567890", "Sinvastatina", "EMS", 32.90, "03/2026", "Redutor de colesterol", "medicamento"},
        {"89012345678901", "Losartana", "Eurofarma", 28.70, "07/2025", "Anti-hipertensivo", "medicamento"},
        {"90123456789012", "Metformina", "Aché", 18.20, "09/2025", "Hipoglicemiante para diabetes", "medicamento"},
        {"01234567890123", "Diazepam", "Neo Quimica", 22.40, "04/2024", "relaxante muscular", "medicamento"},
        
        {"11223344556677", "Sabonete Líquido", "Palmolive", 6.90, "10/2024", "Hidratante para pele sensível", "higiene"},
        {"22334455667788", "Shampoo Anticaspa", "Seda", 18.90, "08/2024", "Controle de caspa e coceira", "higiene"},
        {"33445566778899", "Creme Dental", "Colgate", 4.50, "12/2025", "Proteção contra cáries", "higiene"},
        {"44556677889900", "Desodorante Roll-on", "Rexona", 14.90, "09/2024", "Proteção 48h contra odor", "higiene"},
        {"55667788990011", "Absorvente Intimus", "Johnson", 12.30, "05/2025", "Proteção diária feminina", "higiene"},
       
        {"66778899001122", "Protetor Solar FPS 50", "Nivea", 42.50, "11/2024", "Proteção contra raios UVA/UVB", "cosmético"},
        {"77889900112233", "Hidratante Facial", "Natura", 27.50, "07/2025", "Hidratação profunda 24h", "cosmético"},
        {"88990011223344", "Batom Hidratante", "Avon", 15.90, "02/2025", "Hidratação e cor", "cosmético"},
        {"99001122334455", "Máscara de Cílios", "Maybelline", 29.90, "10/2024", "Volume e alongamento", "cosmético"},
        {"00112233445566", "Perfume Masculino", "O Boticário", 89.90, "06/2026", "Fragrância amadeirada", "cosmético"},
        
        {"10293847561234", "Vitamina C", "Nutraway", 29.90, "03/2025", "Suplemento vitamínico antioxidante", "suplemento"},
        {"29384756123456", "Ômega 3", "NutriGold", 45.70, "08/2025", "Suplemento de ácidos graxos", "suplemento"},
        {"38475612345678", "Whey Protein", "Growth", 79.90, "12/2024", "Proteína para musculação", "suplemento"},
        {"47561234567890", "Multivitamínico", "Centrum", 52.30, "05/2025", "Complexo de vitaminas e minerais", "suplemento"},
        {"56123456789012", "Colágeno", "Sanavita", 38.50, "09/2024", "Suplemento para pele e articulações", "suplemento"}
    };
    
    
    
    int numProdutos = sizeof(produtosExemplo)/sizeof(produtosExemplo[0]);
        for (int i = 0; i < numProdutos; i++) {
            int offset = salvarProduto(&produtosExemplo[i]);
            if (offset != -1) {
                inserir(arvore, produtosExemplo[i].codigo, offset);
            }
        }
    } else {
        FILE *f = fopen(ARQUIVO_DADOS, "r");
        if (f) {
            char linha[TAM_REGISTRO];
            int offset = 0;
            
            while (fgets(linha, sizeof(linha), f)) {
                Produto p;
                if (sscanf(linha, "%14[^|]|%50[^|]|%30[^|]|%f|%10[^|]|%50[^|]|%15[^\n]", 
                    p.codigo, p.nome, p.fabricante, &p.preco, p.validade, p.funcionalidade, p.categoria) == 7) {
                    
                    inserir(arvore, p.codigo, offset);
                }
                offset = ftell(f);
            }
            fclose(f);
        }
    }
    int opcao;
   do {
    exibirMenuPrincipal();
    scanf("%d", &opcao);
    getchar();

    switch(opcao) {
        case 1:
            cadastrarProduto(arvore);
            break;
        case 2:
            buscarPorCodigo(arvore);
            break;
        case 3:
            buscarPorFuncionalidade(arvore);
            break;
        case 4:
            listarTodosProdutos(arvore);
            break;
        case 5:
            editarProduto(arvore);
            break;
        case 6:
            removerProduto(arvore);
            break;
        case 7:
            reconstruirIndice(arvore);
            printf("Índice reconstruído com sucesso!\n");
            system("pause");
            break;
        case 8:
            printf("Saindo do sistema...\n");
            break;
        default:
            printf("Opção inválida! Tente novamente.\n");
            system("pause");
    }
} while (opcao != 8);
    liberarArvore(arvore);
    return 0;
}
