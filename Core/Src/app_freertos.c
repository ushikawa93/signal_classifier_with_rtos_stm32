/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : FreeRTOS applicative file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_freertos.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include "NanoEdgeAI.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define ADC_START (1 << 0)
#define ADC_HALF_READY  (1 << 1)
#define ADC_FULL_READY  (1 << 2)

#define DAC_FREQ   (1 << 0)
#define DAC_NOISE   (1 << 1)
#define DAC_WAVE   (1 << 2)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

// Señales para el ADC
uint32_t adc_buf[256];
uint32_t n_buffers_en_cola = 0;

// Señales para el DAC
volatile uint32_t nivel_ruido = 0;
volatile uint32_t frec_dac = 1;
volatile uint8_t signal_idx = 0;
volatile uint8_t buffer_completo_actualmente = 0;

// Señales para la clasificacion:
float input_float[128];

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

osThreadId_t blinkTaskHandle;
const osThreadAttr_t blinkyTask_attributes = {
  .name = "blinkyTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};

osThreadId_t dacTaskHandle;
const osThreadAttr_t dacTask_attributes = {
  .name = "dacTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};

osThreadId_t adcTaskHandle;
const osThreadAttr_t adcTask_attributes = {
  .name = "adcTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};

osThreadId_t uartTaskHandle;
const osThreadAttr_t uartTask_attributes = {
  .name = "uartTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 32
};


/* USER CODE END FunctionPrototypes */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  neai_classification_init();
  blinkTaskHandle = osThreadNew(StartBlinkyTask, NULL, &blinkyTask_attributes);
  dacTaskHandle = osThreadNew(StartDacTask, NULL, &dacTask_attributes);
  adcTaskHandle = osThreadNew(StartAdcTask, NULL, &adcTask_attributes);
  uartTaskHandle = osThreadNew(StartUartTask, NULL, &uartTask_attributes);

  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}
/* USER CODE BEGIN Header_StartDefaultTask */
/**
* @brief Function implementing the defaultTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN defaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END defaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */


/* -------------------- Seccion Blinky para probar -------------------- */
void StartBlinkyTask(void *argument)
{
	for(;;){
		BSP_LED_Toggle(LED_RED);
		osDelay(500);
	}
}

/* -------------------- Seccion Tareas de UART -------------------- */
void StartUartTask(void *argument)
{
	for(;;){
		printf(MENU_PRINCIPAL, nivel_ruido);
		uint8_t opcion;
		scanf(" %c",&opcion);

		switch (opcion){
		case '1':
		{
			char input[16];

			printf("\n\rIngrese numero de datos a adquirir ('c' para adquisicion continua  [q para salir])...\n\r");

			scanf("%15s", input);

			if(strcmp(input, "c") == 0 || strcmp(input, "C") == 0)
			{
			    n_buffers_en_cola = -1;
			}
			else
			{
			    int32_t buf = atoi(input);
			    n_buffers_en_cola = (buf > 0) ? buf : 4;
			}
			xTaskNotify(
					adcTaskHandle,
					ADC_START,
					eSetBits
			);
			while(n_buffers_en_cola != 0){}
			break;
		}
		case '2':
		{
			float similarities[neai_get_number_of_classes()];
			int id_class;
			uint32_t flags;

			uint8_t tecla = uart_getchar_timeout(0);

			while(tecla !='q' && tecla !='Q'){
				xTaskNotifyWait(0,
				                0xFFFFFFFF,
				                &flags,
				                portMAX_DELAY);
				if(flags & ADC_HALF_READY)
				{
					for(int i = 0; i < 128; i++)
					{
						 input_float[i] = (float)adc_buf[i];
					}
				}
	            if(flags & ADC_FULL_READY)
	            {
	                for(int i = 0; i < 128; i++)
					{
						input_float[i] = (float)adc_buf[i+128];
					}
	            }
				neai_classification(input_float,similarities,&id_class);
				char *time = convertir_a_tiempo(timer_tick);
				printf("\n\r%s: Señal clasificada como %s con %d%% de confianza [q para salir] \n\r", time, neai_get_class_name(id_class), (int)(similarities[id_class] * 100));
				tecla = uart_getchar_timeout(0);
			}
			break;
		}
		case '3':
		{
			printf("\n\rIngrese tipo de señal: \n\r	(0: Sinusoidal | 1: Cuadrada | 2: Triangular)  \n\r");
			uint8_t signal;
			scanf("%hhu",&signal);
			signal_idx = ((signal == 0)||(signal == 1)||(signal == 2))? signal : 0;
			xTaskNotify(
				dacTaskHandle,
				DAC_WAVE,
				eSetBits
			);
			break;
		}
		case '4':
		{
			printf("\n\rIngrese frecuencia deseada [Hz] (max 50): \n\r");
			uint32_t frec;
			scanf("%ld",&frec);
			frec_dac = (frec < 50)? frec : 1;
			xTaskNotify(
				dacTaskHandle,
				DAC_FREQ,
				eSetBits
			);
			break;
		}
		case '5':
		{
			printf("\n\rIngrese nivel de ruido: \n\r");
			scanf("%ld",&nivel_ruido);
			xTaskNotify(
				dacTaskHandle,
				DAC_NOISE,
				eSetBits
			);
			break;
		}
		case '6':
		{
		    printf("\n\rIniciando test sistematico...\n\r");
		    test_clasificacion_sistematico();
		    break;
		}
		default:
			printf("\n\rOpcion incorrecta\n\r");
		}
		fflush(stdin);
	}
}


// Recibe un caracter por UART con timeout en ms. Retorna 0 si no llego nada.
// Para recibir bloqueante uso scanf pero esto es necesario en casos que quiero trabajar sin bloquear todo!
static uint8_t uart_getchar_timeout(uint32_t timeout_ms)
{
    uint8_t ch = 0;
    HAL_StatusTypeDef status = HAL_UART_Receive(&hcom_uart[COM1], &ch, 1, timeout_ms);
    if(status == HAL_OK){
        return ch;
    }
    return 0;
}

/* -------------------- Seccion Tareas de DAC -------------------- */
void StartDacTask(void *argument)
{
	generateSignalsWithNoise(nivel_ruido);
	uint32_t eventos;

	HAL_DAC_Start_DMA(&hdac1,DAC_CHANNEL_1,(uint32_t*)signals[signal_idx],128,DAC_ALIGN_12B_R);
	HAL_TIM_Base_Start(&htim2);

	for(;;){
		if (xTaskNotifyWait(
		            0,          // no limpiar bits al entrar
		            0xFFFFFFFF, // limpiar todos los bits al salir
		            &eventos,
		            0           // no bloquear
		        ) == pdTRUE)
		{
			if (eventos & DAC_WAVE)
			{
				HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
				HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1,(uint32_t*)signals[signal_idx], 128, DAC_ALIGN_12B_R);
			}
		    if (eventos & DAC_NOISE)
		    {
		    	HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
		    	generateSignalsWithNoise(nivel_ruido);
		    	HAL_DAC_Start_DMA(&hdac1,DAC_CHANNEL_1,(uint32_t*)signals[signal_idx],128,DAC_ALIGN_12B_R);
		    }
		    if (eventos & DAC_FREQ)
		    {
		    	HAL_TIM_Base_Stop_IT(&htim2);
		    	__HAL_TIM_SET_AUTORELOAD(&htim2, getTIMER_DAC(frec_dac));
		    	__HAL_TIM_SET_COUNTER(&htim2, 0);
		    	HAL_TIM_Base_Start_IT(&htim2);

		    }
		}
	}
}

/* -------------------- Seccion Tareas de ADC -------------------- */
void StartAdcTask(void *argument)
{
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buf, 256);
	HAL_TIM_Base_Start(&htim15);

	for (;;)
	{
	    uint32_t flags;

	    xTaskNotifyWait(
	        0,
	        0xFFFFFFFF,
	        &flags,
	        portMAX_DELAY);

	    if(flags & ADC_START)
	    {

	        while((n_buffers_en_cola > 0)||(n_buffers_en_cola == -1))
	        {
	            xTaskNotifyWait(
	                0,
	                0xFFFFFFFF,
	                &flags,
	                portMAX_DELAY);

	            if(flags & ADC_HALF_READY)
	            {
	                procesar_adc(&adc_buf[0], 128);
	            }

	            if(flags & ADC_FULL_READY)
	            {
	                procesar_adc(&adc_buf[128], 128);
	            }

	            uint8_t tecla = uart_getchar_timeout(0);

	            if(n_buffers_en_cola != -1) {n_buffers_en_cola--;}
	            else if (tecla == 'q' || tecla == 'Q' ){n_buffers_en_cola = 0;};
	        }
	    }
	}
}

