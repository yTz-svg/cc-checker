#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <sys/stat.h>

#define API_KEY "sua_chave_api" // Substitua pela sua chave de acesso real
#define NUM_THREADS 4
#define MAX_CARDS_PER_THREAD 10
#define MAX_CARD_LENGTH 16
#define RESPONSE_SIZE 4096
#define AES_KEY_SIZE 256
#define AES_BLOCK_SIZE 16
#define ENCRYPTED_FOLDER "criptografia"
#define RESULTS_FOLDER "resultados"
#define PROXY_CONFIG_FILE "proxy/config.txt"
#define MAX_PROXY_URL_LENGTH 256

typedef struct
{
    char numero[MAX_CARD_LENGTH + 1];
    char resultado[RESPONSE_SIZE];
} InformacoesCartaoCredito;

size_t escrever_callback(void *dados, size_t tamanho, size_t nmemb, void *usuario)
{
    size_t tamanho_total = tamanho * nmemb;
    memcpy(usuario, dados, tamanho_total);
    return tamanho_total;
}

int fazerSolicitacaoAPI(const char *numero_cartao, char *resposta, const char *proxy_url)
{
    CURL *curl;
    CURLcode resultado;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if (!curl)
    {
        fprintf(stderr, "Erro ao inicializar a biblioteca libcurl.\n");
        return -1;
    }

    const char *url = "https://www.bincodes.com/api-creditcard-checker/";

    curl_easy_setopt(curl, CURLOPT_URL, url);

    char dados_postagem[256];
    snprintf(dados_postagem, sizeof(dados_postagem), "cc=%s&apiKey=%s", numero_cartao, API_KEY);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, dados_postagem);

    struct curl_slist *cabecalhos = NULL;
    cabecalhos = curl_slist_append(cabecalhos, "Content-Type: application/x-www-form-urlencoded");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, cabecalhos);

    char dados_resposta[RESPONSE_SIZE];
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, escrever_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, dados_resposta);

    if (proxy_url != NULL)
    {
        curl_easy_setopt(curl, CURLOPT_PROXY, proxy_url);
    }

    resultado = curl_easy_perform(curl);

    if (resultado == CURLE_OK)
    {
        strcpy(resposta, dados_resposta);
    }
    else
    {
        fprintf(stderr, "Erro ao fazer a solicitação: %s\n", curl_easy_strerror(resultado));
        return -2;
    }

    curl_easy_cleanup(curl);

    curl_global_cleanup();

    return 0;
}

void gerarChaveAES(unsigned char *chave_aes)
{
    if (!RAND_bytes(chave_aes, AES_KEY_SIZE / 8))
    {
        fprintf(stderr, "Erro ao gerar a chave AES.\n");
        exit(EXIT_FAILURE);
    }
}

void criptografarAES(const unsigned char *dados, size_t tamanho_dados, const unsigned char *chave_aes, unsigned char *dados_criptografados)
{
    AES_KEY chave;
    if (AES_set_encrypt_key(chave_aes, AES_KEY_SIZE, &chave) < 0)
    {
        fprintf(stderr, "Erro ao definir a chave de criptografia AES.\n");
        exit(EXIT_FAILURE);
    }

    AES_encrypt(dados, dados_criptografados, &chave);
}

int lerConfiguracaoProxy(const char *arquivo_configuracao, char *url_proxy)
{
    FILE *arquivo = fopen(arquivo_configuracao, "r");
    if (!arquivo)
    {
        fprintf(stderr, "Erro ao abrir o arquivo de configuração do proxy: %s\n", arquivo_configuracao);
        return -1;
    }

    if (fgets(url_proxy, MAX_PROXY_URL_LENGTH, arquivo) == NULL)
    {
        fprintf(stderr, "Erro ao ler o URL do proxy do arquivo de configuração\n");
        fclose(arquivo);
        return -2;
    }

    size_t len = strlen(url_proxy);
    if (len > 0 && url_proxy[len - 1] == '\n')
    {
        url_proxy[len - 1] = '\0';
    }

    fclose(arquivo);
    return 0;
}

