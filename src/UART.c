#include "UART.h"
#include <avr/interrupt.h>

volatile char rx_buffer[16]; 
volatile uint8_t rx_pos = 0;
volatile uint8_t ny_data_klar = 0;  // flag der fortæller main om den skal læse ny data

void uart0_Init(unsigned int ubrr) {
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0A = (1 << U2X0); 
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0); 
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); 
}

void putchUSART0(char tx) {
    while (!(UCSR0A & (1 << UDRE0))); // UDRE0 bliver lav når data registeret er fyldt
    UDR0 = tx;  // CPU'en flytter alle 8 bits parallelt ind i UDR0
}

void printString(const char* s) {   // benytter print string
    while (*s) putchUSART0(*s++);
}

ISR(USART0_RX_vect) {
    char c = UDR0;

    // Hvis vi allerede har data, der venter på main, så stopper den med at modtage
    if (ny_data_klar) return; 

    if (c == '\n' || c == '\r') {
        if (rx_pos > 0) { // Kun hvis vi faktisk har modtaget tegn
            rx_buffer[rx_pos] = '\0'; 
            ny_data_klar = 1;         
            rx_pos = 0;
        }
    } 
    else if (rx_pos < 15) { // håndtere buffer overrun
        putchUSART0(c); // Echo tegnet tilbage til monitor, så vi kan se hvad vi skriver
        rx_buffer[rx_pos++] = c;    // gemmer i buffer
    }
}