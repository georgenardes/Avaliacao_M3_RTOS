/*
Arquivo: monitoramento_esteiras.c
Autor:  Diogo Marchi
        George Borba
Função do arquivo: 
        Cria threads para contagem, pesagem e exibição de
        produtos em 3 esteiras.
Criado em 02 de dezembro de 2020
Modificado em 02 de dezembro de 2020

*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "nvs_flash.h"

// NUM máximo de produto
#define NUM_MAX_PROD 100

 // periodo entre passagem de produtos nas esteiras 
#define TEMPO_EST_1 1000
#define TEMPO_EST_2 500
#define TEMPO_EST_3 100

// periodo entre atualizações do display
#define TEMPO_ATUALIZACAO 2000

// peso dos produtos nas esteiras
#define PESO_EST_1 5.0
#define PESO_EST_2 2.0
#define PESO_EST_3 0.5

// variavel do semaforo
SemaphoreHandle_t mutual_exclusion_mutex;

// handler de task para suspender
TaskHandle_t handler_display;

// handler das esteiras 
TaskHandle_t handler_est1;
TaskHandle_t handler_est2;
TaskHandle_t handler_est3;

static int num_produtos = 0;
static float pesos[NUM_MAX_PROD] = {0x0};


void suspender_tasks ()
{
    // display
    vTaskSuspend(handler_display);
    // inserir tasks para suspender aqui
}

void resumir_tasks ()
{
    // display
    vTaskResume(handler_display);
    // inserir tasks para suspender aqui
}


void soma_pesos()
{
    // suspender as  tasks
    suspender_tasks();

    float peso_total = 0;
    
    // TODO: criar threads para realizar a soma paralelamente
    // dividir entre os dois cores 50% pra cada
    // mutex nas tasks para poder somar corretamente
    for(int i = 0; i < NUM_MAX_PROD; i++)
    {
        peso_total += pesos[i];
    }

    printf("O peso total dos prodos foi %f \n", peso_total);        

    // resumir as tasks
    resumir_tasks();
	
}

void soma_produto(float peso)
{
    // mutex (semaforo)
    xSemaphoreTake(mutual_exclusion_mutex, portMAX_DELAY);

    pesos[num_produtos] = peso;
    num_produtos++;

    if (num_produtos >= NUM_MAX_PROD)
    {
        // cria task de somar peso
        soma_pesos(); 
        num_produtos = 0;
    }

    // end mutex
    xSemaphoreGive(mutual_exclusion_mutex);
}

void esteira_1(void *pvParameter)
{    
    TickType_t xLastWakeTime;

    // tempo atual
    xLastWakeTime = xTaskGetTickCount ();

	while(1)
	{
        // aguardar produto
        vTaskDelayUntil(&xLastWakeTime, TEMPO_EST_1 / portTICK_RATE_MS);

	    // somar produto
        soma_produto(PESO_EST_1);
	}
}

void esteira_2(void *pvParameter)
{
    TickType_t xLastWakeTime;

    // tempo atual
    xLastWakeTime = xTaskGetTickCount ();

    while(1)
	{
         // aguardar produto
        vTaskDelayUntil(&xLastWakeTime, TEMPO_EST_2 / portTICK_RATE_MS);

	    // somar produto
        soma_produto(PESO_EST_2);
	}
}

void esteira_3(void *pvParameter)
{
    TickType_t xLastWakeTime;

    // tempo atual
    xLastWakeTime = xTaskGetTickCount ();

    while(1)
	{
         // aguardar produto
        vTaskDelayUntil(&xLastWakeTime, TEMPO_EST_3 / portTICK_RATE_MS);

	    // somar produto
        soma_produto(PESO_EST_3);
	}
}


 
void display(void *pvParameter)
{
    
    while(1) 
    {
        vTaskDelay(TEMPO_ATUALIZACAO / portTICK_RATE_MS);
        printf("Quantidade produtos %d\n", num_produtos);
    }
}
 
 
void app_main()
{
    nvs_flash_init();

    // inicializa semáforo
    mutual_exclusion_mutex = xSemaphoreCreateMutex();

    if( mutual_exclusion_mutex == NULL ){
        printf("Erro na criação do mutex\n");
        exit(0);
    }

    xTaskCreate(&esteira_1, "esteira_1", 2048, NULL, 3, &handler_est1);
    configASSERT(handler_est1);

    xTaskCreate(&esteira_2, "esteira_2", 2048, NULL, 3, &handler_est2);
    configASSERT(handler_est2);

    xTaskCreate(&esteira_3, "esteira_3", 2048, NULL, 3, &handler_est3);
    configASSERT(handler_est2);
    
    xTaskCreate(&display, "display", 2048, NULL, 1, &handler_display);
    configASSERT(handler_display);
    

    // criar task para monitoramento do botão de parada

}