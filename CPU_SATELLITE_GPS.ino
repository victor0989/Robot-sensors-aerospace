#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <SFE_BMP180.h>
#include <Wire.h>
/*
   Projet Otachi-Arduino par Nanikana-Aerospace.
Sonde stratosphérique avec Arduino Uno R3  + Adxl345 + Ublox 6 + DHT11 + Bmp180
Code basé sur des sketchs sous licence Open Source. TinyGPS++ library by Mikal Hart
*/

#define DEVICE (0x53)    //ADXL345 device address
#define TO_READ (6)        //num of bytes we are going to read each time (two bytes for each axis)

byte buff[TO_READ] ;    //6 bytes buffer for saving data read from the device
char str[512];                      //string buffer to transform data before sending it to the serial port

static const int RXPin = 11, TXPin = 10;
static const uint32_t GPSBaud = 9600;

//Uno, Redboard, Pro:        A4   A5 BMP180
SFE_BMP180 pressure;

double baseline; // baseline pressure

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

unsigned long time; // variable pour definir le running time du prog.


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
  return result;
}

void setup()
{
  Wire.begin();        // join i2c bus (address optional for master)
  
  Serial.begin(9600);
  ss.begin(GPSBaud);
  Serial.println("Nanikana-Aerospace 2015");
  Serial.println("Initialisation.....");
  
  Serial.println("CLEARDATA"); //clears any residual data PLX-DAQ
  
   //Turning on the ADXL345
  writeTo(DEVICE, 0x2D, 0);      
  writeTo(DEVICE, 0x2D, 16);
  writeTo(DEVICE, 0x2D, 8);
  smartDelay(5000);
// Get the baseline pressure:
  if (pressure.begin())
    Serial.println("BMP180 init success");
  else
  {
    // Oops, something went wrong, this is usually a connection problem,
    // see the comments at the top of this sketch for the proper connections.

    Serial.println("BMP180 init fail (disconnected?)\n\n");
    while(1); // Pause forever.
  }
  baseline = getPressure();
  Serial.println();
 Serial.println("LABEL,Run, Sats, Latitude, Longitude, AltG, Speed, Card, Temp, AltB, X, Y, Z, Volts");
  Serial.println();
  //Serial.println(F("Run  Sats  Latitude   Longitude     Date       Time        AltG  Speed Card  Temp  AltB   X  Y  Z"));
  
  //Serial.println(F("-------------------------------------------------------------------------------------------------"));
}

void loop()
{
  static const double LONDON_LAT = 51.508131, LONDON_LON = -0.128002;
  char status;
  double a,P;
  
  Serial.print("       ");
  time = millis(); //prints time since program started
  time = time /60000;
  Serial.print(time); Serial.print (" ");
  
  printInt(gps.satellites.value(), gps.satellites.isValid(), 5); //Serial.print (" , ");
  printFloat(gps.location.lat(), gps.location.isValid(), 11, 6); //Serial.print (" , ");
  printFloat(gps.location.lng(), gps.location.isValid(), 12, 6); //Serial.print (" , "); 
  //printDateTime(gps.date, gps.time); Serial.print (" , ");
  printFloat(gps.altitude.meters(), gps.altitude.isValid(), 7, 2); //Serial.print (" ");
  printFloat(gps.speed.kmph(), gps.speed.isValid(), 6, 2); //Serial.print (" ");
  printStr(gps.course.isValid() ? TinyGPSPlus::cardinal(gps.course.value()) : "*** ", 6); Serial.print ("");
// Get a new pressure reading:

  P = getPressure();

  // Show the relative altitude difference between
  // the new reading and the baseline reading:

  a = pressure.altitude(P,baseline);
  
  
  if (a >= 0.0) Serial.print(" "); // add a space for positive numbers
  Serial.print(a,1); Serial.print (" ");
  Serial.print(" ");
  
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
  Serial.print(str); Serial.print(" ");
  Serial.print( readVcc(), DEC );
  Serial.write(10);
  
  
  
Serial.println("");
  smartDelay(1000);

  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println(F("No GPS data received"));
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



double getPressure()
{
  char status;
  double T,P,p0,a;

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
    // Use '&T' to provide the address of T to the function.
    // Function returns 1 if successful, 0 if failure.

    status = pressure.getTemperature(T);
    if (status != 0)
    {
      // Start a pressure measurement:
      // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
      // If request is successful, the number of ms to wait is returned.
      // If request is unsuccessful, 0 is returned.
Serial.print(T,2);
  Serial.print(" ");
      status = pressure.startPressure(3);
      if (status != 0)
      {
        // Wait for the measurement to complete:
        delay(status);

        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P.
        // Use '&P' to provide the address of P.
        // Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.

        status = pressure.getPressure(P,T);
        if (status != 0)
        {
          return(P);
        }
        else Serial.println("error retrieving pressure measurement\n");
      }
      else Serial.println("error starting pressure measurement\n");
    }
    else Serial.println("error retrieving temperature measurement\n");
  }
  else Serial.println("error starting temperature measurement\n");

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
