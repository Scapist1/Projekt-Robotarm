#ifndef TIMER_H_
#define TIMER_H_

#include <avr/io.h>

// TCNT1 (Timer 1) tæller op konstant, OCR1A er slutværdien der sender TCNT1 tilbage til 0
// Vi bruger Timer 1 (16-bits), da det er mere præcist desto flere bits. Den har 65.535 ticks. En 8-bit har kun 255 ticks. stor forskel!

#define F_CPU 16000000UL        // clock frekvens (16 MHz)
#define PRESCALER 128            // prescaler (lav prescaler til små intervaller)
#define TARGET_INTERVAL 0.001   // interval vi ønsker i sekundet til vores ur
#define OCR_VALUE ((F_CPU / (PRESCALER * (1 / TARGET_INTERVAL))) - 1)   // OCR værdi: 249 ticks per millisekund (reset counter)

extern volatile uint16_t ms; // gør variabler synlige uden at oprette igen
extern volatile uint8_t ss_flag;
extern uint8_t ss, mm, hh;

void init_timer1(void); // prototype

#endif