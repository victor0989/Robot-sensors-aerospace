#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <SFE_BMP180.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
/*
********Projet Otachi-Arduino par Nanikana-Aerospace*********
 
 ML8511
 3.3V = 3.3V
 OUT = A1
 GND = GND
 EN = 3.3V
 3.3V = A2
 
 Ublox6 GPS
 RX = D10
 TX = D12 (unused)
 
 BMP180/ADXL345
 A4
 A5
 
 Solar Pannel
 A0
 
 OpenLog/Radio TX
 D1 (TX) 
 
 Dallas DS18b20.
 D5 (One Wire)
 
 HeartBeat from RX Nano : D2 (int 0)
 HeattBeat from TX Nano AND TX On Signal : D3 (int 1)
 D4  Acknowledment Beep to Tx Arduino D12 (On valid command Beep)
 
 D6 To (CS) RX Radio
 D11 to Send Acknowledgment Beep // Change to D4 to D12 TX
 
*/
#define DEVICE (0x53)    //ADXL345 device address
#define TO_READ (6)        //num of bytes we are going to read each time (two bytes for each axis)
byte buff[TO_READ] ;    //6 bytes buffer for saving data read from the device
char str[512];                      //string buffer to transform data before sending it to the serial port
char emic_cmd;
int incomingByte = 0;   // for incoming serial data

int rxcommand = 0; // Serial command from RX Arduino
int rxstr = 9;
int txstate = 1;
int heartbeatrx = 0;
  int heartbeattx = 0;
int txstatus =1;
int mcode =0;
int emic_wait = 500; //wait 0.5 between data ...if emic speak wait 30 sec
static const int RXPin = 10, TXPin = 12;
static const uint32_t GPSBaud = 9600;
int vhftx = 0;

//Uno, Redboard, Pro:        A4   A5 BMP180
SFE_BMP180 pressure;

double baseline; // baseline pressure

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);
int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
//unsigned long time; // variable pour definir le running time du prog.
#define ALTITUDE 307.0 // Altitude of NKA HQ in Valdor,QC,Canada in meters

#define ONE_WIRE_BUS 5 // Setup a oneWire instance to communicate with any OneWire devices 
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
// arrays to hold device address
//DeviceAddress insideThermometer;

//ML8511 pin definitions
int UVOUT = A1; //Output from the sensor
int REF_3V3 = A2; //3.3V power on the Arduino board

long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result/10;
 }
//SoftwareSerial emicSerial =  SoftwareSerial(rxPinE, txPinE);

void setup()
{
  
  Wire.begin();        // join i2c bus (address optional for master)
 // pinMode(rxPinE, INPUT);
  //pinMode(txPinE, OUTPUT);
  // set the data rate for the SoftwareSerial port
 // emicSerial.begin(9600);
  
  Serial.begin(9600);
  
  ss.begin(GPSBaud);
  Serial.println("Otachi Flight COMP Prog. v.09032016 (v.10b)");
  Serial.println("Nanikana-Aerospace 2016");
  //Serial.print("MVE2ZDK\r\n");               //SET YOUR CALLSIGN HERE ONCE (APRS SHIELD ONLY)
  Serial.println("Booting.....");
  /*
    When the Emic 2 powers on, it takes about 3 seconds for it to successfully
    initialize. It then sends a ":" character to indicate it's ready to accept
    commands. If the Emic 2 is already initialized, a CR will also cause it
    to send a ":"
  */
  //emicSerial.print('\n');             // Send a CR in case the system is already up
  //while (emicSerial.read() != ':');   // When the Emic 2 has initialized and is ready, it will send a single ':' character, so wait here until we receive it
  //delay(10);                          // Short delay
  //emicSerial.flush();                 // Flush the receive buffer
  
  Serial.println("");
  
  pinMode(4, OUTPUT); //Acknowledge Command Receive
  digitalWrite(4, LOW);
  pinMode(2, INPUT); // Heartbeat RX
  pinMode(3, INPUT); //HeartBeat TX
  pinMode(6, OUTPUT); // CS signal to Radio PCB
  //pinMode (11, OUTPUT); // SPARE
  pinMode(UVOUT, INPUT);
  pinMode(REF_3V3, INPUT);
  pinMode(8, OUTPUT); // PTT VHF RADIO
  Serial.println("ML8511 : Config Set");
   //Turning on the ADXL345
  writeTo(DEVICE, 0x2D, 0);      
  writeTo(DEVICE, 0x2D, 16);
  writeTo(DEVICE, 0x2D, 8);
  smartDelay(1000);
// Get the baseline pressure:
  if (pressure.begin())
    Serial.println("BMP180 : Ready");
  else
  {
    // Oops, something went wrong, this is usually a connection problem,
    // see the comments at the top of this sketch for the proper connections.

    Serial.println("BMP180 : init fail");
    //while(1); // Pause forever.
  }

  Serial.println();
 
  Serial.println();

// Start up the library
  sensors.begin();


}

