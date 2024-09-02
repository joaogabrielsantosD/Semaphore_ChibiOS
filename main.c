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

#define FREQUENCY 16000000 / 255  
#define PERIOD    255

volatile int count = 0;

pwmcallback_t teste(PWMDriver *pwmp)
{
  palTogglePort(IOPORT2, PORTB_LED1);
}

//gptcallback_t gpt_t(GPTDriver *gptp)
//{
//
//}

/*
 * LED blinker thread, times are in milliseconds.
 */
//static THD_WORKING_AREA(waThread1, 32);
//static THD_FUNCTION(Thread1, arg) 
//{
//
//  (void)arg;
//  chRegSetThreadName("Blinker");
//  while (true) {
//    palTogglePad(IOPORT2, PORTB_LED1);
//    chThdSleepMilliseconds(100);
//  }
//}

/*
 * Application entry point.
 */
int main(void) 
{
  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  PWMConfig DrivePWMConfig = 
  {
    .frequency = FREQUENCY,
    .period    = PERIOD,
    .callback  = teste,
    .channels  = {{PWM_OUTPUT_ACTIVE_HIGH, 0}, {PWM_OUTPUT_DISABLED, 0}}
  };

  //GPTConfig gpt =
  //{
  //  .callback = NULL,
  //  .frequency = 1
  //};

  //gptStart(&GPTD1, &gpt);

  /*
   * Activates the serial driver 1 using the driver default configuration.
   */
  //sdStart(&SD1, &DriveSerial);

  /*
   * Starts the LED blinker thread.
   */
  //chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

  //chnWrite(&SD1, (const uint8_t *)"Hello World!\r\n", 14);

  //gptStartContinuous(&GPTD1, 10);

  pwmStart(&PWMD1, &DrivePWMConfig);
  pwmEnableChannel(&PWMD1, 0, 50);
  
  while (true) 
  {
    if (count == 1000)
    {
      pwmStop(&PWMD1);
    }
    //chThdSleepMilliseconds(1000);
  }
}
