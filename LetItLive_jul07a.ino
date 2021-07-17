#include "arduino_secrets.h"
#include <Arduino_MKRIoTCarrier.h>
#include <Arduino_MKRIoTCarrier_Buzzer.h>
#include <Arduino_MKRIoTCarrier_Qtouch.h>
#include <Arduino_MKRIoTCarrier_Relay.h>

#include "thingProperties.h"
#include <arduino-timer.h>

// ArduinoOTA - Version: Latest 
#include <ArduinoOTA.h>

// Blynk - Version: Latest 
#include <Blynk.h>
#include <SPI.h>

//extra Blynk libs
#include <WiFiNINA.h>
#include <BlynkSimpleWiFiNINA.h>

#define AirValue 1023
#define MoisterValue 700
#define MoisterPin A5
#define HOUR_IN_MILIS 3600000
#define MIN_IN_MILIS 60000
#define INT_MAX 2147483647


MKRIoTCarrier carrier;

// Globals
int counter = 0;
int totalLight = 0;
float totalTemperature = 0;
float totalHumidity = 0;
int displayState = -2;
int moistState = 0;
int lightState = 0;

int page = 0;

//Enums
const int LIGHT_CONFIG = -1;
const int SHADOW_LOVING = 0;
const int BRIGHT_AND_INDIRECT = 1;
const int BRIGHT_LIGHT = 2;
const int STRONG_DIRECT = 3;

const int MOIST_CONFIG = -1;
const int LOVES_WET_SOIL = 0; 
const int LOVES_MOIST_SOIL = 1;
const int LOVES_DRY = 2;

const int DISPLAY_CONFIG_LIGHT = -2;
const int DISPLAY_CONFIG_MOIST = -1;
const int DISPLAY_OFF = 0;
const int DISPLAY_ON = 1;
const int DISPLAY_DEFAULT = 0;
const int DISPLAY_LIGHT = 1;
const int DISPLAY_MOIST = 2;

const int DEFAULT_PAGE = 0;
const int TEMP_PAGE = 1;
const int HUMIDITY_PAGE = 2;
const int MOIST_PAGE = 3;
const int LIGHT_PAGE = 4;

int buttonState = 0; //0- configLight; 1- configMoist, 2- finshed config
bool allowScreenOff = false;
auto timer = timer_create_default();

//define the globals with the min/max values for moister & light
int minLight, maxLight, minMoist, maxMoist;

char blynkAuthToken[] = "LdJJ6g7tCxAxc2Bz8GVQ5OYtK2ujBdxP";

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
  
  displayState = DISPLAY_CONFIG_LIGHT;

  timer.every(MIN_IN_MILIS, checkAndAlertValues);
  
  CARRIER_CASE = true; //TODO - to be changed when done
  // Blynk.begin(blynkAuthToken, SSID, PASS, IPAddress(3,9,144,235), 8080);
  carrier.begin();
  sampleData();
}

