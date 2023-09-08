## Projeto de Verificação de Cartão de Crédito

Este é um projeto que verifica a validade de números de cartão de crédito usando a API de verificação de cartão de crédito do [bincodes.com](https://www.bincodes.com/api-creditcard-checker/). O projeto também inclui recursos de criptografia para proteger os resultados.

## Como Funciona

1. **API de Verificação de Cartão de Crédito:** O projeto faz solicitações à API do [bincodes.com](https://www.bincodes.com/api-creditcard-checker/) para verificar se um número de cartão de crédito é válido.

2. **Criptografia dos Resultados:** Após a verificação, os resultados são criptografados usando AES para proteger os dados sensíveis.

3. **Múltiplas Threads:** O projeto usa várias threads para processar vários números de cartão de crédito simultaneamente, tornando-o eficiente.

4. **Suporte a Proxy:** O projeto suporta o uso de proxies para fazer solicitações à API de verificação de cartão de crédito de forma anônima.

## Como Usar

Siga estas etapas para usar o projeto:

1. Clone este repositório:

   ```bash
   git clone https://github.com/yTz-svg/cc-checker.git

2. Compile o código C:

   ```bash
   gcc -o main main.c -lcurl -lssl -lcrypto -lpthread
3.  Verefuque o arquivo cartoes.txt com os números dos cartões de crédito a serem verificados, um por linha.

4.  Configuração do Proxy (Opcional): Se você deseja usar proxies para fazer solicitações à API, siga estas etapas:

    - Crie uma pasta chamada proxy no diretório do projeto.

    - Dentro da pasta proxy, crie um arquivo chamado config.txt.

    - No arquivo config.txt, adicione a URL do seu proxy, um por linha. Exemplo

    ```bash
    http://proxy1.example.com
    http://proxy2.example.com
    
5. Execute o programa:

   ```bash
   ./main
6. Os resultados serão salvos na pasta results (resultados não criptografados) e na pasta criptografia (resultados criptografados).

## Configuração da API

Para usar a API de verificação de cartão de crédito, você precisará obter uma chave de API do bincodes.com. Substitua "seu_api_key" no código pelo seu token de acesso real.

## Autor

Este projeto foi criado por ytz para a empresa [ZKTeco Brasil](https://www.zkteco.com.br/). O objetivo é fornecer conteúdo educativo e demonstrar a integração com a API de verificação de cartão de crédito. 