void loop()
{
  emic_cmd = '!';
  emic_wait = 500;
  if (Serial.available() > 0) {
                // read the incoming byte:
                rxstr = Serial.read();}
  
  //rxstr = Serial.readStringUntil('C');
  if (rxstr == 53) {Serial.println ("S MASTER CODE RECEIVE"); mcode = 1; digitalWrite(6, HIGH); digitalWrite(4, HIGH); delay (250); digitalWrite(4, LOW); digitalWrite(6, LOW);}
  if (rxstr == 48 && mcode == 1) {Serial.println ("S STOP UHF TX COMMAND RECEIVE"); mcode = 0; txstate = 0; digitalWrite(6, HIGH); digitalWrite(4, HIGH); delay (1000); digitalWrite(4, LOW); digitalWrite(6, LOW);}
  if (rxstr == 49 && mcode == 1) {Serial.println ("S START UHF TX COMMMAND RECEIVE"); mcode = 0; txstate = 1; digitalWrite(6, HIGH); digitalWrite(4, HIGH);delay (1000); digitalWrite(4, LOW); digitalWrite(6, LOW);}
  if (rxstr == 52 && mcode == 1) {Serial.println ("Voice VHF Info TX"); mcode = 0; txstate = 0; emic_cmd ='S'; vhftx = 1; digitalWrite(8, HIGH); emic_wait = 45000;}
  
  //static const double LONDON_LAT = 51.508131, LONDON_LON = -0.128002;
  char status;
  double T,P,p0,a;
  // USE WITH APRS SHIELD FROM ARGENT DATA SYSTEM :
  //Serial.print("@");
  //Serial.print("VE2ZDK");  //Call Sign in Morse Code
  //Serial.print("\r\n"); // TX Morse
  //smartDelay(5000); // WAIT TO TX CALLSIGN 
  
  //Serial.print("!"); //BEGIN OF DATA TO TX in APRS
  
  //END OF APRS SHIELD CMD
  
  
  //Serial.print (rxstr, DEC); // DEBUG : ASCII Command Receive from RX Arduino (DTMF)
  //Serial.print("!"); //BEGIN OF DATA TO TX in APRS
  
  //Serial.print ("{MAP|SET|NKA01|");
  Serial.print (emic_cmd);
  printFloat(gps.location.lat(), gps.location.isValid(), 11, 6); //Serial.print (" ");
  printFloat(gps.location.lng(), gps.location.isValid(), 12, 6); //Serial.print (" "); 
  //printDateTime(gps.date,gps.time); //Serial.print (" ");

  //Serial.print ("Fix ");
 // printInt(gps.satellites.value(), gps.satellites.isValid(), 5); //Serial.print (" ");
Serial.print ("Alt ");
  printFloat(gps.altitude.meters(), gps.altitude.isValid(), 7, 2); //Serial.print (" ");
  
  Serial.print (" Speed ");
  printFloat(gps.speed.kmph(), gps.speed.isValid(), 6, 2); //Serial.print (" ");
  
 // Serial.print ("Head ");
 // printStr(gps.course.isValid() ? TinyGPSPlus::cardinal(gps.course.value()) : "*** ", 6); //Serial.print (" ");
// You must first get a temperature measurement to perform a pressure reading.
  
  // Start a temperature measurement:
  // If request is successful, the number of ms to wait is returned.
  // If request is unsuccessful, 0 is returned.

  status = pressure.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);

    // Retrieve the completed temperature measurement:
    // Note that the measurement is stored in the variable T.
    // Function returns 1 if successful, 0 if failure.

    status = pressure.getTemperature(T);
    if (status != 0)
    {
      // Print out the measurement:
      Serial.print("T_int ");
      Serial.print(T,2);
      //Serial.print(" ");
      //Serial.print((9.0/5.0)*T+32.0,2);
      //Serial.println(" deg F");
      
      // Start a pressure measurement:
      // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
      // If request is successful, the number of ms to wait is returned.
      // If request is unsuccessful, 0 is returned.

      status = pressure.startPressure(3);
      if (status != 0)
      {
        // Wait for the measurement to complete:
        delay(status);

        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P.
        // Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.

        status = pressure.getPressure(P,T);
        if (status != 0)
        {
          // Print out the measurement:
          Serial.print(" Press ");
          Serial.print(P,2);
          //Serial.print(" ");
         // Serial.print(P*0.0295333727,2);
         // Serial.println(" inHg");

          // The pressure sensor returns abolute pressure, which varies with altitude.
          // To remove the effects of altitude, use the sealevel function and your current altitude.
          // This number is commonly used in weather reports.
          // Parameters: P = absolute pressure in mb, ALTITUDE = current altitude in m.
          // Result: p0 = sea-level compensated pressure in mb

          p0 = pressure.sealevel(P,ALTITUDE); // we're at 1655 meters (Boulder, CO)
          //Serial.print("rel.pressure: ");
          //Serial.print(p0,2);
          //Serial.print(" mb, ");
          //Serial.print(p0*0.0295333727,2);
          //Serial.println(" inHg");

          // On the other hand, if you want to determine your altitude from the pressure reading,
          // use the altitude function along with a baseline pressure (sea-level or other).
          // Parameters: P = absolute pressure in mb, p0 = baseline pressure in mb.
          // Result: a = altitude in m.

          a = pressure.altitude(P,p0);
          //Serial.print("Ab");
          Serial.print(a,0);
          //Serial.print(" ");
          //Serial.print(a*3.28084,0);
          //Serial.println(" feet");
        }
        else Serial.println("S error pressure"); //REMOVE S if you dont use Emic2
      }
      else Serial.println("S error starting pressure");
    }
    else Serial.println("S error temperature");
  }
  else Serial.println("S error starting temperature");

  
  int regAddress = 0x32;    //first axis-acceleration-data register on the ADXL345
  int x, y, z;
  
  readFrom(DEVICE, regAddress, TO_READ, buff); //read the acceleration data from the ADXL345
  
   //each axis reading comes in 10 bit resolution, ie 2 bytes.  Least Significat Byte first!!
   //thus we are converting both bytes in to one int
  x = (((int)buff[1]) << 8) | buff[0];   
  y = (((int)buff[3])<< 8) | buff[2];
  z = (((int)buff[5]) << 8) | buff[4];
   //we send the x y z values as a string to the serial port
  sprintf(str, "%d %d %d", x, y, z);  
  //Serial.print(str);
  Serial.print (" X ");
  Serial.print (x); //Serial.print(" ");
  Serial.print(" Y ");
  Serial.print (y); //Serial.print(" ");
  Serial.print(" Z ");
  Serial.print (z); Serial.print(" ");
  
  Serial.print(" Vcpu ");
  Serial.print( readVcc(), DEC ); Serial.print(" ");
  //Serial.write(10);
  
  int sensorValue = analogRead(A0);
  // Convert the analog reading (which goes from 0 - 1023) to a % of sunlight (base on 5v max solar panel:
  float voltage = sensorValue * (100 / 1023.0);
  // print out the value you read:
  Serial.print(" Sun ");
  Serial.print(voltage); Serial.print(" ");
  
  Serial.print(" T_ext ");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.print(sensors.getTempCByIndex(0)); 
  Serial.print(" ");
  
  int uvLevel = averageAnalogRead(UVOUT);
  int refLevel = averageAnalogRead(REF_3V3);
  
  //Use the 3.3V power pin as a reference to get a very accurate output value from sensor
  float outputVoltage = 3.3 / refLevel * uvLevel;
  
  float uvIntensity = mapfloat(outputVoltage, 0.99, 2.8, 0.0, 15.0); //Convert the voltage to a UV intensity level

  //Serial.print("output: ");
  //Serial.print(refLevel);

  //Serial.print("ML8511 output: ");
  //Serial.print(uvLevel);

  //Serial.print(" / ML8511 voltage: ");
  //Serial.print(outputVoltage);
  
  //Serial.print("UV (mW/cm^2): ");
  Serial.print("UV  ");
  Serial.print(uvIntensity);
  Serial.print(" "); 
  Serial.print(" Mem ");
  Serial.print (freeRam());
  heartbeatrx = digitalRead (2);
  heartbeattx = digitalRead (3);
  Serial.print (" State ");
  if (heartbeattx == 1 && txstate == 1) {digitalWrite(6, HIGH); txstatus =1; } else {digitalWrite(6, LOW); txstatus = 0;}  // If heartbeat/tx = 1 then CS Signal to Radio else stay LOW 
  Serial.print (heartbeatrx); Serial.print(" "); Serial.print (heartbeattx); Serial.print(" "); Serial.print (txstatus); Serial.print(" ");
  Serial.print (mcode);
  
   
  Serial.print("\r\n");
  
  
  
  
