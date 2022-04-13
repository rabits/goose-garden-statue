#include <ArduinoLowPower.h>
#include "Fuzzy_DAC_Audio.h"

//#include "fart_sound.h"
#include "honk_01.h"
#include "honk_02.h"
#include "honk_03.h"
#include "honk_04.h"
#include "honk_05.h"
#include "honk_honk.h" // 06

#define PIN_PIR 10
#define PIN_AUDIO_SHUTDOWN 2

FuzzyDACAudio audio;

volatile int wakeups = 0;

// Configure the Watchdog Timer on SAMD21
void configureWdt() {
  // Set up the generic clock (GCLK2) used to clock the watchdog timer at 1.024kHz
  REG_GCLK_GENDIV = GCLK_GENDIV_DIV(4) |            // Divide the 32.768kHz clock source by divisor 32, where 2^(4 + 1): 32.768kHz/32=1.024kHz
                    GCLK_GENDIV_ID(2);              // Select Generic Clock (GCLK) 2
  while (GCLK->STATUS.bit.SYNCBUSY);                // Wait for synchronization

  REG_GCLK_GENCTRL = GCLK_GENCTRL_DIVSEL |          // Set to divide by 2^(GCLK_GENDIV_DIV(4) + 1)
                     GCLK_GENCTRL_IDC |             // Set the duty cycle to 50/50 HIGH/LOW
                     GCLK_GENCTRL_GENEN |           // Enable GCLK2
                     GCLK_GENCTRL_SRC_OSCULP32K |   // Set the clock source to the ultra low power oscillator (OSCULP32K)
                     GCLK_GENCTRL_ID(2);            // Select GCLK2         
  while (GCLK->STATUS.bit.SYNCBUSY);                // Wait for synchronization

  // Feed GCLK2 to WDT (Watchdog Timer)
  REG_GCLK_CLKCTRL = GCLK_CLKCTRL_CLKEN |           // Enable GCLK2 to the WDT
                     GCLK_CLKCTRL_GEN_GCLK2 |       // Select GCLK2
                     GCLK_CLKCTRL_ID_WDT;           // Feed the GCLK2 to the WDT
  while (GCLK->STATUS.bit.SYNCBUSY);                // Wait for synchronization

  REG_WDT_CONFIG = WDT_CONFIG_PER_16K;              // Set the WDT reset timeout to 1 second
  while(WDT->STATUS.bit.SYNCBUSY);                  // Wait for synchronization
  REG_WDT_CTRL = WDT_CTRL_ENABLE;                   // Enable the WDT in normal mode
  while(WDT->STATUS.bit.SYNCBUSY);                  // Wait for synchronization
}

void tickWdt() {
  if( !WDT->STATUS.bit.SYNCBUSY ) {             // Check if the WDT registers are synchronized
    REG_WDT_CLEAR = WDT_CLEAR_CLEAR_KEY;        // Clear the watchdog timer
  }
}

void sleepChange() {
  if( digitalRead(PIN_PIR) ) {
    wakeups++;
    digitalWrite(LED_BUILTIN, LOW);
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

void setup() {
  Serial.begin(9600);
  
  // Set PIR input: HIGH - motion, LOW - no motion
  pinMode(PIN_PIR, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  audio.begin();
  audio.setShutdownPin(PIN_AUDIO_SHUTDOWN);
  // To debug audio
  //pinMode(PIN_AUDIO_SHUTDOWN, OUTPUT);
  //digitalWrite(PIN_AUDIO_SHUTDOWN, HIGH);

  LowPower.attachInterruptWakeup(PIN_PIR, sleepChange, CHANGE);

  delay(2000);
  Serial.println("Wait 8 sec for firmware update...");
  delay(8000);
  // To test audio - make sure the battery is charged - otherwise it
  // will eat it during playback and will not be able to boot which is
  // crucial if you using the solar power harvester board 
  //audio.playHuffArray(fart_huffman, fart_sounddata_bits, fart_sounddata);

  randomSeed(analogRead(1));

  // Enable WDT to reset in case power will be interrupted
  configureWdt();
}

void loop() {
  tickWdt();
  LowPower.sleep(14000);
  tickWdt();

  delay(500);

  // In case there is no movement - skip
  if( !digitalRead(PIN_PIR) )
    return;
  
  bool detected = true;

  // Continuously check for 5 sec to filter out the noise
  for( uint8_t i = 0; i<=50; i++ ) {
    delay(100);
    if( !digitalRead(PIN_PIR) ) {
      detected = false;
      break;
    }
  }

  // Honk if the movement was detected for a long time
  if( detected ) {
    tickWdt();
    Serial.printf("Long movement detected, wakeup %d\n", wakeups);

    uint8_t rand_number = random(1, 6);
    switch( rand_number ) {
      case 1:
        audio.playHuffArray(honk_01_huffman, honk_01_sounddata_bits, honk_01_sounddata);
        break;
      case 2:
        audio.playHuffArray(honk_02_huffman, honk_02_sounddata_bits, honk_02_sounddata);
        break;
      case 3:
        audio.playHuffArray(honk_03_huffman, honk_03_sounddata_bits, honk_03_sounddata);
        break;
      case 4:
        audio.playHuffArray(honk_04_huffman, honk_04_sounddata_bits, honk_04_sounddata);
        break;
      case 5:
        audio.playHuffArray(honk_05_huffman, honk_05_sounddata_bits, honk_05_sounddata);
        break;
      case 6:
        audio.playHuffArray(honk_06_huffman, honk_06_sounddata_bits, honk_06_sounddata);
        break;
      default:
        audio.playHuffArray(honk_01_huffman, honk_01_sounddata_bits, honk_01_sounddata);
        break;
    }

    // The amplifier takes quite alot of power so going to deep sleep to
    // recharge capacitors and supercap on output and not to honk too often
    tickWdt();
    delay(10000);
    tickWdt();
    delay(10000);
  }
}
