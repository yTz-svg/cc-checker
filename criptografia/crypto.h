#ifndef CRYPTO_H
#define CRYPTO_H

#define AES_KEY_SIZE 256

void generateAESKey(unsigned char *aes_key);
void encryptAES(const unsigned char *data, size_t data_length, const unsigned char *aes_key, unsigned char *encrypted_data);
void salvarResultadosCriptografados(const char *nome_arquivo, CreditCardInfo *credit_cards, int num_credit_cards, const unsigned char *aes_key);

#endif
