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
#include "driver/touch_pad.h"
#include "esp_log.h"

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

// Valores do touch
#define TOUCH_PAD_NO_CHANGE   (-1)
#define TOUCH_THRESH_NO_USE   (0)
#define TOUCH_FILTER_MODE_EN  (0)
#define TOUCHPAD_FILTER_TOUCH_PERIOD (10)

// variavel do semaforo
SemaphoreHandle_t mutual_exclusion_mutex;

// handler de task para suspender
TaskHandle_t handler_display;

// handler das esteiras 
TaskHandle_t handler_est1;
TaskHandle_t handler_est2;
TaskHandle_t handler_est3;
TaskHandle_t handler_touch;
TaskHandle_t handler_task1;
TaskHandle_t handler_task2;

static int num_produtos = 0;
static float pesos[NUM_MAX_PROD] = {0x0};
float peso_total = 0;

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

void task1(int valor){

    int max = 0, i = 0;

    if(valor == 1){
        i = 0;
        max = 50;
    }else{
        i = 50;
        max = 100;
    }
    for(int j = i; j < max; j ++ ){
        // start semaphore
        xSemaphoreTake(mutual_exclusion_mutex, portMAX_DELAY);
        printf("peso[%d] = %f\n", j, pesos[j]);
        printf("peso total = %f\n", peso_total);
        peso_total += pesos[j];
        // end mutex
        xSemaphoreGive(mutual_exclusion_mutex);
    }
}

void soma_pesos()
{
    // suspender as  tasks
    suspender_tasks();
    // TODO: criar threads para realizar a soma paralelamente
    // dividir entre os dois cores 50% pra cada
    // mutex nas tasks para poder somar corretamente
  

    xTaskCreatePinnedToCore(task1, "task_1", 2048, (int*)1, 3, &handler_task1, 0);
    configASSERT(handler_task1);
    xTaskCreatePinnedToCore(task1, "task_2", 2048, (int*)2, 3, &handler_task2, 1);
    configASSERT(handler_task2);
   
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
 

// configura o touch
static void tp_example_touch_pad_init(void)
{
    for (int i = 0;i< TOUCH_PAD_MAX;i++) {
        touch_pad_config(i, TOUCH_THRESH_NO_USE);
    }
}
 

// função que le os valores dos pinos que acionam o touch
static void tp_example_read_task(void *pvParameter)
{
    uint16_t touch_value;
    uint16_t touch_filter_value;
#if TOUCH_FILTER_MODE_EN
    printf("Touch Sensor filter mode read, the output format is: \nTouchpad num:[raw data, filtered data]\n\n");
#else
    printf("Touch Sensor normal mode read, the output format is: \nTouchpad num:[raw data]\n\n");
#endif
    while (1) {

#if TOUCH_FILTER_MODE_EN
            // If open the filter mode, please use this API to get the touch pad count.
            touch_pad_read_raw_data(i, &touch_value);
            touch_pad_read_filtered(i, &touch_filter_value);
            printf("T%d:[%4d,%4d] ", i, touch_value, touch_filter_value);
#else
            touch_pad_read(0, &touch_value);
            if(touch_value < 1000){
                printf("Desligando\n");
                printf("Reinicie para começar o programa novamente\n");
  
                vTaskDelete(handler_est1);
                vTaskDelete(handler_est2);
                vTaskDelete(handler_est3);
                vTaskDelete(handler_display);
                vTaskDelete(handler_touch);
            }
#endif
            if(touch_value < (uint16_t)100){
                printf("\nSistema de Emergência Acinado");
                vTaskDelay(2000/portTICK_PERIOD_MS);
            }

        printf("\n");
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
} 
 
void app_main()
{
    nvs_flash_init();

    // inicializa semáforo
    mutual_exclusion_mutex = xSemaphoreCreateMutex();

    // Inicializa o touch
    touch_pad_init();

    if( mutual_exclusion_mutex == NULL ){
        printf("Erro na criação do mutex\n");
        exit(0);
    }

    // Configuração touch
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    tp_example_touch_pad_init();
    #if TOUCH_FILTER_MODE_EN
        touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD);
    #endif

    // Start task to read values sensed by pads
    xTaskCreate(&tp_example_read_task, "touch_pad_read_task", 2048, NULL, 3, &handler_touch);
    configASSERT(handler_touch);

    xTaskCreate(&esteira_1, "esteira_1", 2048, NULL, 3, &handler_est1);
    configASSERT(handler_est1);

    xTaskCreate(&esteira_2, "esteira_2", 2048, NULL, 3, &handler_est2);
    configASSERT(handler_est2);

    xTaskCreate(&esteira_3, "esteira_3", 2048, NULL, 3, &handler_est3);
    configASSERT(handler_est2);
    
    xTaskCreate(&display, "display", 2048, NULL, 3, &handler_display);
    configASSERT(handler_display);
    

    // criar task para monitoramento do botão de parada

}