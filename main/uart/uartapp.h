#ifndef UARTAPP_H
#define UARTAPP_H

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"

//uart pins
#define TXD_PIN 10
#define RXD_PIN 11
#define RTS_PIN (UART_PIN_NO_CHANGE)
#define CTS_PIN (UART_PIN_NO_CHANGE)
//#define EX_UART_NUM UART_NUM_0
#define BUF_SIZE (128)
//#define RD_BUF_SIZE (BUF_SIZE)


extern QueueHandle_t uart_app_queue; 
extern QueueHandle_t uart_app_queue_rx;


typedef enum{
    UPresion,
    SPage
}command_t;


struct uartDataIn{
    uint8_t command;
    uint8_t value;
};

struct ToUartData{
    command_t command;
    uint8_t value;
};

//Definiciones para trabajar con los registros usados en la pantalla
#define HEADER_LENGTH 2
#define ACK_LENGTH 6
#define READING_LENGTH 9

#define BRILLO_REG 0x0082
#define PRESION_REG 0x1002
#define TIEMPO_REG 0x1004
#define HUMEDAD_REG 0x1006

#define INIT_REG 0x1000
#define FUGAS_REG 0x1001
#define RUNNING 0x1005

#define PRESION_ACT 0x1003

#define PAGINA_ACT 0x0084

#define F_TESTING 0
#define F_HAPPY 1
#define F_SAD 2

/*funciones*/
void uart_app(void *pvParameters);

void checkSerialDwin();
void saveData();
bool checkSequence();
void changeToPage(uint8_t page);
bool writeDWIN(unsigned int reg, uint8_t value);
void initScreen();

#endif // UARTAPP_H

