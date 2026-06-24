# Signal Classifier RTOS

Clasificador de formas de onda en tiempo real utilizando NanoEdge AI Studio sobre una placa STM32 NUCLEO-U575ZI-Q.

El sistema utiliza FreeRTOS para ejecutar en paralelo la generación de señales, adquisición de datos, clasificación y comunicación UART mediante tareas independientes sincronizadas con Task Notifications.

El proyecto incluye tanto el firmware embebido para STM32 como una aplicación de escritorio en Python para monitoreo, captura y visualización de señales en tiempo real.

## Descripción

El DAC genera señales sinusoidales, cuadradas o triangulares mediante DMA. Estas señales son adquiridas por el ADC utilizando DMA circular con doble buffer.

Los datos adquiridos pueden:

* Transmitirse por UART para generación de datasets.
* Clasificarse mediante un modelo de Machine Learning generado con NanoEdge AI Studio.
* Utilizarse para pruebas y validación del sistema en tiempo real.

La comunicación entre tareas se realiza mediante Task Notifications de FreeRTOS.

## Hardware

* STM32 NUCLEO-U575ZI-Q
* Conexión entre la salida DAC1 y la entrada ADC1
* Terminal serie configurada a 115200 baudios

## Arquitectura

El firmware se divide en las siguientes tareas:

### DAC Task

Responsable de:

* Generar señales sinusoidales, cuadradas y triangulares.
* Actualizar frecuencia de salida.
* Agregar ruido configurable.
* Controlar el DMA del DAC.

### ADC Task

Responsable de:

* Procesar eventos de ADC Half Complete y ADC Complete.
* Adquirir muestras mediante DMA circular.
* Transmitir buffers completos por UART para captura de datasets.

### UART Task

Responsable de:

* Mostrar el menú interactivo.
* Recibir comandos del usuario.
* Configurar parámetros del sistema.
* Ejecutar clasificaciones bajo demanda.

### Blinky Task

Tarea auxiliar utilizada para verificar el correcto funcionamiento del scheduler.

## Menú UART

Al iniciar el sistema se presenta el siguiente menú:

### 1 - Captura de datos

Permite transmitir buffers adquiridos por el ADC.

Opciones:

* Número fijo de buffers.
* Modo continuo (`c`).

Durante la captura continua:

* Presionar `q` para finalizar.

### 2 - Clasificar señal actual

Clasifica el último buffer adquirido utilizando NanoEdge AI Studio.

Se muestra:

* Clase detectada.
* Nivel de confianza.

### 3 - Seleccionar forma de onda

Opciones:

* 0 → Sinusoidal
* 1 → Cuadrada
* 2 → Triangular

### 4 - Cambiar frecuencia

Permite modificar la frecuencia de salida del DAC.

Rango recomendado:

* 1 Hz a 50 Hz

### 5 - Configurar ruido

Permite agregar ruido aleatorio a la señal generada.

## Adquisición

### DAC

* DMA circular
* Timer TIM2 como trigger
* Buffer de 128 muestras

### ADC

* DMA circular
* Doble buffer de 256 muestras
* Eventos:

  * Half Complete
  * Complete

## Clasificación

El modelo de clasificación fue generado utilizando NanoEdge AI Studio.

Clases soportadas:

* Sinusoidal
* Cuadrada
* Triangular

La clasificación se ejecuta sobre bloques de 128 muestras adquiridas por el ADC.

## Comunicación Serie

Configuración:

* Baudrate: 115200
* Data bits: 8
* Paridad: None
* Stop bits: 1

## Software

### Herramientas utilizadas

* STM32CubeIDE
* STM32CubeMX
* FreeRTOS
* NanoEdge AI Studio

## Aplicación de Monitoreo para PC

El repositorio incluye una aplicación de escritorio desarrollada en Python utilizando PySide6 para interactuar con el STM32 mediante UART.

### Funcionalidades

* Conexión y desconexión serie.
* Selección automática de puertos COM disponibles.
* Configuración de baudrate.
* Envío de comandos al firmware.
* Visualización de mensajes RX/TX en tiempo real.
* Exportación de logs a archivos de texto.
* Visualización gráfica de los buffers adquiridos por el ADC.
* Acumulación de múltiples buffers para inspección temporal de la señal.

### Formato de Datos

La aplicación interpreta automáticamente los mensajes JSON enviados por el firmware durante la captura:

```json
{
    "ts": "00:00:00",
    "Muestras": [ ... ]
}
```

Los datos recibidos son procesados y representados gráficamente en tiempo real.

### Dependencias

```bash
pip install PySide6 pyserial
```

### Ejecución

```bash
python main.py
```

La herramienta resulta útil para:

* Verificar el funcionamiento del ADC.
* Visualizar datasets antes de exportarlos.
* Validar la generación de señales.
* Depurar la comunicación serie.
* Analizar rápidamente los datos utilizados por NanoEdge AI.


### MCU

* STM32U575ZI-Q
* Cortex-M33

## Estado del proyecto

Características implementadas:

* Generación de señales por DMA.
* Adquisición ADC por DMA circular.
* Clasificación con NanoEdge AI.
* Interfaz UART interactiva.
* Control de frecuencia.
* Inyección de ruido.
* Arquitectura basada en FreeRTOS.
