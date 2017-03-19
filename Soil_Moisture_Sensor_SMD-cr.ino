/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * Code and ideas from 
 * mfalkvidd (http://forum.mysensors.org/user/mfalkvidd), 
 * carlierd (https://www.openhardware.io/user/97/projects/carlierd)
 * Scalz (https://www.openhardware.io/user/12/projects/scalz)
 */

/**************************************************************************************/
/* Moisture sensor SMD.                                                               */
/*                                                                                    */
/* Version     : V1.1.2                                                               */
/* Supported Hardware     : V1.1.x                                                    */
/* Date        : 19/03/2017                                                           */
/* Modified by : Jordan Bouey                                                         */
/**************************************************************************************/

 /* Fuse L:E2 H:DA E:06 x
  *  BOD 1.8V - Int 8Mhz Osc 
  *  avec boot  ATmegaBOOT_168_atmega328_pro_8MHz.hex
  */

//#define MY_NODE_ID 12

//#define MY_DEBUG    // Enables debug messages in the serial log
//#define MY_DEBUG_VERBOSE_SIGNING

//Enable this if you want to use this node without controler and set MOISTURE_WARN_PCNT != 0
//#define MY_TRANSPORT_WAIT_READY_MS 5000 //Set for use this node in standalone mode = no need a controler for run (need library mysensors >= 2.1.0) 

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#defin MY_OTA //Enables smart sleep if you use OTA
//#define MY_OTA_FIRMWARE_FEATURE  // Enables OTA firmware updates if DualOptiBoot
// Enables software signing
//#define MY_SIGNING_SOFT
// SETTINGS FOR MY_SIGNING_SOFT
//#define MY_SIGNING_SOFT_RANDOMSEED_PIN 7  //!< Unconnected analog pin for random seed
// Enable node whitelisting
//#define MY_SIGNING_NODE_WHITELISTING {{.nodeId = 0,.serial = {0xEB,0x86,0x43,0x73,0x14,0xB1,0xAF,0x0E,0x17}}} 
// Enable this if you want destination node to sign all messages sent to this node. 
// If enabled, You need to use securityPersonalizer for more security and change the default HMAC_KEY (https://www.mysensors.org/about/signing)
//#define MY_SIGNING_REQUEST_SIGNATURES



#include <MySensors.h>
#include <SPI.h>

//Define functions
#define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#define N_ELEMENTS(array) (sizeof(array)/sizeof((array)[0]))

//Constants for MySensors
#define SKETCH_NAME           "Moisture Sensor SMD"
#define SKETCH_VERSION        "1.1.2"


#define CHILD_ID_VOLTAGE      0 // Id of the sensor child (default 0)
#define CHILD_ID_MOISTURE     1   // Id of the sensor child (default 1)
#define BATTERY_SENSE_PIN     A0  // select the input pin for the battery sense point (default A0)
#define LED_PIN_INFO          8 //INFO LED PIN (default 8)
#define LED_PIN_WARN          3 //WARNING LED PIN (default 3)
#define MOISTURE_WARN_PCNT    0 //MOISTURE WARNING PERCENT (default 0) 0 = disabled
#define BATTERY_WARN_PCNT     10 //BATTERY_WARN_PCNT WARNING PERCENT (default 10) 0 = disabled
#define THRESHOLD             1.1     // Only make a new reading with reverse polarity if the change is larger than 10% (default 1.1)
#define MOISTURE_THRESHOLD    1    // Delta needing for sending moisture to the controler in percent (default 1)
#define STABILIZATION_TIME    2000    // Let the sensor stabilize before reading (default 2000)
#define BATTERY_FULL          3240    // when full CR123A (default 3240)
#define BATTERY_ZERO          2340    // 2.34V limit for 328p at 8MHz (set extended fuse to 0x06 for BOD 1.8V or you'll never see 2.34V)  (default 2340)
const int SENSOR_ANALOG_PINS[] = {A1, A2}; //(default {A1, A2})

#ifdef MY_DEBUG
 #define MEASURE_INTERVAL      10000 // Minimal Measure interval even if sleep time is smaller (default 1000)(10 sec)
 #define WARN_TIME             5000 //WARNING REPEAT TIME (in milliseconds) (default 5000)(5sec)
#else
 #define MEASURE_INTERVAL      3600000 // Minimal Measure interval even if sleep time is smaller (default 3600000)(1 hour)
 #define WARN_TIME             180000 //WARNING REPEAT TIME (in milliseconds) (default 180000)(3min)
#endif



//Variables
int oldbatteryPcnt = -1;
byte direction = 0;
int oldMoistureLevel = -1;
int oldMoisturePcnt = -1;
int moisturePcnt = -1;
int batteryPcnt = -1;
long sleepTime = MEASURE_INTERVAL;
long lastMeasure = MEASURE_INTERVAL;

MyMessage msgVolt(CHILD_ID_VOLTAGE, V_VOLTAGE);
MyMessage msgMoisture(CHILD_ID_MOISTURE, V_HUM);


/**************************************************************************************/
/* Presentation                                                                       */
/**************************************************************************************/
void presentation()  
{ 
  //Get time (for presentation)
  #ifdef MY_DEBUG
    unsigned long startTime = millis();
  #endif

  //Start MySensors and send the sketch version information to the gateway
  sendSketchInfo(SKETCH_NAME, SKETCH_VERSION);

  //Register all sensors
  present(CHILD_ID_VOLTAGE, S_MULTIMETER);
  present(CHILD_ID_MOISTURE, S_MOISTURE);

   //Setup LED INFO pin
   pinMode(LED_PIN_INFO, OUTPUT);
     
  //Setup done !
  blinkLedFastly(3, LED_PIN_INFO);

    //Print setup debug
  #ifdef MY_DEBUG
    int duration = millis() - startTime;
    Serial.print("[Presentation duration: "); Serial.print(duration, DEC); Serial.println(" ms]");
  #endif
}
/**************************************************************************************/
/* Initialization                                                                     */
/**************************************************************************************/
void setup()  
{ 

  //Setup LED WARNING pin
   pinMode(LED_PIN_WARN, OUTPUT);

  //Set moisutre sensor pins
  for (int i = 0; i < N_ELEMENTS(SENSOR_ANALOG_PINS); i++)
    {
    pinMode(SENSOR_ANALOG_PINS[i], OUTPUT);
    digitalWrite(SENSOR_ANALOG_PINS[i], LOW);
    }
  
}

/**************************************************************************************/
/* Main loop                                                                          */
/**************************************************************************************/
void loop()     
  {
  //Get time (for a complete loop)
  #ifdef MY_DEBUG
    unsigned long startTime = millis();
  #endif
  
    #ifdef MY_DEBUG
    Serial.print("lastMeasure: ");
    Serial.println(lastMeasure);
    #endif
    
  if(lastMeasure >= MEASURE_INTERVAL) {
    //Get moisture level
    int moistureLevel = readMoisture();
  
    //Send rolling average of 2 samples to get rid of the "ripple" produced by different resistance in the internal pull-up resistors
    //See http://forum.mysensors.org/topic/2147/office-plant-monitoring/55 for more information
  
    //Verify if current measurement is not too far from the previous one
    if (moistureLevel > (oldMoistureLevel * THRESHOLD) || moistureLevel < (oldMoistureLevel / THRESHOLD))
      {
      //The change was large, so it was probably not caused by the difference in internal pull-ups.
      //Measure again, this time with reversed polarity.
      moistureLevel = readMoisture();
      }
  
  
    #ifdef MY_DEBUG
    Serial.print("Soil Value: ");
    Serial.println(moistureLevel);
    #endif
     
    if (oldMoistureLevel == -1) {
       oldMoistureLevel = moistureLevel;    
    }
  
   // int moisturePcnt = (moistureLevel + oldMoistureLevel) / 2.0 / 10.23;
      moisturePcnt = moistureLevel / 10.23;
    if (moistureLevel >= (oldMoistureLevel +  10.23 * MOISTURE_THRESHOLD) || moistureLevel <= (oldMoistureLevel - 10.23 * MOISTURE_THRESHOLD) || oldMoisturePcnt == -1) //Send moisture only if moisture changed more than MOISTURE_THRESHOLD (control change value percent and not a cast)
    {   
     if (moisturePcnt >= (oldMoisturePcnt +  MOISTURE_THRESHOLD) || moisturePcnt <= (oldMoisturePcnt - MOISTURE_THRESHOLD) || oldMoisturePcnt == -1) //Send moisture only if moisture percent changed more than MOISTURE_THRESHOLD (control if display changed)
      {
         send(msgMoisture.set(moisturePcnt)); 
         blinkLedFastly(1, LED_PIN_INFO);       
        //Store current moisture pcnt
        oldMoisturePcnt = moisturePcnt;
        //Store current moisture level 
        oldMoistureLevel = moistureLevel;
        
      }  
    }
  }

//LED WARNING MOISTURE
if(moisturePcnt < MOISTURE_WARN_PCNT)
{
 if( WARN_TIME < MEASURE_INTERVAL){ sleepTime = WARN_TIME; }
 blinkLedFastly(1, LED_PIN_WARN);
}

if(lastMeasure >= MEASURE_INTERVAL) {
    //Report data to the gateway
    long voltage = getVoltageByHard();  
    batteryPcnt = round((voltage - BATTERY_ZERO) * 100.0 / (BATTERY_FULL - BATTERY_ZERO));
   
   if (batteryPcnt > 100) {batteryPcnt = 100;}
   if (batteryPcnt <= 0) {batteryPcnt = 0;}          
   if (oldbatteryPcnt != batteryPcnt) {
      send(msgVolt.set(voltage / 1000.0, 2));
      sendBatteryLevel(batteryPcnt);
     oldbatteryPcnt = batteryPcnt;

   }
}

//LED WARNING BATTERY
if(batteryPcnt < BATTERY_WARN_PCNT)
{
 if( WARN_TIME < MEASURE_INTERVAL){ sleepTime = WARN_TIME; }
 blinkLedFastly(3, LED_PIN_WARN);
}


if(lastMeasure >= MEASURE_INTERVAL) {
 lastMeasure = sleepTime;
}else{
 lastMeasure = lastMeasure + sleepTime;
}


   //Print debug
  #ifdef MY_DEBUG
    Serial.print(moisturePcnt);
    Serial.print("%");
    Serial.print("   ");
    Serial.print(batteryPcnt);
    Serial.print("%");
    int duration = millis() - startTime;
    Serial.print("   ");
    Serial.print("["); Serial.print(duration, DEC); Serial.println(" ms]");
    Serial.flush();
  #endif 
  
 //Sleep until next measurement - smart sleep for OTA
  #ifdef MY_OTA
    smartSleep(sleepTime);
  #else
    sleep(sleepTime);
  #endif
  }


/**************************************************************************************/
/* Allows to get moisture.                                                            */
/**************************************************************************************/
int readMoisture()
  {
  //Power on the sensor and read once to let the ADC capacitor start charging
  pinMode(SENSOR_ANALOG_PINS[direction], INPUT_PULLUP);
  analogRead(SENSOR_ANALOG_PINS[direction]);
  
  //Stabilize and read the value
  sleep(STABILIZATION_TIME);
  int moistureLevel = (1023 - analogRead(SENSOR_ANALOG_PINS[direction]));

  //Turn off the sensor to conserve battery and minimize corrosion
  pinMode(SENSOR_ANALOG_PINS[direction], OUTPUT);
  digitalWrite(SENSOR_ANALOG_PINS[direction], LOW);

  //Make direction alternate between 0 and 1 to reverse polarity which reduces corrosion
  direction = (direction + 1) % 2;
  return moistureLevel;
  }
  
/**************************************************************************************/
/* Allows to fastly blink the LED.                                                    */
/**************************************************************************************/
void blinkLedFastly(byte loop, byte pinToBlink)
  {
  byte delayOn = 150;
  byte delayOff = 150;
  for (int i = 0; i < loop; i++)
    {
    blinkLed(pinToBlink, delayOn);
    delay(delayOff);
    }
  }


/**************************************************************************************/
/* Allows to blink a LED.                                                             */
/**************************************************************************************/
void blinkLed(byte pinToBlink, int delayInMs)
  {
  digitalWrite(pinToBlink,HIGH);
  delay(delayInMs);
  digitalWrite(pinToBlink,LOW);
  }

/**************************************************************************************/
/* Allows to get the real Vcc (return value in mV).                                   */
/* http://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/   */
/**************************************************************************************/
long getVoltageBySoft()
  {
  ADMUX = (0<<REFS1) | (1<<REFS0) | (0<<ADLAR) | (1<<MUX3) | (1<<MUX2) | (1<<MUX1) | (0<<MUX0);
  delay(50);  // Let mux settle a little to get a more stable A/D conversion
  //Start a conversion  
  ADCSRA |= _BV( ADSC );
  //Wait for it to complete
  while (bit_is_set(ADCSRA, ADSC));

  //Compute and return the value
  uint8_t low  = ADCL;                  // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH;                  // unlocks both
  long result = (high << 8) | low;
  result = 1125300L / result;           // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result;                        // Vcc in millivolts
  }

  
/**************************************************************************************/
/* Mesure battery with divider                                                        */
/*                                                                                    */
/**************************************************************************************/
long getVoltageByHard()
  {
    analogReference(INTERNAL);

   // get the battery Voltage
   int sensorValue = analogRead(BATTERY_SENSE_PIN);
   #ifdef MY_DEBUG
   Serial.print("Battery Value: ");
   Serial.println(sensorValue);
   #endif
   
      //L'analog reférence ne change que lorsque l'on fait un analogread et il met un peu de temps à changer, d'où la tempo et la répétition de la commande analogRead
      delay(1000);     // this delay is needed
      sensorValue = analogRead(BATTERY_SENSE_PIN);


   #ifdef MY_DEBUG
   Serial.print("Battery Value: ");
   Serial.println(sensorValue);
   #endif
   
   // 1M, 470K divider across battery and using internal ADC ref of 1.1V
   // Sense point is bypassed with 0.1 uF cap to reduce noise at that point
   // ((1e6+470e3)/470e3)*1.1 = Vmax = 3.44 Volts
   // 3.44/1023 = Volts per bit = 0.003363075

    float batteryV  = sensorValue * 0.003363075;
    int batteryMv  = round(batteryV * 1000);
   // int batteryPcnt = sensorValue * 0.0977517106549365; 

  int batteryPcnt = round((batteryMv - BATTERY_ZERO) * 100.0 / (BATTERY_FULL - BATTERY_ZERO));
  if (batteryPcnt > 100) {batteryPcnt = 100;}
  
   #ifdef MY_DEBUG
  
   Serial.print("Battery Voltage: ");
   Serial.print(batteryMv);
   Serial.println(" mV");

   Serial.print("Battery percent: ");
   Serial.print(batteryPcnt);
   Serial.println(" %");
   #endif

   analogReference(DEFAULT);
  return batteryMv;                        // Vcc in mV
  }
