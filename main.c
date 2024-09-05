/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the   License.
*/

#include "ch.h"
#include "hal.h"
#include <string.h>
#include <stdio.h>
#include "definitions.h"

/*
  * Global Variables
*/ 
static msg_t queue[QUEUE_SIZE], *rdp, *wrp;
static size_t qsize;
static mutex_t qmtx;
static condition_variable_t qempty, qfull;
static virtual_timer_t vt;
uint8_t EVENT = 0;
state_via_t STATE = PRINCIPAL;
state_LED_t LED_PRINCIPAL = VERDE;
uint32_t counter = 0;

/*
  * Global Functions
*/
void InitBuffer(void);
void PushBUffer(msg_t msg);
uint8_t PopBUffer(void);
int IsBUfferEmpty(void);
int IsBufferFull(void);
void PrincipalTimer(void);

/* Virtual Timer */
static void AvenidaPrincipalSinalVerde(void *arg);
static void AvenidaPrincipalSinalAmarelo(void *arg);

/* 
  * Thread Write Event 
*/
static THD_WORKING_AREA(wa_WriteEvent, 128);
static THD_FUNCTION(Write_Save_Event, arg)
{
  chRegSetThreadName("Save/Write Event");
  while (1)
  {
    if (palReadPad(IOPORT2, PEDESTRE) == PAL_LOW)
      PushBUffer(PEDESTRE);

    if (palReadPad(IOPORT2, CARRO_SECUNDARIA) == PAL_LOW)
      PushBUffer(CARRO_SECUNDARIA);

    if (palReadPad(IOPORT2, AMBULANCIA_PRINCIPAL) == PAL_LOW)
      PushBUffer(AMBULANCIA_PRINCIPAL);

    if (palReadPad(IOPORT2, AMBULANCIA_SECUNDARIA) == PAL_LOW)
      PushBUffer(AMBULANCIA_SECUNDARIA);

    /* Debouce Delay */
    chThdSleepMilliseconds(100);
  }
}

/* 
  * Thread Read Event
*/
static THD_WORKING_AREA(wa_ReadEvent, 128);
static THD_FUNCTION(Read_Collect_Event, arg)
{
  chRegSetThreadName("Read/Collect Event");
  while (1)
  {
    if (!IsBUfferEmpty())
      EVENT = PopBUffer();
    chThdSleepMilliseconds(1);
  }
}

/* 
  * Thread Process Event 
*/
static THD_WORKING_AREA(wa_ProcessEvent, 128);
static THD_FUNCTION(ProcessEvent, arg)
{
  state_funcion_t state_funcion[1] = {PrincipalTimer};

  chRegSetThreadName("Process Event");
  while (1)
  {
    switch (STATE)
    {
      case IDLE_ST:
        break;
    
      case PRINCIPAL:
        state_funcion[PRINCIPAL - 1]();
        STATE = IDLE_ST;
        break;
    }

    chThdSleepMilliseconds(1);
  }
}

/*
 * Application entry point.
 */
int main(void) 
{
  thread_t *thd0 = 0, *thd1 = 0, *thd2 = 0;
  SerialConfig Serial_Configuration = {.sc_brr = UBRR2x(115200), .sc_bits_per_char = USART_CHAR_SIZE_8};

  InitBuffer();
  chVTObjectInit(&vt);
  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  sdStart(&SD1, &Serial_Configuration);

  /* Buttons */
  palSetPadMode(IOPORT2, PEDESTRE, PAL_MODE_INPUT_PULLUP);
  palSetPadMode(IOPORT2, CARRO_SECUNDARIA, PAL_MODE_INPUT_PULLUP);
  palSetPadMode(IOPORT2, AMBULANCIA_PRINCIPAL, PAL_MODE_INPUT_PULLUP);
  palSetPadMode(IOPORT2, AMBULANCIA_SECUNDARIA, PAL_MODE_INPUT_PULLUP);  

  /* LEDs main semaphore */
  palSetPadMode(IOPORT2, LED_VERDE_PRINCIPAL, PAL_MODE_OUTPUT_PUSHPULL);
  palClearPad(IOPORT2, LED_VERDE_PRINCIPAL);

  palSetPadMode(IOPORT4, LED_AMARELO_PRINCIPAL, PAL_MODE_OUTPUT_PUSHPULL);
  palClearPad(IOPORT4, LED_AMARELO_PRINCIPAL);

  palSetPadMode(IOPORT4, LED_VERMELHO_PRINCIPAL, PAL_MODE_OUTPUT_PUSHPULL);
  palClearPad(IOPORT4, LED_VERMELHO_PRINCIPAL);

  thd0 = chThdCreateStatic(wa_WriteEvent, sizeof(wa_WriteEvent), NORMALPRIO, Write_Save_Event, NULL);
  thd1 = chThdCreateStatic(wa_ReadEvent, sizeof(wa_ReadEvent), NORMALPRIO, Read_Collect_Event, NULL);
  thd2 = chThdCreateStatic(wa_ProcessEvent, sizeof(wa_ProcessEvent), NORMALPRIO, ProcessEvent, NULL);

  while (true) 
  {
    chThdSleepMilliseconds(1);
  }
}

