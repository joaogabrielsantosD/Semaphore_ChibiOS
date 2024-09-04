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
static condition_variable_t qempty;
static condition_variable_t qfull;
uint8_t STATE = 0;

/*
  * Global Functions
*/
void InitBuffer(void);
void PushBUffer(msg_t msg);
uint8_t PopBUffer(void);
int IsBUfferEmpty(void);
int IsBufferFull(void);

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
  char r[5];
  chRegSetThreadName("Read/Collect Event");
  while (1)
  {
    if (!IsBUfferEmpty())
      STATE = PopBUffer();

    snprintf(r, 5, "%d\r\n", STATE);
    sdWrite(&SD1, &r, sizeof(r));
    chThdSleepMilliseconds(1);
  }
}

/* 
  * Thread Process Event 
*/
static THD_WORKING_AREA(wa_ProcessEvent, 128);
static THD_FUNCTION(ProcessEvent, arg)
{
  char msg[] = "Maquina de estado\r\n";

  chRegSetThreadName("Process Event");
  while (1)
  {
    sdWrite(&SD1, msg, sizeof(msg));
    chThdSleepMilliseconds(100);
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

  palSetPadMode(IOPORT2, PEDESTRE, PAL_MODE_INPUT_PULLUP);
  palSetPadMode(IOPORT2, CARRO_SECUNDARIA, PAL_MODE_INPUT_PULLUP);
  palSetPadMode(IOPORT2, AMBULANCIA_PRINCIPAL, PAL_MODE_INPUT_PULLUP);
  palSetPadMode(IOPORT2, AMBULANCIA_SECUNDARIA, PAL_MODE_INPUT_PULLUP);  

  thd0 = chThdCreateStatic(wa_WriteEvent, sizeof(wa_WriteEvent), NORMALPRIO, Write_Save_Event, NULL);
  thd1 = chThdCreateStatic(wa_ReadEvent, sizeof(wa_ReadEvent), NORMALPRIO, Read_Collect_Event, NULL);
  //thd2 = chThdCreateStatic(wa_ProcessEvent, sizeof(wa_ProcessEvent), NORMALPRIO, ProcessEvent, NULL);

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
