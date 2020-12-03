/*
Arquivo: hello_word_and_blink_task.c
Autor: Felipe Viel
Função do arquivo: Cria uma task para printar o Hello World e uma para blink. Baseado no exempo do ESP-IDF
Criado em 17 de novembro de 2020
Modificado em 17 de novembro de 2020

*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
 
#define BLINK_GPIO 2
 
static int num_produtos = 0;
static int pesos[1500] = {0x0};

void soma_pesos(void *pvParameter)
{
    while(1)
    {
        // suspender ela mesmo e aguardar por resumir
        // suspender as outras threads
        // contar peso
        // resumir sumir outras threads
    }
}

void esteira_x(void *pvParameter)
{
 
	while(1)
	{
        // aguardar produto

	    // região mutex
        // somar produto
        // verificar total < 1500
            // resumir soma de peso
            // se autosuspender
        // fim mutex

        if(num_produtos >= 1500) num_produtos = 0;
        num_produtos++;
	    vTaskDelay(1000 / portTICK_RATE_MS);
	}
}
 
void display(void *pvParameter)
{
    
    TickType_t xLastWakeTime;

    // Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount ();

    const TickType_t xTempo = 2000;

    while(1) 
    {
        vTaskDelayUntil(&xLastWakeTime, xTempo / portTICK_RATE_MS);
        printf("Quantidade produtos %d\n", num_produtos);
    }
}
 
 
void app_main()
{

    nvs_flash_init();
    xTaskCreate(&esteira_x, "esteira_1", 2048, NULL, 5, NULL);
    xTaskCreate(&esteira_x, "esteira_2", 2048, NULL, 5, NULL);
    xTaskCreate(&esteira_x, "esteira_3", 2048, NULL, 5, NULL);
    xTaskCreate(&display, "display", 2048, NULL, 5, NULL);
}