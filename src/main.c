#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ssd1306.h"
#include "I2C.h"
#include <UART.h>  // angle brackets så test/UART.h ikke skygger include/UART.h

#define BAUD 115200
#define UBRR_VAL ((F_CPU / 8 / BAUD) - 1)

// Slew-rate matcher rigtig_main's joystick max: (1023-512)/64 = 7.98 ≈ 8
// smooth_values per tick ved fuldt udsving. UART target og dance bruger samme
// rate så bevægelsen føles ens uanset om man styrer med joystick eller serial.
#define SLEW_STEP_SMOOTH 8

// Konverterer pwm-mikrosekunder (500-2500) til smooth_values (0-1023). Brugt
// af UART-parser (kommandoer skrives som n:value i us) og dance-targets.
#define SMOOTH_OF_PWM(pwm_us) ((int16_t)(((int32_t)(pwm_us) - 500) * 1023L / 2000))

volatile uint16_t joystick_values[4]; // A0, A1, A2 og A3 værdier
volatile uint8_t current_ch = 0;

void init_ADC()
{
    ADMUX = (1 << REFS0);                                 // Reference spænding sat til 5V (AVCC) ("=" sætter REFS0 bit høj og resten lavt)
    ADCSRA |= (1 << ADEN) | (1 << ADIE);                  // enable ADC and enable interrupt adc complete
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Scaling 16 Mhz to 125 kHz ADC clock frequency with prescaler 128
    DIDR0 = 0xFF;                                         // Deaktiverer alle digitale input A0-A7 for at spare på strømmen
}

void init_timer0()
{
    TCCR0A = (1 << WGM01);              // Sæt Timer0 til CTC mode
    OCR0A = 155;                        // Sæt sammenligningsværdi (OCR0A) med prescaler 1024: (16MHz / 1024) / 156 ≈ 100 Hz (10ms mellem samples)
    TIMSK0 = (1 << OCIE0A);             // Enable Timer Compare Interrupt
    TCCR0B = (1 << CS02) | (1 << CS00); // Start timer med prescaler 1024
}

ISR(TIMER0_COMPA_vect)
{                                               // starter sampling
    ADMUX = (1 << REFS0) | (current_ch & 0x07); // Vælg den aktuelle kanal i ADMUX (bevar REFS0)
    ADCSRA |= (1 << ADSC);                      // Start konvertering (Sæt ADSC bit)
}

ISR(ADC_vect)
{ // henter resultat fra sampling

    joystick_values[current_ch] = ADC;

    current_ch++;
    if (current_ch > 3)
        current_ch = 0;
}

void init_ph_frPWM_1()
{                                         // Timer 1
    DDRB |= (1 << PB5);                   // pin 11
    TCCR1A |= (1 << COM1A1);              // Clear OC1A on Compare Match when up-counting. Set OC1A on Compare Match when down-counting
    TCCR1B |= (1 << CS11) | (1 << WGM13); // prescaling by 8
    ICR1 = 20000;                         // top value then OC1A pin can be used // 8bit top value
    OCR1A = 1500;                         // 7.5% duty cycle
}

void init_ph_frPWM_2()
{                                         // Timer 3
    DDRE |= (1 << PE3);                   // pin 5
    TCCR3A |= (1 << COM3A1);              // Clear OC1A on Compare Match when up-counting. Set OC1A on Compare Match when down-counting
    TCCR3B |= (1 << CS31) | (1 << WGM33); // prescaling by 8
    ICR3 = 20000;                         // top value then OC1A pin can be used // 8bit top value
    OCR3A = 1500;                         // 7.5% duty cycle
}

void init_ph_frPWM_3()
{                                         // Timer 4
    DDRH |= (1 << PH3);                   // pin 6
    TCCR4A |= (1 << COM4A1);              // Clear OC1A on Compare Match when up-counting. Set OC1A on Compare Match when down-counting
    TCCR4B |= (1 << CS41) | (1 << WGM43); // prescaling by 8
    ICR4 = 20000;                         // top value then OC1A pin can be used // 8bit top value
    OCR4A = 1500;                         // 7.5% duty cycle // bliver ændret af int fra joystick
}

