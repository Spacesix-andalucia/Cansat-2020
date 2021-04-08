/*

  Programa para el Cansat del equipo SpaceSix 
  Versión v1
  
  Versión en la que se mide la altura y la temperatura con el BMP 280

*/

#include <SoftwareSerial.h>
#include <Universal_GPS_inopya.h>


#define ANTENA_RX           5 
#define ANTENA_TX           6 

#define GPS_RX               3
#define GPS_TX               4





/* creación de un puerto serie para Emisor de radiofrecuencia */
SoftwareSerial radioLink( ANTENA_TX , ANTENA_RX); 

/* creación de un puerto serie para el GPS */
SoftwareSerial gpsPort(GPS_TX, GPS_RX);  //4 ,3

/* creación del objeto GPS */
Universal_GPS_inopya gps(&gpsPort);

//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm 
//***************************************************************************************************
//  ANTES DEL LANZAMIENTO
//***************************************************************************************************
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm 

void setup() 
{

  /* inicializamos el puerto serie para el PC  (pruebas) */
  Serial.begin(9600);
  
  /* inicializamos el puerto serie para el emisor RF */
  radioLink.begin(9600);

  /* inicializamos el GPS */
  gps.begin(9600);  

}

//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm 
//***************************************************************************************************
//  DURANTE EL LANZAMIENTO 
//***************************************************************************************************
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm 
void loop() 
{ 
    /* obtener datos del GPS */
    gps.get(); 

    /* Transmitir datos de POSICION GPS */
    radioLink.print("Longitud "); radioLink.print(gps.longitud,6);
    radioLink.print(F(" , "));
    radioLink.print("Latitud "); radioLink.println(gps.latitud,6);
    radioLink.println("");

    Serial.print("Longitud ");Serial.print(gps.longitud,6);
    Serial.print(F(" , "));
    Serial.print("Latitud ");Serial.println(gps.latitud,6);
    Serial.println("");   

    delay(2000);
                                                                                                                                                                      
}
