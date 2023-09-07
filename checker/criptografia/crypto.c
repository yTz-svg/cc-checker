#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include "credit_card.h"
#include "crypto.h"

void generateAESKey(unsigned char *aes_key)
{
    if (!RAND_bytes(aes_key, AES_KEY_SIZE / 8))
    {
        fprintf(stderr, "Erro ao gerar a chave AES.\n");
        exit(EXIT_FAILURE);
    }
}

void encryptAES(const unsigned char *data, size_t data_length, const unsigned
