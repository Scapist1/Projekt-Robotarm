
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include "ssd1306.h"
#include "I2C.h"
#define SERVO_MID 1500

void init_timer0()
{
    TCCR0A = (1 << WGM01);              // Sæt Timer0 til CTC mode
    OCR0A = 155;                        // Sæt sammenligningsværdi (OCR0A) med prescaler 1024: (16MHz / 1024) / 156 ≈ 100 Hz (10ms mellem samples)
    TIMSK0 = (1 << OCIE0A);             // Enable Timer Compare Interrupt
    TCCR0B = (1 << CS02) | (1 << CS00); // Start timer med prescaler 1024
}

void init_ph_frPWM()
{
    DDRB |= (1 << PB5);                   // enable output på pin 11
    TCCR1A |= (1 << COM1A1);              // Clear OC1A on Compare Match when up-counting. Set OC1A on Compare Match when down-counting
    TCCR1B |= (1 << CS11) | (1 << WGM13); // prescaling by 8
    ICR1 = 20000;                         // top value then OC1A pin can be used // 8bit top value
    OCR1A = SERVO_MID;                    // 50 duty cycle (er det ikke 7,5%?)
    TCNT1 = 0;                            // Sikrer os at tælleren starter fra 0
};


void main(){

    char sweep_buffer[20];

    init_ADC();
    init_timer0();
    I2C_Init();
    init_ph_frPWM();
    clear_display();
    InitializeDisplay();

    // PWM test sweep: (Jeg håber det virker som jeg tænker, at den tager én ad gangen sekventielt og ikke har konflikt)
    for (uint16_t position = 1500; position <= 2500; position += 10) // sweep op
    { 
        OCR1A = position;
        sprintf(sweep_buffer, "Position: %4d", OCR1A);
        sendStrXY(sweep_buffer, 0, 0);
        clear_display();
        _delay_ms(20); // venter en fuld pwm period
    }
    for (uint16_t position = 2500; position >= 500; position -= 10) // sweep helt ned
    {
        OCR1A = position;
        sprintf(sweep_buffer, "Position: %4d", OCR1A);
        sendStrXY(sweep_buffer, 0, 0);
        clear_display();
        _delay_ms(20); // venter en fuld pwm period
    }
    for (uint16_t position = 500; position <= 1500; position += 10) // sweep tilbage i midten
    {
        OCR1A = position;
        sprintf(sweep_buffer, "Position: %4d", OCR1A);
        sendStrXY(sweep_buffer, 0, 0);
        clear_display();
        _delay_ms(20); // venter en fuld pwm period
    }
}