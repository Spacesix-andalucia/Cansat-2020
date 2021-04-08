/***************************************************************************
  This is a library for the BMP280 humidity, temperature & pressure sensor

  Designed specifically to work with the Adafruit BMEP280 Breakout
  ----> http://www.adafruit.com/products/2651

  These sensors use I2C or SPI to communicate, 2 or 4 pins are required
  to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>

#define BMP_SCK 5
#define BMP_MISO 12
#define BMP_MOSI 11 
#define BMP_CS 4

float alti; //
float n; //added up first ten height measurements
float forecast; //change according to local weather
float approx;
float highest;


Adafruit_BMP280 bme; // I2C

void setup() {
  n = 0;
  forecast = 1012;
  highest = 0;
  Serial.begin(9600);
  Serial.println(F("REBOOT"));
  Serial.println(F("\nBMP280 test\n"));
  
  if (!bme.begin(0X76)) {  
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
  }
  Serial.println("calibrating sensor");
  for(int s=1; s<=10; s++){
        n+=bme.readAltitude(forecast);
        delay(500);
  }
  Serial.println("\n\nCALIBRATION COMPLETE\n");
  Serial.print("ALTITUDE AT START:  ");
  Serial.print(n/10);
  Serial.println(" m \n\n\n\n");
  delay(1000);
}

void loop() {
      approx=0;   //reset change
      for(int i=1; i<=5; i++){
        alti=bme.readAltitude(forecast)-(n/10);  //measure height difference 5 times
        approx+= alti;
        delay(200);
      }
      Serial.print(F("Approx altitude:            "));
      Serial.print(bme.readAltitude(forecast)); 
      Serial.println(" m");
      
      Serial.print("Approx height difference:   ");
      Serial.print(approx/5);                         //serial out average height difference
      Serial.println(" m");

      if(approx/5>highest){                           //compare heights and store highest value
        highest = approx/5;
        Serial.print("rekord altitude:            ");
        Serial.print(highest+n/10);
        Serial.println(" m");
        Serial.print("\nNEW REKORD ALTITUDE:        ");
        Serial.print(highest);
        Serial.println(" m    NEW REKORD!");
      }else{
        Serial.print("rekord difference:          ");
        Serial.print(highest);
        Serial.println(" m");
        
      }
    Serial.println();
}
