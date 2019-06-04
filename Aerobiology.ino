// Aerobiology project
// Author: Andrew Van Gerpen

#include <SD.h>
#include <SPI.h>
#include <UbloxGPS.h> //Ublox module
#include <XBee.h>
#include <RelayXBee.h>
#include <TinyGPS++.h> 
#include <OneWire.h>
#include <DallasTemperature.h>

//Pin declarations
#define xBee_Serial Serial5
#define gps_Serial Serial4

#define sdLED_pin 29
#define switchLED_pin 25
#define chipSelect_pin BUILTIN_SDCARD
#define switch_pin 37  // make sure these pins are correct preflight
#define reset_pin 39 // *********************************

//variable initialization
String ID = "BIO";
double minRange = 15000.0; //ballpark estimates subject to change
double maxRange = 90000.0; // *********************************
double GPSaltitude = 0;
double filteredAltitude = 0;
double projectedAlt = 0;
boolean ASCENDING = true;
boolean switchStatus = false; // FALSE BEFORE LAUNCH
boolean resetStatus = true; // TRUE BEFORE LAUNCH
double altArr[10];
int altArrLocation = 0;
int checkArrLocation = 0;
double avgRate;
double checkArr[10];
double maximum;
double minimum;
int descentCounter = 0; 
double noFixCounter = 0;
double PROJECTION = 0;
double noFixProjection = 0; 
boolean fix = false;
double timerr;
unsigned long int ledTime;

boolean initialFix = false; // first fix is when projections will begin

//each device has its own setup function
void setup() {
  SDsetup();
  sensorSetup();
  pinMode(switchLED_pin,OUTPUT);
  pinMode(reset_pin, OUTPUT);
  pinMode(switch_pin, OUTPUT);
  initializeProjectedAlt();
  SWITCH();
}

//alternate between logging sensor data and checking for xBee commands
void loop() { 
  updateSensors();
  updateValve();
  UpdateXbee();
} //makes sure ten minutes have passed before thinking about opening **not yet**

void updateValve()
{
  basicBlink();
  if(millis()-timerr>1000){ // 7 minutes switch
  
  GPSaltitude = getGPSaltitude();
  timerr = millis();
  filteredAltitude = GPSfilter(GPSaltitude);
  
  if (switchStatus == true){
    if(((filteredAltitude!=0.0 && filteredAltitude < minRange) || filteredAltitude > maxRange) || ASCENDING == false || millis()>8100000){  //7200000 millis = 120 mins
      switchStatus = false;
      resetStatus = true;
      SWITCH();
      return;}
  }
  if(switchStatus == false) {
    if ((filteredAltitude > minRange && filteredAltitude < maxRange && ASCENDING == true) || (millis()>10000 && millis()<4200000)){
      switchStatus = true;
      resetStatus = false;
      Serial.println("SWITCHING ON");
      SWITCH();
      return;
    }
  }
  }
}
double GPSfilter(double alt){
  descentCheck();
  if(fix == false){
    if(noFixCounter == 0){noFixCounter == altArrLocation;}
    altArrLocation = 0;
    if(ASCENDING == true){noFixProjection = (projectedAlt + (avgRate*(noFixCounter+1)));}
    else{noFixProjection = (projectedAlt - (avgRate*(noFixCounter+1)));}
    noFixCounter++;
    return noFixProjection;
  }

  if(fix == true){    
  projectedAlt = altitudeProjection();
  
  if((alt > 300.0+projectedAlt || alt < projectedAlt-300.0)) //change the range of acceptable altitudes if necessary
  {
    checkArr[checkArrLocation] = alt; 
    checkArrLocation++;
    if(checkArrLocation >= 10){
      for(int i=0; i<10; i++){altArr[i] = checkArr[i];}
      newMaxMin();
      descentCheck();
      projectedAlt = altitudeProjection();
      checkArrLocation = 0;
    }
    return projectedAlt; 
  }                      // if this is done then the projected alt will stay the same while the payload is descending and thus check array will fill fast
  else{
    altArr[altArrLocation] = alt;      
    altArrLocation++;   
    if(altArrLocation >= 10 || checkArrLocation>0){newMaxMin();}
    clearCheckArr();     
    return projectedAlt;}
}}  

void clearCheckArr()
{
  for(int i=0; i<10; i++){checkArr[i]=0;}
  checkArrLocation = 0;
}

double altitudeProjection()
{
  double locationDouble = (double)altArrLocation + (double)checkArrLocation + 1.0;
  if(noFixCounter != 0){
    noFixCounter++;
    if(ASCENDING == true){PROJECTION = (noFixProjection + (locationDouble*avgRate));}
    if(ASCENDING == false){PROJECTION = (noFixProjection - (locationDouble*avgRate));}}
  else{
  if(ASCENDING == true){PROJECTION =  (maximum + avgRate*locationDouble);}   //rate of altitude ascension + maximum for a projection
  if(ASCENDING == false){PROJECTION = (minimum - avgRate*locationDouble);}}
  
  return PROJECTION;//rate of altitude descension - minimum for a projection  
}

void newMaxMin()
{
    maximum = 0.0;
    minimum = 180000.0;
    for(int i=0; i<10; i++){
      if(altArr[i] > maximum){maximum = altArr[i];}
      if(altArr[i] < minimum){minimum = altArr[i];}}
    avgRate = ((maximum-minimum)/(9.0+(double)checkArrLocation));
    altArrLocation = 0;
    noFixCounter = 0;
}

void descentCheck()
{
  descentCounter = 0;
  for(int i=1; i<10; i++){
    if(altArr[i]-altArr[i-1] < 0){descentCounter++;}}
  if(descentCounter > 7){
    ASCENDING = false;
    }
}

void SWITCH() // mechanical switch stays in position it is told so keeping pins high is not necessary
{
  digitalWrite(switch_pin, switchStatus);
  digitalWrite(reset_pin, resetStatus);
  delay(100);
  digitalWrite(switch_pin, LOW);
  digitalWrite(reset_pin, LOW);
}

void initializeProjectedAlt()
{
  for(int i=0; i<10; i++){
  altArr[i]=getGPSaltitude();
  newMaxMin();
  Serial.println(altArr[i]);}
}
double getFilteredAltitude() {return filteredAltitude;}

void basicBlink(){
  if(switchStatus == true && fix == true)
  {
    if(millis()-ledTime > 200){digitalWrite(switchLED_pin, HIGH);}
    if(millis()-ledTime > 1500)
    {digitalWrite(switchLED_pin,LOW);
    ledTime = millis();}
  }
  if(switchStatus == false && fix == false)
  {
    if(millis()-ledTime > 200){digitalWrite(switchLED_pin, HIGH);}
    if(millis()-ledTime > 400)
    {digitalWrite(switchLED_pin,LOW);
    ledTime = millis();}
  }
  if(switchStatus == false && fix == true)
  {
    if(millis()-ledTime > 1400){digitalWrite(switchLED_pin, HIGH);}
    if(millis()-ledTime > 1500)
    {digitalWrite(switchLED_pin,LOW);
    ledTime = millis();}
  }
  if(switchStatus == true && fix == true)
  {
    if(millis()-ledTime > 2000){digitalWrite(switchLED_pin, HIGH);}
    if(millis()-ledTime > 4000)
    {digitalWrite(switchLED_pin,LOW);
    ledTime = millis();}
  }
}