void loop() 
{ 
  carrier.Buttons.update();
  timer.tick();
  ArduinoCloud.update();
  //TODO - delete
  // Serial.println(buttonState);
  
  if(allowScreenOff && carrier.Button1.getTouch())
  {
    toggleDisplayState();
  }
  else
  {
    switch(buttonState)
    {
      case 0 :
        if (carrier.Button2.getTouch()) 
        {
          toggleLightConfig();
        }
        else if (carrier.Button3.getTouch())
        {
          buttonState = 1;
          toggleMoistConfig();
        }
        else
        {
          // printMessage("Light config:\n  2-next\n  3- accept");
        }
        break;
    
      case 1:
        if (carrier.Button2.getTouch()) 
        {
          toggleMoistConfig();
        }
        else if (carrier.Button3.getTouch()) 
        {
          buttonState = 2;
          allowScreenOff = true;
          // start showing info
        }
        else
        {
        // printMessage("Moist config:\n  2-next\n  3- accept");
        }
        break;
  
      case 2:
        if (carrier.Button2.getTouch())
        {
          buttonState = 0;
          allowScreenOff = false;
        }
        else
        {
          displayPage(NULL);
        }
        
        break;
      }
  }
  
  
    
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

void setRanges()
/** set the ranges of required light & soil moister **/
{
  switch(lightState)
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
  
  switch(moistState)
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

void displayPage(char msg[]) {
  switch (displayState) {
    case DISPLAY_ON:
      switch (page) {
        case TEMP_PAGE:
          printValue("Temp", temperature, DISPLAY_DEFAULT);
          break;
        case HUMIDITY_PAGE:
          printValue("Humi", humidity, DISPLAY_DEFAULT);
          break;
        case MOIST_PAGE:
          printValue("Moist", moistPrecentage, DISPLAY_MOIST);
          break;
        case LIGHT_PAGE:
          printValue("Light", light, DISPLAY_LIGHT);
          break;
        default:
          break;
      }
      page = (page + 1) % 6;
      break;
      
    case DISPLAY_OFF:
      printMessage(msg, false);
      break;
    }
}

void printMessage(char *msg, bool isMoistOrLight) {
  carrier.display.fillScreen(ST77XX_BLACK); 
  carrier.display.setTextColor(ST77XX_WHITE);
  carrier.display.setTextSize(2); //medium sized text
  int x = strlen(msg);
  char *token = strtok(msg, "\n");
  if(isMoistOrLight)
  {
    carrier.display.setCursor(48, 50);
    carrier.display.println("Next - click 2");
    carrier.display.setCursor(28, 70);
    carrier.display.println("Confirm - click 3");
    x = strlen(token);
    carrier.display.setCursor(120 - x/2, 110);
    Serial.println(token);
    carrier.display.println(token);
    msg = strtok(NULL, "\n");
    Serial.println(msg);
    x = strlen(msg);
    // Serial.println(msg)
    
  }
  carrier.display.setCursor(120 - x/2, 140); //sets position for printing (x and y)
  carrier.display.print(msg);
}

void printValue(char header[], int val, int displayMode) {
  int min, max;
  
  switch(displayMode) {
    case DISPLAY_LIGHT:
      min = minLight;
      max = maxLight;
      break;
    
    case DISPLAY_MOIST:
      min = minMoist;
      max = maxMoist;
      break;
      
    default:
      min = -1;
      max = INT_MAX;
      break;
  }
  
  if (min > val || max < val) {
    //configuring display, setting background color, text size and text color
    carrier.display.fillScreen(ST77XX_RED); 
    carrier.display.setTextColor(ST77XX_WHITE); //white text
  }
  else {
    carrier.display.fillScreen(ST77XX_GREEN); 
    carrier.display.setTextColor(ST77XX_BLACK); //black text
  }
  
  carrier.display.setTextSize(2); //medium sized text
  carrier.display.setCursor(20, 110); //sets position for printing (x and y)
  carrier.display.print(header);
  carrier.display.print(": ");
  carrier.display.println(val);
  
  delay(1000);
}

void toggleDisplayState() {
  char *msg;
  if (displayState == DISPLAY_ON){
    msg = "Display off..";
    page = 0;
  }
  else {
    msg = "Display on..";
    page = 1;
  }
  displayState = ((displayState + 1) % 2); 
  printMessage(msg, false);
}

void toggleMoistConfig() {
  moistState = (moistState + 1) % 3;
  setRanges();
  char out[1024] = "Set moist to:\n";
  
  printMessage(strcat(out, getMoistStr()), true);
  if (displayState < 0) {
    displayState = 1;
  }
}

void toggleLightConfig() {
  lightState = (lightState + 1) % 4;
  setRanges();
  char out[1024] = "Set light to:\n";
  printMessage(strcat(out, getLightStr()), true);
  if (displayState < 0) {
    displayState = -1;
  }
}

char* getMoistStr() {
  char* res;
  switch(moistState) {
    case LOVES_WET_SOIL:
      res = "Wet Soil\n";
      break;
    case LOVES_MOIST_SOIL:
      res = "Moist Soil\n";
      break;
    case LOVES_DRY:
      res = "Dry Soil\n";
      break;
  }
  
  return res;
}

char* getLightStr()
{
  char* res;
  switch(lightState) {
    case SHADOW_LOVING:
      res = "Shadowed\n";
      break;
    case BRIGHT_AND_INDIRECT:
      res = "Bright & Indirect\n";
      break;
    case BRIGHT_LIGHT:
      res = "Bright light\n";
      break;
    case STRONG_DIRECT:
      res = "Direct Light\n";
      break;
  }
  
  return res;
}

