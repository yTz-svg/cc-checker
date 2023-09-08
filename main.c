#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <sys/stat.h>

#define API_KEY "seu_api_key" // Substitua pelo seu token de acesso real
#define NUM_THREADS 4
#define MAX_CARDS_PER_THREAD 10
#define MAX_CARD_LENGTH 16
#define RESPONSE_SIZE 4096
#define AES_KEY_SIZE 256
#define AES_BLOCK_SIZE 16
#define ENCRYPTED_FOLDER "criptografia"
#define RESULTS_FOLDER "results"
#define PROXY_CONFIG_FILE "proxy/config.txt"
#define MAX_PROXY_URL_LENGTH 256

typedef struct
{
    char numero[MAX_CARD_LENGTH + 1];
    char resultado[RESPONSE_SIZE];
} CreditCardInfo;

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t total_size = size * nmemb;
    memcpy(userp, contents, total_size);
    return total_size;
}

int fazerSolicitacaoAPI(const char *numero_cartao, char *resposta, const char *proxy_url)
{
    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if (!curl)
    {
        fprintf(stderr, "Erro ao inicializar a biblioteca libcurl.\n");
        return -1;
    }

    const char *url = "https://www.bincodes.com/api-creditcard-checker/";

    curl_easy_setopt(curl, CURLOPT_URL, url);

    char post_data[256];
    snprintf(post_data, sizeof(post_data), "cc=%s&apiKey=%s", numero_cartao, API_KEY);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    char response_data[RESPONSE_SIZE];
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_data);

    if (proxy_url != NULL)
    {
        curl_easy_setopt(curl, CURLOPT_PROXY, proxy_url);
    }

    res = curl_easy_perform(curl);

    if (res == CURLE_OK)
    {
        strcpy(resposta, response_data);
    }
    else
    {
        fprintf(stderr, "Erro ao fazer a solicitação: %s\n", curl_easy_strerror(res));
        return -2;
    }

    curl_easy_cleanup(curl);

    curl_global_cleanup();

    return 0;
}

void generateAESKey(unsigned char *aes_key)
{
    if (!RAND_bytes(aes_key, AES_KEY_SIZE / 8))
    {
        fprintf(stderr, "Erro ao gerar a chave AES.\n");
        exit(EXIT_FAILURE);
    }
}

void encryptAES(const unsigned char *data, size_t data_length, const unsigned char *aes_key, unsigned char *encrypted_data)
{
    AES_KEY key;
    if (AES_set_encrypt_key(aes_key, AES_KEY_SIZE, &key) < 0)
    {
        fprintf(stderr, "Erro ao definir a chave de criptografia AES.\n");
        exit(EXIT_FAILURE);
    }

    AES_encrypt(data, encrypted_data, &key);
}

int lerProxyConfig(const char *config_file, char *proxy_url)
{
    FILE *arquivo = fopen(config_file, "r");
    if (!arquivo)
    {
        fprintf(stderr, "Erro ao abrir o arquivo de configuração do proxy: %s\n", config_file);
        return -1;
    }

    if (fgets(proxy_url, MAX_PROXY_URL_LENGTH, arquivo) == NULL)
    {
        fprintf(stderr, "Erro ao ler o URL do proxy do arquivo de configuração\n");
        fclose(arquivo);
        return -2;
    }

    size_t len = strlen(proxy_url);
    if (len > 0 && proxy_url[len - 1] == '\n')
    {
        proxy_url[len - 1] = '\0';
    }

    fclose(arquivo);
    return 0;
}

int lerCartoesDeArquivo(const char *nome_arquivo, CreditCardInfo **credit_cards)
{
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (!arquivo)
    {
        fprintf(stderr, "Erro ao abrir o arquivo %s\n", nome_arquivo);
        exit(EXIT_FAILURE);
    }

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
        (*credit_cards)[i].numero[strlen((*credit_cards)[i].numero) - 1] = '\0';
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

int main()
{
    char proxy_url[MAX_PROXY_URL_LENGTH] = "";

    int proxy_result = lerProxyConfig(PROXY_CONFIG_FILE, proxy_url);

    if (proxy_result == 0)
    {
        printf("URL do proxy: %s\n", proxy_url);
    }
    else
    {
        fprintf(stderr, "Erro ao ler a configuração do proxy. Código de erro: %d\n", proxy_result);
        return -1;
    }

    CreditCardInfo *credit_cards = NULL;
    int num_credit_cards = lerCartoesDeArquivo("resultados/cartoes.txt", &credit_cards);

    if (num_credit_cards == 0)
    {
        fprintf(stderr, "Nenhum cartão de crédito encontrado no arquivo.\n");
        return -2;
    }

    pthread_t threads[NUM_THREADS];
    int i;
    int chunk_size = num_credit_cards / NUM_THREADS;
    for (i = 0; i < NUM_THREADS; i++)
    {
        int start_index = i * chunk_size;
        int end_index = (i == NUM_THREADS - 1) ? num_credit_cards : start_index + chunk_size;
        ThreadData *thread_data = (ThreadData *)malloc(sizeof(ThreadData));
        thread_data->start_index = start_index;
        thread_data->end_index = end_index;
        thread_data->credit_cards = credit_cards;

        int thread_create_result = pthread_create(&threads[i], NULL, processarCartoes, (void *)thread_data);

        if (thread_create_result != 0)
        {
            fprintf(stderr, "Erro ao criar a thread %d. Código de erro: %d\n", i, thread_create_result);
            return -3;
        }
    }

    for (i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    salvarResultadosEmArquivo("results/resultado.txt", credit_cards, num_credit_cards);
    salvarResultadosEmArquivo("criptografia/resultado_criptografado.txt", credit_cards, num_credit_cards);

    free(credit_cards);

    return 0;
}
