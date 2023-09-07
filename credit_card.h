#ifndef CREDIT_CARD_H
#define CREDIT_CARD_H

#include <stdbool.h>

#define NUM_THREADS 4
#define MAX_CARD_LENGTH 16
#define RESPONSE_SIZE 4096

typedef struct
{
    char numero[MAX_CARD_LENGTH + 1];
    char resultado[RESPONSE_SIZE];
} CreditCardInfo;

int lerCartoesDeArquivo(const char *nome_arquivo, CreditCardInfo **credit_cards);
void *processarCartoes(void *arg);
void salvarResultadosEmArquivo(const char *nome_arquivo, CreditCardInfo *credit_cards, int num_credit_cards);

#endif
