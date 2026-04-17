#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include "ssd1306.h"
#include "I2C.h"

volatile uint16_t joystick_values[4]; // A0, A1, A2 og A3 værdier
volatile uint8_t current_ch = 0;

void init_ADC() {
  ADMUX = (1 << REFS0); // Reference spænding sat til 5V (AVCC) ("=" sætter REFS0 bit høj og resten lavt)
  ADCSRA |= (1 << ADEN) | (1 << ADIE); // enable ADC and enable interrupt adc complete
  ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Scaling 16 Mhz to 125 kHz ADC clock frequency with prescaler 128
  DIDR0 = 0xFF;  // Deaktiverer alle digitale input A0-A7 for at spare på strømmen
}

void init_timer0() {
  TCCR0A = (1 << WGM01);  // Sæt Timer0 til CTC mode
  OCR0A = 155;     // Sæt sammenligningsværdi (OCR0A) med prescaler 1024: (16MHz / 1024) / 156 ≈ 100 Hz (10ms mellem samples)
  TIMSK0 = (1 << OCIE0A); // Enable Timer Compare Interrupt
  TCCR0B = (1 << CS02) | (1 << CS00);  // Start timer med prescaler 1024
}

ISR(TIMER0_COMPA_vect)  { // starter sampling
  ADMUX = (1 << REFS0) | (current_ch & 0x07); // Vælg den aktuelle kanal i ADMUX (bevar REFS0) 
  ADCSRA |= (1 << ADSC);  // Start konvertering (Sæt ADSC bit)
}

ISR(ADC_vect) { // henter resultat fra sampling
  
  joystick_values[current_ch] = ADC;

  current_ch++;
  if (current_ch > 3) {
    current_ch = 0;
  }
}

int main(void) {
  char buffer[20];
  
  init_ADC();
  init_timer0();
  I2C_Init(); 
  InitializeDisplay();
    
  sei();
    
  while (1) {
      for(uint8_t i = 0; i < 4; i++) {
          sprintf(buffer, "Kanal %d: %4d", i, joystick_values[i]);     
          sendStrXY(buffer, i + 2, 0);
      }
      _delay_ms(50); // Opdater ca. 20 gange i sekundet
  }
}