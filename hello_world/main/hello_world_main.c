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
#include "esp_timer.h"

// NUM máximo de produto
#define NUM_MAX_PROD 1500

// periodo milissegundos entre passagem de produtos nas esteiras 
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
SemaphoreHandle_t mutual_exclusion_mutex_soma;

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
static float peso_total = 0;

int64_t start_soma, end_soma;
double total_time;

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

void soma_paralela(int ID){

    float resultado = 0;
    int max = 0, i = 0; 

    while(1)
    {
        if (ID == 1)
        {
            i = 0;
            max = (int) NUM_MAX_PROD/2;
        } else if (ID == 2)
        {
            i = (int) NUM_MAX_PROD/2;
            max = NUM_MAX_PROD;
        } else 
        {
            printf("ID recebido errado!\n");
            vTaskDelete(NULL);
        }

        // soma as posições do valor incial ao final
        for(int j = i; j < max; j ++ )
        {                
            resultado += pesos[j];                
        }        

        // start semaphore
        xSemaphoreTake(mutual_exclusion_mutex_soma, portMAX_DELAY);
        peso_total += resultado;    // adiciona a soma total
        xSemaphoreGive(mutual_exclusion_mutex_soma);
        // end mutex

        // printf("Resultado task %d = %f\n", ID, resultado);

        // deleta task atual
        vTaskDelete(NULL);
    }    
}

void soma_pesos()
{
    // suspender as  tasks
    suspender_tasks();

    // cria threads para realizar a soma paralelamente
    // divide entre os dois cores 50% pra cada
    // colocado mutex nas tasks para poder somar corretamente
    xTaskCreatePinnedToCore(&soma_paralela, "soma_1", 2048, (int*)1, 4, &handler_task1, APP_CPU_NUM);
    configASSERT(handler_task1);
    xTaskCreatePinnedToCore(&soma_paralela, "soma_2", 2048, (int*)2, 4, &handler_task2, PRO_CPU_NUM);
    configASSERT(handler_task2);
    
    // aguarda task1 finalizar
    while(eTaskGetState(handler_task1) == eRunning)
    {
        vTaskDelay(1 / portTICK_RATE_MS); // pra liberar o core
    } 

    // aguarda task2 finalizar
    while(eTaskGetState(handler_task2) == eRunning)
    {
        vTaskDelay(1 / portTICK_RATE_MS); // pra liberar o core
    } 

    printf("Peso total = %f\n", peso_total);
       
    //zera o peso total
	peso_total = 0;

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
        start_soma = esp_timer_get_time();
        soma_pesos(); 
        end_soma = esp_timer_get_time();
        total_time = ((double) (end_soma - start_soma)) / 1000000;
        printf("Tempo soma total: %f\n", total_time);

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
    for (int i = 0;i< TOUCH_PAD_MAX;i++) 
    {
        touch_pad_config(i, TOUCH_THRESH_NO_USE);
    }
}
 

// função que le os valores dos pinos que acionam o touch
static void tp_example_read_task(void *pvParameter)
{
    uint16_t touch_value;    
#if TOUCH_FILTER_MODE_EN
    printf("Touch Sensor filter mode read, the output format is: \nTouchpad num:[raw data, filtered data]\n\n");
#else
    printf("Touch Sensor normal mode read, the output format is: \nTouchpad num:[raw data]\n\n");
#endif
    while (1) 
    {

#if TOUCH_FILTER_MODE_EN
            // If open the filter mode, please use this API to get the touch pad count.
            touch_pad_read_raw_data(i, &touch_value);
            touch_pad_read_filtered(i, &touch_filter_value);
            printf("T%d:[%4d,%4d] ", i, touch_value, touch_filter_value);
#else
            touch_pad_read(0, &touch_value);
            if(touch_value < 1000)
            {
                printf("Desligando\n");
                printf("Reinicie para começar o programa novamente\n");
  
                vTaskDelete(handler_est1);
                vTaskDelete(handler_est2);
                vTaskDelete(handler_est3);
                vTaskDelete(handler_display);
                vTaskDelete(handler_touch);
            }
#endif
            if(touch_value < (uint16_t)100)
            {
                printf("\nSistema de Emergência Acinado");
                vTaskDelay(2000/portTICK_PERIOD_MS);
            }
        
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
} 
 
void app_main()
{
    nvs_flash_init();

    // inicializa semáforo
    mutual_exclusion_mutex = xSemaphoreCreateMutex();
    mutual_exclusion_mutex_soma = xSemaphoreCreateMutex();

    // Inicializa o touch
    touch_pad_init();

    if( mutual_exclusion_mutex == NULL )
    {
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
    xTaskCreate(&tp_example_read_task, "touch_pad_read_task", 2048, NULL, 5, &handler_touch);
    configASSERT(handler_touch);

    xTaskCreate(&esteira_1, "esteira_1", 2048, NULL, 3, &handler_est1);
    configASSERT(handler_est1);

    xTaskCreate(&esteira_2, "esteira_2", 2048, NULL, 3, &handler_est2);
    configASSERT(handler_est2);

    xTaskCreate(&esteira_3, "esteira_3", 2048, NULL, 3, &handler_est3);
    configASSERT(handler_est2);
    
    xTaskCreate(&display, "display", 2048, NULL, 1, &handler_display);
    configASSERT(handler_display);        

}