void InitBuffer()
{
  chMtxObjectInit(&qmtx);
  chCondObjectInit(&qempty);
  chCondObjectInit(&qfull);
 
  rdp = wrp = &queue[0];
  qsize = 0;
}

void PushBUffer(msg_t msg)
{
  /* Entering monitor.*/
  chMtxLock(&qmtx);
 
  /* Waiting for space in the queue.*/
  while (IsBufferFull())
    chCondWait(&qfull);
 
  /* Writing the message in the queue.*/  
  *wrp = msg;
  if (++wrp >= &queue[QUEUE_SIZE])
    wrp = &queue[0];
  qsize++;

  /* Signaling that there is at least a message.*/
  chCondSignal(&qempty);

  /* Leaving monitor.*/
  chMtxUnlock(&qmtx);
}

uint8_t PopBUffer()
{
  msg_t msg;
 
  /* Entering monitor.*/
  chMtxLock(&qmtx);
 
  /* Waiting for messages in the queue.*/
  while (IsBUfferEmpty())
    chCondWait(&qempty);
 
  /* Reading the message from the queue.*/  
  msg = *rdp;
  if (++rdp >= &queue[QUEUE_SIZE])
    rdp = &queue[0];
  qsize--;
 
  /* Signaling that there is at least one free slot.*/
  chCondSignal(&qfull);
 
  /* Leaving monitor.*/
  chMtxUnlock(&qmtx);
 
  return (uint8_t)msg;
}

int IsBUfferEmpty()
{
  return qsize == 0;
}

int IsBufferFull()
{
  return qsize >= QUEUE_SIZE;
}

/*====================== Avenida Principal ===============================*/
void PrincipalTimer()
{
  switch (LED_PRINCIPAL)
  {
    case VERDE:
    {
      palSetPad(IOPORT2, LED_VERDE_PRINCIPAL);
      palClearPad(IOPORT4, LED_AMARELO_PRINCIPAL);
      palClearPad(IOPORT4, LED_VERMELHO_PRINCIPAL);
      chVTSet(&vt, TIME_MS2I(1000), AvenidaPrincipalSinalVerde, (void*)&vt);
      break;
    }

    case AMARELO:
    {
      palClearPad(IOPORT2, LED_VERDE_PRINCIPAL);
      palClearPad(IOPORT4, LED_VERMELHO_PRINCIPAL);
      palSetPad(IOPORT4, LED_AMARELO_PRINCIPAL);
      chVTSet(&vt, TIME_MS2I(1000), AvenidaPrincipalSinalAmarelo, (void*)&vt);
      break;
    }
  }  
}

static void AvenidaPrincipalSinalVerde(void *arg)
{
  chSysLockFromISR();
  
  counter++;

  while (EVENT == AMBULANCIA_PRINCIPAL)
    chThdSleepMilliseconds(1);

  if (counter >= 40 && (EVENT == PEDESTRE || EVENT == CARRO_SECUNDARIA))
  {
    counter = 0;
    palClearPad(IOPORT2, LED_VERDE_PRINCIPAL);
    STATE = PRINCIPAL;
    LED_PRINCIPAL = AMARELO;
    chVTReset((virtual_timer_t*)arg);
  }

  if (counter >= 10 && EVENT == AMBULANCIA_SECUNDARIA)
  {
    counter = 0;
    palClearPad(IOPORT2, LED_VERDE_PRINCIPAL);
    STATE = PRINCIPAL;
    LED_PRINCIPAL = AMARELO;
    chVTReset((virtual_timer_t*)arg);
  }

  chVTSetI((virtual_timer_t*)arg, TIME_MS2I(1000), AvenidaPrincipalSinalVerde, arg);
  chSysUnlockFromISR();
}

static void AvenidaPrincipalSinalAmarelo(void *arg)
{
  chSysLockFromISR();
  
  counter++;

  if (counter >= 5)
  {
    counter = 0;
    palClearPad(IOPORT2, LED_VERDE_PRINCIPAL);
    palClearPad(IOPORT4, LED_AMARELO_PRINCIPAL);
    palSetPad(IOPORT4, LED_VERMELHO_PRINCIPAL);
    chVTReset((virtual_timer_t*)arg);
  }

  chVTSetI((virtual_timer_t*)arg, TIME_MS2I(1000), AvenidaPrincipalSinalAmarelo, arg);
  chSysUnlockFromISR();
}