int lerCartoesDeArquivo(const char *nome_arquivo, InformacoesCartaoCredito **cartoes_credito)
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

    *cartoes_credito = (InformacoesCartaoCredito *)malloc(num_linhas * sizeof(InformacoesCartaoCredito));
    if (!(*cartoes_credito))
    {
        fprintf(stderr, "Erro ao alocar memória para os cartões de crédito\n");
        exit(EXIT_FAILURE);
    }

    int i = 0;
    while (fgets((*cartoes_credito)[i].numero, MAX_CARD_LENGTH + 1, arquivo))
    {
        (*cartoes_credito)[i].numero[strlen((*cartoes_credito)[i].numero) - 1] = '\0';
        i++;
    }

    fclose(arquivo);
    return num_linhas;
}

void salvarResultadosEmArquivo(const char *nome_arquivo, InformacoesCartaoCredito *cartoes_credito, int num_cartoes_credito)
{
    FILE *arquivo = fopen(nome_arquivo, "w");
    if (!arquivo)
    {
        fprintf(stderr, "Erro ao abrir o arquivo para escrita: %s\n", nome_arquivo);
        return;
    }

    for (int i = 0; i < num_cartoes_credito; i++)
    {
        fprintf(arquivo, "%s\n", cartoes_credito[i].resultado);
    }

    fclose(arquivo);
}

void *processarCartoes(void *argumento)
{
    ThreadData *dados_thread = (ThreadData *)argumento;
    for (int i = dados_thread->start_index; i < dados_thread->end_index; i++)
    {
        char resposta[RESPONSE_SIZE];
        int resultado_api = fazerSolicitacaoAPI(dados_thread->credit_cards[i].numero, resposta, NULL);

        if (resultado_api == 0)
        {

            strncpy(dados_thread->credit_cards[i].resultado, resposta, sizeof(dados_thread->credit_cards[i].resultado));
        }
        else
        {
            fprintf(stderr, "Erro ao verificar o cartão de crédito %s. Código de erro: %d\n", dados_thread->credit_cards[i].numero, resultado_api);
        }
    }
    pthread_exit(NULL);
}

int main()
{
    char url_proxy[MAX_PROXY_URL_LENGTH] = "";

    int resultado_proxy = lerConfiguracaoProxy(PROXY_CONFIG_FILE, url_proxy);

    if (resultado_proxy == 0)
    {
        printf("URL do proxy: %s\n", url_proxy);
    }
    else
    {
        fprintf(stderr, "Erro ao ler a configuração do proxy. Código de erro: %d\n", resultado_proxy);
        return -1;
    }

    InformacoesCartaoCredito *cartoes_credito = NULL;
    int num_cartoes_credito = lerCartoesDeArquivo("resultados/cartoes.txt", &cartoes_credito);

    if (num_cartoes_credito == 0)
    {
        fprintf(stderr, "Nenhum cartão de crédito encontrado no arquivo.\n");
        return -2;
    }

    pthread_t threads[NUM_THREADS];
    int i;
    int tamanho_grupo = num_cartoes_credito / NUM_THREADS;
    for (i = 0; i < NUM_THREADS; i++)
    {
        int indice_inicial = i * tamanho_grupo;
        int indice_final = (i == NUM_THREADS - 1) ? num_cartoes_credito : indice_inicial + tamanho_grupo;
        ThreadData *dados_thread = (ThreadData *)malloc(sizeof(ThreadData));
        dados_thread->start_index = indice_inicial;
        dados_thread->end_index = indice_final;
        dados_thread->credit_cards = cartoes_credito;

        int resultado_criacao_thread = pthread_create(&threads[i], NULL, processarCartoes, (void *)dados_thread);

        if (resultado_criacao_thread != 0)
        {
            fprintf(stderr, "Erro ao criar a thread %d. Código de erro: %d\n", i, resultado_criacao_thread);
            return -3;
        }
    }

    for (i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    salvarResultadosEmArquivo("resultados/resultado.txt", cartoes_credito, num_cartoes_credito);
    salvarResultadosEmArquivo("criptografia/resultado_criptografado.txt", cartoes_credito, num_cartoes_credito);

    free(cartoes_credito);

    return 0;
}

