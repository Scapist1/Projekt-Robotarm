#ifndef UART_H_
#define UART_H_

#include <avr/io.h>
#include <stdint.h>

extern volatile char rx_buffer[16];
extern volatile uint8_t ny_data_klar;

void uart0_Init(unsigned int ubrr);
void putchUSART0(char tx);
void printString(const char* s);

#endif