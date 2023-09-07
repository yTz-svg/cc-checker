#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "credit_card.h"

int lerCartoesDeArquivo(const char *nome_arquivo, CreditCardInfo **credit_cards)
{
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (!arquivo)
    {
        fprintf(stderr, "Erro ao abrir o arquivo %s\n", nome_arquivo);
        exit(EXIT_FAILURE);
    }

    // Contar linhas no arquivo
    int num_linhas = 0;
    char c;
    while ((c = fgetc(arquivo)) != EOF)
    {
        if (c == '\n')
        {
            num_linhas++;
        }
    }
    rewind(arquivo);

    *credit_cards = (CreditCardInfo *)malloc(num_linhas * sizeof(CreditCardInfo));
    if (!(*credit_cards))
    {
        fprintf(stderr, "Erro ao alocar memória para os cartões\n");
        exit(EXIT_FAILURE);
    }

    int i = 0;
    while (fgets((*credit_cards)[i].numero, MAX_CARD_LENGTH + 1, arquivo))
    {
        (*credit_cards)[i].numero[strlen((*credit_cards)[i].numero) - 1] = '\0'; // Remover o caractere de nova linha
        i++;
    }

    fclose(arquivo);
    return num_linhas;
}

void salvarResultadosEmArquivo(const char *nome_arquivo, CreditCardInfo *credit_cards, int num_credit_cards)
{
    FILE *arquivo = fopen(nome_arquivo, "w");
    if (!arquivo)
    {
        fprintf(stderr, "Erro ao abrir o arquivo para escrita: %s\n", nome_arquivo);
        return;
    }

    for (int i = 0; i < num_credit_cards; i++)
    {
        fprintf(arquivo, "%s\n", credit_cards[i].resultado);
    }

    fclose(arquivo);
}
