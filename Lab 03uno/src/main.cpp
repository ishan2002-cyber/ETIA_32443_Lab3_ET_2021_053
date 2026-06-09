#include <Arduino.h>
#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// ---------------- LED Pins ----------------
#define ROAD_RED      PB0
#define ROAD_YELLOW   PB1
#define ROAD_GREEN    PB2

#define PED_RED       PB3
#define PED_GREEN     PB4

// ---------------- Flags ----------------
volatile uint8_t emergency_flag = 0;
volatile uint8_t pedestrian_flag = 0;

// Maintenance mode memory flag
volatile uint8_t maintenance_mode_flag = 0;

// ---------------- Functions ----------------

void all_leds_off()
{
    PORTB &= ~((1 << ROAD_RED) |
               (1 << ROAD_YELLOW) |
               (1 << ROAD_GREEN) |
               (1 << PED_RED) |
               (1 << PED_GREEN));
}

// ---------------- Normal RED State ----------------
void normal_red_state()
{
    PORTB |= (1 << ROAD_RED);
    PORTB &= ~((1 << ROAD_GREEN) | (1 << ROAD_YELLOW));

    PORTB |= (1 << PED_GREEN);
    PORTB &= ~(1 << PED_RED);
}

// ---------------- Normal GREEN State ----------------
void normal_green_state()
{
    PORTB |= (1 << ROAD_GREEN);
    PORTB &= ~((1 << ROAD_RED) | (1 << ROAD_YELLOW));

    PORTB |= (1 << PED_RED);
    PORTB &= ~(1 << PED_GREEN);
}

// ---------------- Emergency Mode ----------------
void emergency_mode()
{
    emergency_flag = 0;

    // Road GREEN
    PORTB |= (1 << ROAD_GREEN);
    PORTB &= ~((1 << ROAD_RED) | (1 << ROAD_YELLOW));

    // Pedestrian RED
    PORTB |= (1 << PED_RED);
    PORTB &= ~(1 << PED_GREEN);

    for(int i = 0; i < 10; i++)
    {
        _delay_ms(1000);
    }
}

// ---------------- Pedestrian Mode ----------------
void pedestrian_mode()
{
    pedestrian_flag = 0;

    // Yellow ON
    PORTB |= (1 << ROAD_YELLOW);

    for(int i = 0; i < 5; i++)
    {
        _delay_ms(1000);
    }

    // Road RED
    PORTB |= (1 << ROAD_RED);
    PORTB &= ~((1 << ROAD_GREEN) | (1 << ROAD_YELLOW));

    // Pedestrian GREEN
    PORTB |= (1 << PED_GREEN);
    PORTB &= ~(1 << PED_RED);

    for(int i = 0; i < 10; i++)
    {
        _delay_ms(1000);
    }
}

// ---------------- Maintenance Mode ----------------
void maintenance_mode()
{
    all_leds_off();

    // Blink Yellow LED
    PORTB |= (1 << ROAD_YELLOW);
    _delay_ms(500);

    PORTB &= ~(1 << ROAD_YELLOW);
    _delay_ms(500);
}

// ---------------- Interrupts ----------------

// Emergency Interrupt
ISR(INT0_vect)
{
    emergency_flag = 1;
}

// Pedestrian Interrupt
ISR(INT1_vect)
{
    pedestrian_flag = 1;
}

// Maintenance Interrupt
ISR(PCINT1_vect)
{
    // Toggle maintenance mode
    maintenance_mode_flag ^= 1;
}

// ---------------- Main ----------------

int main(void)
{
    // LED pins OUTPUT
    DDRB |= (1 << ROAD_RED) |
            (1 << ROAD_YELLOW) |
            (1 << ROAD_GREEN) |
            (1 << PED_RED) |
            (1 << PED_GREEN);

    // Button pins INPUT
    DDRD &= ~((1 << PD2) | (1 << PD3));
    DDRC &= ~(1 << PC0);

    // Enable internal pull-up resistors
    PORTD |= (1 << PD2) | (1 << PD3);
    PORTC |= (1 << PC0);

    // ---------------- Interrupt Setup ----------------

    // INT0 Falling Edge
    EICRA |= (1 << ISC01);
    EICRA &= ~(1 << ISC00);

    // INT1 Falling Edge
    EICRA |= (1 << ISC11);
    EICRA &= ~(1 << ISC10);

    // Enable INT0 and INT1
    EIMSK |= (1 << INT0) | (1 << INT1);

    // Enable Pin Change Interrupt for PC0
    PCICR |= (1 << PCIE1);

    // Enable PC0 (PCINT8)
    PCMSK1 |= (1 << PCINT8);

    // Enable Global Interrupts
    sei();

    // ---------------- Main Loop ----------------

    while(1)
    {
        // -------- Maintenance Mode --------
        // First Press  -> ON
        // Second Press -> OFF

        if(maintenance_mode_flag)
        {
            maintenance_mode();

            // wait until button release
            while(!(PINC & (1 << PC0)));
        }

        // -------- Emergency Mode --------

        else if(emergency_flag)
        {
            emergency_mode();
        }

        // -------- Pedestrian Mode --------

        else if(pedestrian_flag)
        {
            pedestrian_mode();
        }

        // -------- Normal RED State --------

        else
        {
            normal_red_state();

            for(int i = 0; i < 5; i++)
            {
                _delay_ms(1000);

                if(emergency_flag ||
                   pedestrian_flag ||
                   maintenance_mode_flag)
                    break;
            }

            if(emergency_flag ||
               pedestrian_flag ||
               maintenance_mode_flag)
                continue;

            // -------- Normal GREEN State --------

            normal_green_state();

            for(int i = 0; i < 5; i++)
            {
                _delay_ms(1000);

                if(emergency_flag ||
                   pedestrian_flag ||
                   maintenance_mode_flag)
                    break;
            }
        }
    }
}