//Serial.println(" ");
  smartDelay(emic_wait); //WAIT 0.5 SEC or 30 sec
  digitalWrite(8, LOW); // TURN OFF PPT VHF TX
  if (vhftx == 1) txstate = 1;

  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println(F("No GPS data"));
}

// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

static void printFloat(float val, bool valid, int len, int prec)
{
  if (!valid)
  {
    while (len-- > 1)
      Serial.print('*');
    Serial.print(' ');
  }
  else
  {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i=flen; i<len; ++i)
      Serial.print(' ');
  }
  smartDelay(0);
}

static void printInt(unsigned long val, bool valid, int len)
{
  char sz[32] = "*****************";
  if (valid)
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i=strlen(sz); i<len; ++i)
    sz[i] = ' ';
  if (len > 0) 
    sz[len-1] = ' ';
  Serial.print(sz);
  smartDelay(0);
}


static void printDateTime(TinyGPSDate &d, TinyGPSTime &t)
{
  if (!d.isValid())
  {
    Serial.print(F("********** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
    Serial.print(sz);
  }
  
  if (!t.isValid())
  {
    Serial.print(F("******** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
    Serial.print(sz);
  }

  printInt(d.age(), d.isValid(), 5);
  smartDelay(0);
}

static void printStr(const char *str, int len)
{
  int slen = strlen(str);
  for (int i=0; i<len; ++i)
    Serial.print(i<slen ? str[i] : ' ');
  smartDelay(0);
}
//---------------- Functions
//Writes val to address register on device
void writeTo(int device, byte address, byte val) {
   Wire.beginTransmission(device); //start transmission to device 
   Wire.write(address);        // send register address
   Wire.write(val);        // send value to write
   Wire.endTransmission(); //end transmission
}

//reads num bytes starting from address register on device in to buff array
void readFrom(int device, byte address, int num, byte buff[]) {
  Wire.beginTransmission(device); //start transmission to device 
  Wire.write(address);        //sends address to read from
  Wire.endTransmission(); //end transmission
  
  Wire.beginTransmission(device); //start transmission to device
  Wire.requestFrom(device, num);    // request 6 bytes from device
  
  int i = 0;
  while(Wire.available())    //device may send less than requested (abnormal)
  { 
    buff[i] = Wire.read(); // receive a byte
    i++;
  }
  Wire.endTransmission(); //end transmission
}
//Takes an average of readings on a given pin
//Returns the average
int averageAnalogRead(int pinToRead)
{
  byte numberOfReadings = 8;
  unsigned int runningValue = 0; 

  for(int x = 0 ; x < numberOfReadings ; x++)
    runningValue += analogRead(pinToRead);
  runningValue /= numberOfReadings;

  return(runningValue);  
}

//The Arduino Map function but for floats
//From: http://forum.arduino.cc/index.php?topic=3922.0
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}
