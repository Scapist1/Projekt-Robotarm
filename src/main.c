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
  // ADMUX = (1 << REFS0) | (current_ch & 0x07); // Vælg den aktuelle kanal i ADMUX (bevar REFS0) 
  ADCSRA |= (1 << ADSC);  // Start konvertering (Sæt ADSC bit)
}

ISR(ADC_vect) { // henter resultat fra sampling
  
  joystick_values[current_ch] = ADC;

  current_ch++;
  if (current_ch > 3) current_ch = 0;

  ADMUX = (1 << REFS0) | (current_ch & 0x07);
}

void init_ph_frPWM()  {
  DDRB |= (1 << PB5); // pin 11
  TCCR1A |= (1 << COM1A1);  // Clear OC1A on Compare Match when up-counting. Set OC1A on Compare Match when down-counting
  TCCR1B |= (1 << CS11) | (1 << WGM13); // prescaling by 8
  ICR1 = 20000; // top value then OC1A pin can be used // 8bit top value
  OCR1A = 1500;  // 50 duty cycle
}


int main(void) {
  char buffer[20];
  
  init_ADC();
  init_timer0();
  I2C_Init();
  clear_display();
  InitializeDisplay();
  
  sei();
    
  while (1) {
    static int16_t smooth_values[4] = {512, 512, 512, 512}; 

    for(uint8_t i = 0; i < 4; i++) {
      // 2. Atomic read: copy the volatile value safely
      cli();
      int16_t raw_joy = joystick_values[i]; 
      sei();

    if (raw_joy > 524) {
        smooth_values[i] += (raw_joy - 512) / 64;
    } else if (raw_joy < 500) {
        smooth_values[i] -= (512 - raw_joy) / 64;
    }
    if (smooth_values[i] > 1023) smooth_values[i] = 1023;
    if (smooth_values[i] < 0)    smooth_values[i] = 0;

    }

    for(uint8_t i = 0; i < 4; i++) {
        sprintf(buffer, "Kanal %d: %4d", i, smooth_values[i]);     
        sendStrXY(buffer, i + 2, 0);
    }
    _delay_ms(50); // Opdater ca. 20 gange i sekundet
  }
}