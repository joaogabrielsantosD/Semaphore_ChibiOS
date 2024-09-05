#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#define QUEUE_SIZE 128

/* GPIOs */
// Events
#define EVENT_1 4 // PB4
#define EVENT_2 3 // PB3
#define EVENT_3 2 // PB2
#define EVENT_4 1 // PB1

// Leds
#define LED_VERDE_PRINCIPAL    0 // PB0
#define LED_AMARELO_PRINCIPAL  7 // PD7
#define LED_VERMELHO_PRINCIPAL 6 // PD6

/* Events */
#define PEDESTRE              EVENT_1
#define CARRO_SECUNDARIA      EVENT_2
#define AMBULANCIA_PRINCIPAL  EVENT_3
#define AMBULANCIA_SECUNDARIA EVENT_4

typedef enum 
{
    IDLE_ST = 0,
    PRINCIPAL,
    SECUNDARIA,
    _PEDESTRE
} state_via_t;

typedef enum 
{
    VERDE = 0,
    AMARELO,
    VERMELHO
} state_LED_t;

typedef void (*state_funcion_t)(void);

#endif