void init_ph_frPWM_4()
{                                         // Timer 5
    DDRL |= (1 << PL3);                   // pin 46
    TCCR5A |= (1 << COM5A1);              // Clear OC1A on Compare Match when up-counting. Set OC1A on Compare Match when down-counting
    TCCR5B |= (1 << CS51) | (1 << WGM53); // prescaling by 8
    ICR5 = 20000;                         // top value then OC1A pin can be used // 8bit top value
    OCR5A = 1500;                         // 7.5% duty cycle
}

// Skriver alle 4 OCR-registre ud fra smooth_values med samme mapping som main loop.
static void apply_smooth(int16_t smooth_values[4])
{
    for (uint8_t i = 0; i < 4; i++)
    {
        if (smooth_values[i] > 1023) smooth_values[i] = 1023;
        if (smooth_values[i] < 0)    smooth_values[i] = 0;
        uint32_t temp = (uint32_t)smooth_values[i] * 2000;
        uint16_t pwm = (uint16_t)(temp / 1023) + 500;
        if (i == 0) OCR1A = pwm;
        if (i == 1) OCR3A = pwm;
        if (i == 2) OCR4A = pwm;
        if (i == 3) OCR5A = pwm;
    }
}

// Slew alle 4 servoer mod target[] med max_step per 20ms tick. Blokerer indtil i mål.
static void slew_to(int16_t smooth_values[4], const int16_t target[4], int16_t max_step)
{
    uint8_t done = 0;
    while (!done)
    {
        done = 1;
        for (uint8_t i = 0; i < 4; i++)
        {
            int16_t diff = target[i] - smooth_values[i];
            if (diff > max_step)       { smooth_values[i] += max_step; done = 0; }
            else if (diff < -max_step) { smooth_values[i] -= max_step; done = 0; }
            else                        { smooth_values[i]  = target[i]; }
        }
        apply_smooth(smooth_values);
        _delay_ms(20);
    }
}

static void run_dance(int16_t smooth_values[4])
{
    // Servo 4 holder bare sin nuværende position
    int16_t place[4] = {SMOOTH_OF_PWM(2000), SMOOTH_OF_PWM(1400), SMOOTH_OF_PWM(1450), smooth_values[3]};
    int16_t fwd[4]   = {SMOOTH_OF_PWM(2200), SMOOTH_OF_PWM(1600), SMOOTH_OF_PWM(1650), smooth_values[3]};

    // Move into place, så fire gange (frem, tilbage)
    slew_to(smooth_values, place, SLEW_STEP_SMOOTH);
    for (uint8_t reps = 0; reps < 4; reps++)
    {
        slew_to(smooth_values, fwd,   SLEW_STEP_SMOOTH);
        slew_to(smooth_values, place, SLEW_STEP_SMOOTH);
    }
}

static void handle_uart_cmd(int16_t target_smooth[4], uint8_t uart_active[4], int16_t smooth_values[4])
{
    char cmd[16];
    cli();
    memcpy(cmd, (const void *)rx_buffer, 16);
    ny_data_klar = 0;
    sei();

    printString("\r\n"); // ISR echoer ikke newline; tilføj selv før svar

    if (strcmp(cmd, "dance") == 0)
    {
        printString("OK dance\r\n");
        run_dance(smooth_values);
        for (uint8_t i = 0; i < 4; i++)
        {
            target_smooth[i] = smooth_values[i];
            uart_active[i] = 0;
        }
        printString("dance done\r\n");
        return;
    }

    if (cmd[0] < '1' || cmd[0] > '4' || cmd[1] != ':')
    {
        printString("ERR: brug n:value (n=1-4) eller 'dance'\r\n");
        return;
    }

    int value = atoi(&cmd[2]);
    if (value < 500 || value > 2500)
    {
        printString("ERR: value skal vaere 500-2500\r\n");
        return;
    }

    uint8_t servo = cmd[0] - '1'; // 0..3
    target_smooth[servo] = SMOOTH_OF_PWM(value);
    uart_active[servo] = 1;

    char buf[24];
    sprintf(buf, "OK %u:%d\r\n", (unsigned)(servo + 1), value);
    printString(buf);
}

