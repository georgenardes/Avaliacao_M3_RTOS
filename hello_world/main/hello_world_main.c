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
#define NUM_MAX_PROD 1500

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

// handle de task para suspender
TaskHandle_t handler_display;

static int num_produtos = 0;
static float pesos[1500] = {0x0};

void soma_pesos()
{
    // suspender as demais tasks
    vTaskSuspend(handler_display);

    float peso_total = 0;
    
    // criar threads para realizar a soma paralelamente
    for(int i = 0; i < 1500; i++)
    {
        peso_total += pesos[i];
    }

    printf("O peso total dos prodos foi %f \n", peso_total);    

    vTaskDelay(10000 / portTICK_RATE_MS);

    // retornar as demais tasks
    vTaskResume(handler_display);
	
}

void soma_produto(float peso)
{
    // mutex (semaforo)
    xSemaphoreTake(mutual_exclusion_mutex, portMAX_DELAY);

    pesos[num_produtos] = peso;
    num_produtos++;

    if (num_produtos >= 1500)
    {
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
    
    TickType_t xLastWakeTime;

    // tempo atual
    xLastWakeTime = xTaskGetTickCount ();

    while(1) 
    {
        vTaskDelayUntil(&xLastWakeTime, TEMPO_ATUALIZACAO / portTICK_RATE_MS);
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

    xTaskCreate(&esteira_1, "esteira_1", 2048, NULL, 5, NULL);
    xTaskCreate(&esteira_2, "esteira_2", 2048, NULL, 5, NULL);
    xTaskCreate(&esteira_3, "esteira_3", 2048, NULL, 5, NULL);
    xTaskCreate(&display, "display", 2048, NULL, 5, &handler_display);
    // criar task para monitoramento do botão de parada

}