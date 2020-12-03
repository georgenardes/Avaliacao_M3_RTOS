# Sistema para contagem e pesagem de produtos em três esteiras.

## Requisitos

### Não funcionais
1. Usar FreeRTOS versão para ESP32
1. Usar placa ESP32
1. Usar ferramenta ESP-IDF (SDK-IDF)
1. Usar biblioteca com funções de temporizador do próprio ESP32 para medir o tempo.
1. Usar um mecanismo de proteção para a seção crítica.

### Funcionais
2. O sistema deve realizar uma simulação de detecção de produtos em 3 esteiras respeitando as seguintes restrições:
   2. Esteira 1: A cada 1 segundo deve passar um produto de 5 Kg.
   2. Esteira 2: A cada 0.5 segundo deve passar um produto de 2 Kg.
   2. Esteira 3: A cada 0.1 segundo deve passar um produto de 0.5 Kg.
2. O sistema deve possuir um display para exibir a contagem dos produtos.
2. O sistema deve atualizar o display com o numero atual de produtos a cada 2 segundos.
2. O sistema deve inserir o peso de cada produto em um vetor com 1500 posições.
2. O sistema deve interromper a contagem ao atingir 1500 produtos.
2. O sistema deve realizar a soma dos pesos dos produtos ao ser atingido 1500 produtos.
2. O sistema deve continuar a contagem após ter realizado a soma dos pesos.
2. O sistema deve permitir que o usuário interrompa a contagem por meio de um botão do ESP32.


## Questões a serem respondidas 

1. Quanto tempo o sistema leva para somar o vetor de pesos?
1. Quanto tempo o sistema leva para identificar um produto na esteira? (Perguntar ao professor se isso deve ser feito)
1. Quanto tempo o sistema leva para contar um produto identificado? 
1. Quanto tempo o sistema leva para atualizar o display?


## Ponto extra

O uso correto de Queue ou Events Group dará aos alunos uma bonificação na média.

## Projeto

