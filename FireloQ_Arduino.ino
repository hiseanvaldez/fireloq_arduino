#include "SparkFun_LIS331.h"
#include <Wire.h>
#include <Arduino.h>
#include "NMEAGPS.h"
#include <SoftwareSerial.h>
#include <FPC1020.h>

LIS331 accel;
SoftwareSerial ss_gps(9, 23);
  SoftwareSerial ss_gsm(10, 12);
SoftwareSerial ss_fpc(11, 21);
FPC1020 fpc(&ss_fpc);

NMEAGPS gps;
static double longitude = -1;
static double latitude = -1;
static long timer = 0;
int onTime = 1000;
int offTime = 3000;
unsigned long previousMillis = 0;
int interval = onTime, ledState = LOW, bypass = 0;

void setup() {
  Wire.begin();
  pinMode(13, OUTPUT); //LED
  pinMode(5, INPUT);   //INT1
  pinMode(6, INPUT);   //INT2
  pinMode(18, OUTPUT); //SOLENOID
  pinMode(19, INPUT);  //BTN1
  pinMode(20, INPUT);  //BTN2

  accel.setI2CAddr(0x19);
  accel.begin(LIS331::USE_I2C);
  accel.setFullScale(LIS331::LOW_RANGE);        // Set accel sensitivity 100G/200G/400G

  accel.intSrcConfig(LIS331::INT_SRC, 1);
  accel.setIntDuration(3, 1);
  accel.setIntThreshold(3, 1);
  accel.enableInterrupt(LIS331::Z_AXIS, LIS331::TRIG_ON_HIGH, 1, true);

  //  accel.intSrcConfig(LIS331::INT_SRC, 2);
  //  accel.setIntDuration(3, 2);
  //  accel.setIntThreshold(2, 2);
  //  accel.enableInterrupt(LIS331::Z_AXIS, LIS331::TRIG_ON_HIGH, 2, true);

  Serial.begin(9600);
  Serial1.begin(9600);
  ss_gps.begin(9600);
  ss_gsm.begin(9600);
  ss_fpc.begin(9600);
  initGSM();
}

void loop() {
  int16_t x, y, z;
  ss_gps.listen();
  if (digitalRead(19) == HIGH) {
    // turn LED on:
    digitalWrite(13, HIGH);
    digitalWrite(18, LOW);
    
    //FINGERPRINT SEARCH
//    unsigned char l_ucFPID;
//    ss_fpc.listen();
//    if ( fpc.Search()) {
//      Serial.print("Success, your User ID is: ");
//      Serial.println( l_ucFPID, DEC);
//    }
//    else {
//      Serial.println("Failed, please try again.");
//    }
//    delay(1000);
  } else {
    // turn LED off:
    digitalWrite(13, LOW);
    digitalWrite(18, HIGH);
  }
  if (digitalRead(20) == HIGH) {
    // turn LED on:
    bypass = 1;
    Serial.println(bypass);
  }
  else{
    bypass = 0;
  }

  //GET ACCELEROMETER VALUES
  if (millis() - timer > 50) {
    timer = millis();
    accel.readAxes(x, y, z);
    //DISPAY VALUES FOR DEBUGGING & TESTING
    //    Serial.print(x);
    //    Serial.print(", ");
    //    Serial.print(y);
    //    Serial.print(", ");
    //    Serial.print(z);
    //    Serial.println();
    //    Serial1.print(latitude, 6);
    //    Serial1.print(',');
    //    Serial1.print(longitude, 6);
    //    Serial1.print(',');
    //    Serial1.print(timer);
    //    Serial1.println();
  }

  //GET GPS VALUES
  while (gps.available(ss_gps)) {
    getCoords( gps.read() );
  }

  //SEND MESSAGE TO ANDROID VIA BT IF INTERRRUPT
  if ( digitalRead(5) == HIGH) {
    Serial1.print(latitude, 6);
    Serial1.print(',');
    Serial1.print(longitude, 6);
    Serial1.print(',');
    Serial1.print(timer);
    Serial1.println();
    Serial.println("INTERRUPT!");
    sendGSM();
  }
}

//INITIALIZE GSM MODULE FOR GPRS CONNECTION
void initGSM() {
  ss_gsm.listen();
  Serial.println("Config SIM800L...");
  delay(2000);
  Serial.println("Done!...");
  ss_gsm.flush();
  Serial.flush();
  ss_gsm.println("AT+CGATT?");
  delay(100);
  readGSM();
  ss_gsm.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
  delay(250);
  readGSM();
  ss_gsm.println("AT+SAPBR=3,1,\"APN\",\"internet\"");
  delay(250);
  readGSM();
  ss_gsm.println("AT+SAPBR=1,1");
  delay(5000);
  readGSM();
}

//SEND VALUES TO FIRESTORE VIA HTTP SERVICE
void sendGSM() {
  ss_gsm.listen();
  // initialize http service
  ss_gsm.println("AT+HTTPINIT");
  delay(250);
  readGSM();

  // initialize http service
  ss_gsm.println("AT+HTTPPARA=\"CID\",1");
  delay(250);
  readGSM();

  // set http param value
  ss_gsm.print("AT+HTTPPARA=\"URL\",\"https://fireloq-system.000webhostapp.com/?lat=");
  ss_gsm.print(latitude, 6);
  ss_gsm.print("&lon=");
  ss_gsm.print(longitude, 6);
  if (bypass) {
    ss_gsm.print("&byp=1");
  }
  ss_gsm.println("\"");
  delay(250);
  readGSM();

  // activate ssl
  ss_gsm.println("AT+HTTPSSL=1");
  delay(250);
  readGSM();

  // set http action type 0 = GET, 1 = POST, 2 = HEAD
  ss_gsm.println("AT+HTTPACTION=0");
  delay(15000);
  readGSM();

  // terminate http service
  ss_gsm.println("AT+HTTPTERM");
  readGSM();
  delay(250);
}

//READ GSM MODULE REPLIES
void readGSM()
{
  while (ss_gsm.available() != 0)
  {
    Serial.write(ss_gsm.read());
  }
}

//GET COORDINATES FROM GPS MODULE
void getCoords( const gps_fix & fix )
{
  if (fix.valid.location) {
    latitude = fix.latitude();
    longitude = fix.longitude();
    Serial.print( fix.latitude(), 6 ); // floating-point display
    Serial.print( ',' );
    Serial.print( fix.longitude(), 6 ); // floating-point display
    Serial.println();
 
  }
  else {
    Serial.println("No valid GPS data.");
  }
}
