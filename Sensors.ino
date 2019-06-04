
UbloxGPS gps = UbloxGPS(&gps_Serial);
RelayXBee xBee = RelayXBee(&xBee_Serial, ID);
unsigned long timer = 0;
byte counter = 0;

#define ONE_WIRE_BUS 36

OneWire oneWire1(ONE_WIRE_BUS);
DallasTemperature sensor1(&oneWire1);

String data;                 
double x = 0;
String commandd;
float temp;

void sensorSetup() {
  sensor1.begin();
  xBee_Serial.begin(XBEE_BAUD);
  xBee.init('A');
  pinMode(13,OUTPUT);     //teensy onboard LED;
  digitalWrite(13,HIGH); 
  gps_Serial.begin(UBLOX_BAUD);
  gps.init();
  delay(50);
  gps.setAirborne();
  byte i=0;
  while (i < 3) { 
    i++;
    if (gps.setAirborne()) {
      datalog.println("Air mode successfully set.");
      Serial.println("Air mode successfully set.");
      break;
    }}
  String header = "GPS Time,Lat,Lon,Alt (m),# Sats, Ascending??, Filtered Altitude, Switch Status, temperature, seconds since bootup ";  //this file goes at top of datalog
  logData(header);
 }

//called repeatedly during loop()
void updateSensors() {
  while(gps_Serial.available()){gps.update();}
  
  //once per second, log gps and sensor data
  if (millis() - timer > 1000) { //change as the blink sequence will now take up this time
    timer = millis();
    counter++;
    
     data = String(gps.getHour()-5) + ":" + String(gps.getMinute()) + ":" + String(gps.getSecond()) + ", "
                + String(gps.getLat(), 4) + ", " + String(gps.getLon(), 4) + ", " + String(getGPSaltitude()) + ", "
                + String(gps.getSats()) + ", " + String(ASCENDING) + ", " + String(getFilteredAltitude()) + ", " 
                + String(switchStatus) + ", " + String(temperature()) + ", " + String(millis());

    if(gps.getFixAge() > 2000){
      data += "   No Fix,";
      fix = true;
      }
    else
    {
      data += "   Fix,";
      fix = true;
    }
    logData(data);}}

int fixTime(){gps.getFixAge();}

double getGPSaltitude(){
  return gps.getAlt_feet();
  //return fakeClimb();
}

String xbeeData()
{
  return data;
}
double fakeClimb()
{
if(millis() < 20000 || (millis() > 25000 && millis() < 35000)){
    initialFix = true;
    x=300.0;
    x+= millis()*0.005;
    fix = true;   
  }
  if(millis() > 20000 && millis() < 25000){
    x=10000;
    fix = true;
  }
  if(millis()>35000){
    x=650.0;
    x-= millis()*0.005;
    }
  //Serial.println(x);
  return x;
}
float temperature()
{
  sensor1.requestTemperatures();
  temp = sensor1.getTempCByIndex(0)+273.15;
  return temp;
}

void UpdateXbee(){
  
  while(xBee_Serial.available()>0){commandd = xBee.receive();}
  if (commandd == "") {return;}
  else if(commandd.equals("ON"))
  {
    digitalWrite(13,HIGH);
    switchStatus = true;
    resetStatus = false;
    SWITCH();
    xBee.send("\nSAMPLING(1) - " + String(switchStatus));
    return;
  }
  else if(commandd.equals("OFF"))
  {
    digitalWrite(13,LOW);
    switchStatus = false;
    resetStatus = true;
    SWITCH();
    xBee.send("\nSEALED(0) - " + String(switchStatus));
    return;
  }
  else if(commandd.equals("ASCEND"))
  {
    ASCENDING = true;
    xBee.send("\nASCENDING(1) - " + String(ASCENDING));
  }
  else if(commandd.equals("DESCEND"))
  {
    ASCENDING = false;
    xBee.send("\nDESCENDING(0) - " + String(ASCENDING));
  }
  else if(commandd.equals("STATUS")){xBee.send(data);}
  else if ((commandd.substring(0,3)).equals("ONT")) { //blink command
      switchStatus = true;
      resetStatus = false;
      SWITCH();
      xBee.send("\nSAMPLING(1) - " + String(switchStatus));
      byte times = (commandd.substring(3, commandd.length())).toInt();  //check to see how many times to blink
      byte timeinit = millis();
      while((timeinit+(times*1000))> millis()){updateSensors();}
    }
  else if ((commandd.substring(0,4)).equals("OFFT")) { //blink command
      switchStatus = false;
      resetStatus = true;
      SWITCH();
      xBee.send("\nSEALED(0) - " + String(switchStatus));
      byte times = (commandd.substring(4, commandd.length())).toInt();  //check to see how many times to blink
      byte timeinit = millis();
      while((timeinit+(times*1000))> millis()){updateSensors();} 
    }  
  else if(!commandd.equals("")){xBee.send("\nERROR - " + commandd + ": command not recognized");}
}
