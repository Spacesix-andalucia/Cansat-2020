#include <SoftwareSerial.h>

SoftwareSerial mySerial(10, 11); // RX, TX
String texto="";
int buffer,buffer2=0;
char z,y;

void setup() {
  // Open serial communications and wait for port to open:
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  Serial.begin(9600);


  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Hola");
  
  mySerial.begin(9600);
  mySerial.println("Hello, world?");
}

void loop() { // run over and over
  texto = "";

 while (mySerial.available()) {
    //texto=texto+mySerial.read();
    texto= mySerial.readString();
    //y=z;
   // Serial.print(texto);
    //mySerial.print(y);
  }
  if (texto!=""){
    Serial.print(texto);
    mySerial.print("Echo: " + texto);
    texto="";
  }
  while (Serial.available()) {
    mySerial.write(Serial.read());
  }
}