void procesar_adc(uint32_t * ptr_muestras,int n){

	char *t = convertir_a_tiempo(timer_tick); // Agregar un cronometro en el futuro

	printf("\r\n { \"ts\":%s , \"Muestras\": ",t);
	for(int i =0; i < 128;i++){
		printf("%lu, ", ptr_muestras[i]);
	}
	printf("}\r\n");

}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{

	if(adcTaskHandle == NULL)
	    return;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    buffer_completo_actualmente = 1;


    if((n_buffers_en_cola > 0)||(n_buffers_en_cola == -1)){
    	xTaskNotifyFromISR(adcTaskHandle,
   	                       ADC_HALF_READY,
   	                       eSetBits,
   	                       &xHigherPriorityTaskWoken);
    }
    else{
    	xTaskNotifyFromISR(uartTaskHandle,
   	                       ADC_HALF_READY,
   	                       eSetBits,
   	                       &xHigherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

}


void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{

	if(adcTaskHandle == NULL)
	    return;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    buffer_completo_actualmente = 2;

    if((n_buffers_en_cola > 0)||(n_buffers_en_cola == -1)){
        	xTaskNotifyFromISR(adcTaskHandle,
        					   ADC_FULL_READY,
       	                       eSetBits,
       	                       &xHigherPriorityTaskWoken);
        }
        else{
        	xTaskNotifyFromISR(uartTaskHandle,
        					   ADC_FULL_READY,
       	                       eSetBits,
       	                       &xHigherPriorityTaskWoken);
        }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

}


/* -------------------- Seccion Test sistemático -------------------- */
void test_clasificacion_sistematico(void)
{
    float similarities[neai_get_number_of_classes()];
    int id_class;
    uint32_t flags;

    uint32_t  niveles_ruido[] = {0, 100, 200, 500, 1000, 2000, 3000};
    uint8_t  n_niveles       = sizeof(niveles_ruido) / sizeof(niveles_ruido[0]);
    char    *nombres_señal[] = {"Sinusoidal", "Cuadrada", "Triangular"};

    printf("\n\r=== TEST SISTEMATICO DE CLASIFICACION ===\n\r");
    printf("Señal | Ruido | Clasificada como | Confianza\n\r");
    printf("------+-------+------------------+----------\n\r");

    for(uint8_t sig = 0; sig < 3; sig++)
    {
        // Cambiar señal del DAC
        signal_idx = sig;
        xTaskNotify(dacTaskHandle, DAC_WAVE, eSetBits);
        osDelay(2000); // Esperar que el DAC estabilice

        for(uint8_t niv = 0; niv < n_niveles; niv++)
        {
            // Cambiar nivel de ruido
            nivel_ruido = niveles_ruido[niv];
            xTaskNotify(dacTaskHandle, DAC_NOISE, eSetBits);
            osDelay(2000); // Esperar que el DAC regenere la señal

            // Esperar un buffer completo del ADC
            xTaskNotifyWait(0, 0xFFFFFFFF, &flags, portMAX_DELAY);

            if(flags & ADC_HALF_READY)
            {
                for(int i = 0; i < 128; i++)
                    input_float[i] = (float)adc_buf[i];
            }
            else if(flags & ADC_FULL_READY)
            {
                for(int i = 0; i < 128; i++)
                    input_float[i] = (float)adc_buf[i + 128];
            }

            neai_classification(input_float, similarities, &id_class);

            printf("%-10s | %5lu | %-16s | %d%%\n\r",
                   nombres_señal[sig],
                   nivel_ruido,
                   neai_get_class_name(id_class),
                   (int)(similarities[id_class] * 100));
        }
    }

    printf("\n\r=== FIN DEL TEST ===\n\r");

    // Restaurar estado inicial
    nivel_ruido = 0;
    signal_idx  = 0;
    xTaskNotify(dacTaskHandle, DAC_NOISE | DAC_WAVE, eSetBits);
}


static char* convertir_a_tiempo (int s)
{
	int segundos = s % 60;
	int minutos = (s/60) % 60;
	int horas = ((s/60)/60) % 24;

	static char tiempo_formateado[9];
	sprintf(tiempo_formateado,"%02d:%02d:%02d",horas,minutos,segundos);
	return tiempo_formateado;

}



/* USER CODE END Application */

