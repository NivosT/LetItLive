#include "arduino_secrets.h"
#include "thingProperties.h"
#include <Arduino_MKRIoTCarrier.h>
#include <arduino-timer.h>

#define AirValue 1023
#define MoisterValue 700
#define MoisterPin A5
#define HOUR_IN_MILIS 3600000
MKRIoTCarrier carrier;

// Globals
int counter = 0;
int totalLight = 0;
float totalTemperature = 0;
float totalHumidity = 0;

//Enums
const int SHADOW_LOVING = 1;
const int BRIGHT_AND_INDIRECT = 2;
const int BRIGHT_LIGHT = 4;
const int STRONG_DIRECT = 8;
const int LOVES_WET_SOIL = 16; 
const int LOVES_MOIST_SOIL = 32;
const int LOVES_DRY = 64;

auto timer = timer_create_default();

//define the globals with the min/max values for moister & light
int minLight, maxLight, minMoist, maxMoist;


void setup() 
{
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500);
  while (!Serial);

  // Defined in thingProperties.h
  initProperties();
  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  //Get Cloud Info/errors , 0 (only errors) up to 4
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  //Wait to get cloud connection to init the carrier
  while (ArduinoCloud.connected() != 1) {
    ArduinoCloud.update();
    delay(500);
  }
  
  //TODO - temp init, to be removed when Niv fixes carrier menu
  setRanges(72);

  timer.every(HOUR_IN_MILIS, checkAndAlertValues);
  
  CARRIER_CASE = false; //TODO - to be changed when done
  carrier.begin();
}

void loop() 
{ 
  timer.tick();
  ArduinoCloud.update();

  delay(1000);
}

void resetVars()
/** Restes all variables used for calculating daily stats **/
{
  counter = 0;
  totalLight = 0;
  totalHumidity = 0;
  totalTemperature = 0;
}


bool checkAndAlertValues(void *)
/** This method is called every 1 hour - sample data, run calculation and alert/ update if needed**/
{
  sampleData();
  
  // content of the if- else should be changed to the actual action will do
  if(moistPrecentage > maxMoist)
  {
    Serial.print(moistPrecentage);
    Serial.println("\% - Too much water bro!");
  }
  else if(moistPrecentage < minMoist)
  {
    Serial.print(moistPrecentage);
    Serial.println("\% - Give me some water ASAP!");
  }
  
  updateAverageStats();
  
  return true; // needed for the timer thingy
}

void sampleData()
/** Samples data from sensors It is out to alow uses from different locations**/
{
  if (carrier.Light.colorAvailable())
  {
    int none;//not gonna be used
    carrier.Light.readColor(none,  none,  none, light);
  }

  temperature = carrier.Env.readTemperature();
  humidity = carrier.Env.readHumidity();

  int rawMoistValue = analogRead(MoisterPin);
  moistPrecentage = map(rawMoistValue, AirValue, MoisterValue, 0, 100);
}

void updateAverageStats()
/** Update avarege states**/
{
  if (counter < 24)
  {
    totalLight += light;
    totalTemperature += temperature;
    totalHumidity += humidity;
    counter++;
  }
  else if (counter == 24)
  {
    averageTemp = totalTemperature / counter;
    averageLight = totalLight / counter;
    averageHumidity = totalHumidity / counter;
    resetVars();
    //TODO - send some alert if not enough light in the last day?
  }
}

void onAverageTempChange() {
  // Do something
}

void onAverageLightChange() {
  // Do something
}

void onAverageHumidityChange() {
  // Do something
}

void setRanges(int carrier_input)
/** set the ranges of required light & soil moister **/
{
  int res = carrier_input & 15;
  switch(res)
  {
    case SHADOW_LOVING:
      minLight = 5000;
      maxLight = 10000;
      break;
      
    case BRIGHT_AND_INDIRECT:
      minLight = 10000;
      maxLight = 20000;
      break;
    
    case BRIGHT_LIGHT:
      minLight = 20000;
      maxLight = 45000;
      break;
      
    case STRONG_DIRECT:
      minLight = 45000;
      maxLight = INT_MAX;
      break;
  }
  
  res = carrier_input & 112;
  switch(res)
  {
    case LOVES_WET_SOIL:
      minMoist = 41;
      maxMoist = 80;
      break;
      
    case LOVES_MOIST_SOIL:
      minMoist = 41;
      maxMoist = 60;
      break;
      
    case LOVES_DRY:
      minMoist = 21;
      maxMoist = 40;
      break;
  }
}