int main(void)
{
    char buffer[20];

    init_ADC();
    init_timer0();
    init_ph_frPWM_1();
    init_ph_frPWM_2();
    init_ph_frPWM_3();
    init_ph_frPWM_4();
    I2C_Init();
    clear_display();
    InitializeDisplay();
    uart0_Init(UBRR_VAL);

    sei();

    static int16_t smooth_values[4] = {512, 512, 512, 512};
    static int16_t target_smooth[4] = {512, 512, 512, 512};
    static uint8_t uart_active[4] = {0, 0, 0, 0};
    static uint8_t display_counter = 0;

    printString("Klar. Cmd: n:value (n=1-4, value=500-2500) eller 'dance'\r\n");

    while (1)
    {
        if (ny_data_klar)
        {
            handle_uart_cmd(target_smooth, uart_active, smooth_values);
        }

        for (uint8_t i = 0; i < 4; i++)
        {
            // 2. Atomic read: copy the volatile value safely
            cli();
            int16_t raw_joy = joystick_values[i];
            sei();

            // Lineær respons: 1 smooth-step per 64 ADC-units afvigelse fra center.
            // Kvadratisk variant (blødt nær center, hurtigt ved fuldt udsving) ville være:
            //     int32_t dev = raw_joy - 512;
            //     int32_t abs_dev = dev < 0 ? -dev : dev;
            //     smooth_values[i] += dev * abs_dev / 32768;   // erstatter begge if-grene
            // Divisoren 32768 er valgt så max-step (511*511/32768 ≈ 7) matcher den lineære.
            if (raw_joy > 524)
            {
                uart_active[i] = 0;
                smooth_values[i] += (raw_joy - 512) / 64;
            }
            else if (raw_joy < 500)
            {
                uart_active[i] = 0;
                smooth_values[i] -= (512 - raw_joy) / 64;
            }
            else if (uart_active[i])
            {
                // Slew mod UART-target med samme rate som joystick-max
                int16_t diff = target_smooth[i] - smooth_values[i];
                if (diff > SLEW_STEP_SMOOTH)       { smooth_values[i] += SLEW_STEP_SMOOTH; }
                else if (diff < -SLEW_STEP_SMOOTH) { smooth_values[i] -= SLEW_STEP_SMOOTH; }
                else                                { smooth_values[i] = target_smooth[i]; uart_active[i] = 0; }
            }

            if (smooth_values[i] > 1023)
                smooth_values[i] = 1023;
            if (smooth_values[i] < 0)
                smooth_values[i] = 0;

            // Map 0-1023 to 500-2500
            uint32_t temp = (uint32_t)smooth_values[i] * 2000; // 2000 span from 500 - 2500
            uint16_t pwm = (uint16_t)(temp / 1023) + 500;      // 500 offset

            // Assign to the correct hardware register
            if (i == 0)
                OCR1A = pwm;
            if (i == 1)
                OCR3A = pwm;
            if (i == 2)
                OCR4A = pwm;
            if (i == 3)
                OCR5A = pwm;
        }

        // Display kun hver 10. iteration så I2C ikke bremser servo-opdateringen
        if (++display_counter >= 10)
        {
            display_counter = 0;
            for (uint8_t i = 0; i < 4; i++)
            {
                uint32_t temp = (uint32_t)smooth_values[i] * 2000;
                uint16_t pwm = (uint16_t)(temp / 1023) + 500;

                sprintf(buffer, "%4d", smooth_values[i]);
                sendStrXY(buffer, i + 2, 0);
                sprintf(buffer, "%4d", pwm);
                sendStrXY(buffer, i + 2, 5);
                // pwm/200 og (pwm%200)/20 → heltals- og tiendedels-procent uden float
                sprintf(buffer, "%2u,%u%%", pwm / 200, (pwm % 200) / 20);
                sendStrXY(buffer, i + 2, 10);
            }
        }

        _delay_ms(20); // Opdater ca. 50 gange i sekundet
    }
}
