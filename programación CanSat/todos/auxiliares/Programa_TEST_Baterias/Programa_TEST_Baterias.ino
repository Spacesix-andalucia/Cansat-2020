/*

  Programa para el Cansat del equipo SpaceSix 
  Versión v5
   
   Todos los elementos integrados

*/

#include <Wire.h>                 // Utilidades para comunicaciones I2C
#include <Adafruit_Sensor.h>
#include <i2c_BMP280.h>           // Biblioteca de funciones para el barometro
#include <EEPROM.h>
#include <Adafruit_VEML6070.h>
#include <SoftwareSerial.h>
#include <Temporizador_inopya.h>
#include <Universal_GPS_inopya.h>


#define PIN_RESET            5 
#define PIN_ALTAVOZ          6  
#define ANTENA_RX           11 
#define ANTENA_TX           10 
#define GPS_RX               3
#define GPS_TX               4
#define TIEMPO_MUESTREO       VEML6070_HALF_T   //½T 62.5 ms
#define TCAADDR               0x70
#define PUERTO_UV_1           2
#define PUERTO_UV_2           3 
#define PUERTO_UV_3           4 



#define INTERVALO_MUESTRAS    1000    // tiempo en milisegundos entre cada muestra
                                      // Recordad que disponemos de memoria para 204 muestras,
                                      // dependiendo del intervalo, tendremos mayor o menor tiempo de grabacion 
                                      
#define INTERVALO_RECALIBRACION_ALTURA  60000   //  60000 ms == 1 minuto

#define EN_DEPURACION   0            // Si se pone a 1 (true) es que estamos en depuracion y a 0 (false) es que no

/* Sensores Ultravioletas */
Adafruit_VEML6070 uv_1 = Adafruit_VEML6070();
Adafruit_VEML6070 uv_2 = Adafruit_VEML6070();
Adafruit_VEML6070 uv_3 = Adafruit_VEML6070();

/* creaccion del objeto barometro/altimetro */
BMP280 bmp280;

/* creación de un puerto serie para Emisor de radiofrecuencia */
SoftwareSerial radioLink( ANTENA_TX , ANTENA_RX); 

/* creación de un puerto serie para el GPS */
SoftwareSerial gpsPort(GPS_TX, GPS_RX);  //4 ,3

/* creación del objeto GPS */
Universal_GPS_inopya gps(&gpsPort);

/* creación de  objetos Temporizador_inopya */
Temporizador_inopya relojMuestras;
Temporizador_inopya relojMensajes;
Temporizador_inopya relojEscucha;
Temporizador_inopya relojRecalibracionAltura;

/* creacion de un nuevo tipo de datos para contener las muestras */
struct CanSatDatos {  
                     int altura; 
                     int temperatura; 
                     uint8_t uv;
                   };

boolean FLAG_uso_eeprom = false;   // Preserva la eeprom de usos innecesarios
uint8_t contador_suelo = 0;
int puntero_eeprom = 0;   
               
float altura;      
float temperatura;
float indiceUv;

float altura_suelo;
int altura_para_empezar_a_medir = 0;  //A que altura se ha de empezar a tomar muestras
float altura_maxima = 0;
float altura_anterior = 10000;  //para el control de llaga a suelo, iniciada fuera de rango

float intervalo = INTERVALO_MUESTRAS / 1000.0;
int contador = 0;

bool FLAG_pararZumbador = false;
bool orden_lanzamiento = true;

/* Funcion de seleccion del sensor UV que va a estar activo */
void tcaselect(uint8_t i) {
  if (i > 7) return;
 
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();  
}

//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm 
//***************************************************************************************************
//  ANTES DEL LANZAMIENTO
//***************************************************************************************************
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm 

void setup() 
{
  digitalWrite(PIN_RESET, HIGH);
  delay(200); 
  pinMode(PIN_RESET, OUTPUT);
  
  pinMode(PIN_ALTAVOZ, OUTPUT);

  /* inicializamos el puerto serie para el PC  (pruebas) */
  Serial.begin(9600);
  
  /* inicializamos el puerto serie para el emisor RF */
  radioLink.begin(9600);

  /* inicializamos el GPS */
  gps.begin(9600);  

  
  /* inicializamos el barometro */    
  altura_suelo = inicializarBarometro();

  /* inicializamos los sensores Ultravioletas */
  tcaselect(PUERTO_UV_1);
  uv_1.begin(TIEMPO_MUESTREO);
  tcaselect(PUERTO_UV_2);
  uv_2.begin(TIEMPO_MUESTREO);
  tcaselect(PUERTO_UV_3);
  uv_3.begin(TIEMPO_MUESTREO);

  // Fijando el umbral de lanzamiento
  
  enviar_mensaje("Cansat necesita que se le envie un umbral de lanzamiento (en metros)");
  /*
  char orden_recibida = ' ';
   while(!Serial.available() || !radioLink.available()) {
    
    if(Serial.available()){
      altura_para_empezar_a_medir = Serial.parseInt();
      }
    if(radioLink.available()){
      altura_para_empezar_a_medir = radioLink.parseInt();
      }  
    if(altura_para_empezar_a_medir >= 1){
        Serial.println(altura_para_empezar_a_medir);
        radioLink.print("Fijado el umbral de lanzamiento a ");
        radioLink.print (altura_para_empezar_a_medir);
        radioLink.println(" metros");
        radioLink.println("");
        break;
      }
  }
  
  //Esta parte espera a que el cohete despegue y alcance una altura de más de ALTURA_PARA_EMPEZAR_A_MEDIR metros .
  //Se queda en la espera de que la altura menos la altura del suelo sea mayor de ALTURA_PARA_EMPEZAR_A_MEDIR .
  // Tambien se puede salir de esta espera mediante una señal de lanzamiento por radio
  
  while(true) {

    // Cada cierto tiempo recalibramos la altura_suelo para compensar las variaciones meteorológicas
    // Y comprobamos que el GPS recibe bien los datos
    if (relojRecalibracionAltura.estado() == false){
      medirAlturaYTemperatura();
      altura_suelo = altura;
      String mensaje_altura = "Recalibrando el sensor de Altitud, Altura suelo medida:  ";
      mensaje_altura.concat(altura_suelo);
      mensaje_altura.concat("\nPresione L para empezar a tomar datos y D para ver los datos del lanzamiento anterior");
      Serial.println(mensaje_altura);
      radioLink.println(mensaje_altura);
      Serial.println("");
      radioLink.println("");
      comunicar_posicion();
      relojRecalibracionAltura.begin(INTERVALO_RECALIBRACION_ALTURA); 
    }

    // Atiende las posibles ordenes entrantes por radio o puerto serie durante 2 segundos
    atenderPeticionesEntrantes(2000);
    
    if(orden_lanzamiento == true){
      break;
    }
    
    medirAlturaYTemperatura();

    if(EN_DEPURACION){
      Serial.println(altura);
      Serial.println(altura_suelo);
      Serial.println(altura - altura_suelo);
      Serial.println("");
      delay(1000);
    }
    
    if((altura - altura_suelo) > altura_para_empezar_a_medir){
      orden_lanzamiento = true;
      break;
    }
  
  }*/

 FLAG_uso_eeprom = true;

 enviar_mensaje("Cansat procediendo al lanzamiento y la toma de datos");

 if (EN_DEPURACION)
 {
   sonidoLanzamiento();
 }
 
}

//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm 
//***************************************************************************************************
//  DURANTE EL LANZAMIENTO 
//***************************************************************************************************
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm 
void loop() 
{ 
  /* control de tiempo para tareas de toma de datos y transmision a tierra */
  if(relojMuestras.estado()== false){
    relojMuestras.begin(INTERVALO_MUESTRAS); //Reinicio del reloj de toma de muestras y otras tareas.

    /* Actualizar datos de altura y temperatura */
    medirAlturaYTemperatura();

    /* Medimos los sensores UV */
    //indiceUv = obtener_UV_max();

    // Guardamos los datos en la memoria EEPROM , si queda espacio
    // Si no queda espacio en la EEPROM esta función activará la baliza de rescate
    // guardarDatosMemoria();

    /* Actualizamos el dato de altura maxima alcanzada, por si lo usamos para algo */
    if (altura > altura_maxima){
      altura_maxima = altura;
    }
  
    /* Transmitir datos a tierra */
    contador = contador + 1;

    radioLink.print(contador); radioLink.print(F(",\t\t"));
    radioLink.print(altura); radioLink.print(F(",\t\t"));
    radioLink.print(temperatura); radioLink.print(F(",\t\t"));
    radioLink.print(indiceUv);
    radioLink.println("");
   
    /* mostar datos por serial */ 
   if(EN_DEPURACION){
    Serial.print(contador);
    Serial.print("*");  
    Serial.print(altura);
    Serial.print("*");
    Serial.print(temperatura);
    Serial.print("*");
    Serial.print(indiceUv);
    Serial.println("");
   }
  }
                                                                                                                                                                                                                        
}


/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
   ALTIMETRO
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

float inicializarBarometro(){
  
  if (bmp280.initialize()){
      Serial.println(F("Sensor presion ok"));
  }
  else{
      Serial.println(F("Fallo de Sensor, programa detenido"));
    while (true) {}  //si no hay barometro el programa queda en bucle infinito
  }

  /* Configuracion de calibracion */
  bmp280.setPressureOversampleRatio(2);    
  bmp280.setTemperatureOversampleRatio(1);
  bmp280.setFilterRatio(4);                
  bmp280.setStandby(0);                     
  
  /* Medidas bajo demanda */
  bmp280.setEnabled(0);                     //0=Desactivamos el modo automatico. Solo obtendremos respuesta
                                            //del sensor  tras una peticion con 'triggerMeasurement()

  // Si no se toman varias medidas consecutivas no mide bien la altura del suelo
  for(int i =0 ; i < 5 ; i++){
    medirAlturaYTemperatura();
    delay(100);
  }
  
  return  altura;
}


void medirAlturaYTemperatura()
{
  bmp280.triggerMeasurement();    //peticion de nueva medicion
  bmp280.awaitMeasurement();      //esperar a que el sensor termine la medicion
  bmp280.getAltitude(altura);
  bmp280.getTemperature(temperatura);
}

/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//   ULTRAVIOLETAS
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

float obtener_UV_max()
{
  
  int lectura_UV_max = 0;                  

  tcaselect(PUERTO_UV_1);
  int lecturaUV_RAW_1 = uv_1.readUV();
  if(lecturaUV_RAW_1 > lectura_UV_max){
    lectura_UV_max = lecturaUV_RAW_1;
  }
  tcaselect(PUERTO_UV_2);
  int lecturaUV_RAW_2 = uv_2.readUV();
  if(lecturaUV_RAW_2 > lectura_UV_max){
    lectura_UV_max = lecturaUV_RAW_2;
  }
  tcaselect(PUERTO_UV_3);
  int lecturaUV_RAW_3 = uv_3.readUV();
  if(lecturaUV_RAW_3 > lectura_UV_max){
    lectura_UV_max = lecturaUV_RAW_3;
  }

  float indice_UV = lectura_UV_max/280.0;

  if (EN_DEPURACION){
    Serial.print("Sensores UV: "); 
    Serial.print(lecturaUV_RAW_1); Serial.print(" ");
    Serial.print(lecturaUV_RAW_2); Serial.print(" ");
    Serial.print(lecturaUV_RAW_3); Serial.print(" ");
    Serial.println("");
    Serial.print("MaximoUV: "); Serial.print(lectura_UV_max);
    Serial.println("");
    Serial.print("indice_UV: "); Serial.print(indice_UV);
    Serial.println("");
    Serial.println("");
  }

  return indice_UV;
}

/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//   EEPROM
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

CanSatDatos loadData(int posicion)
{
  CanSatDatos muestra;
  EEPROM.get(posicion, muestra);
  return muestra;
}

void saveData(int posicion, CanSatDatos muestra)
{
  EEPROM.put(posicion, muestra);
}

void guardarDatosMemoria(){
  
  /* Salvado de datos (MIENTRAS QUEDE MEMORIA) --  */
    if(FLAG_uso_eeprom==true){
      /* altura con dos decimales, como un entero */
      int altura_int = int(altura*100);
      /* temepratura con dos decimales, como un entero */
      int temperatura_int = int(temperatura*100);
      /* indice UV con 1 decimal, como un entero */  
      uint8_t indiceUv_int = indiceUv*10;
  
      /* empaquetado de los datos de interes en un 'struct' */
      CanSatDatos datos_actuales = { altura_int, temperatura_int, indiceUv_int };
      saveData(puntero_eeprom, datos_actuales);         //enviamos un dato del tipo 'CanSatDatos'
      puntero_eeprom+=5;                                //incrementamos el puntero para escritura
      /* Si llenamos la eeprom, dejamos de grabar y desactivamos los permisos de acceso*/
      if(puntero_eeprom > 1020 || puntero_eeprom < 5){ 
        FLAG_uso_eeprom = false; // bloqueo de acceso para evitar sobreescribir
        orden_lanzamiento = false;
        baliza_Rescate();
      }
    }
}

void listar_datos()
{
  int puntero_lectura = 0;
  int contador_serie = 0;
  int contador_radio = 0;
  
  Serial.println();
  Serial.println(F("Tiempo (s), Altura (m), Temperatura (C) ,  Ultravioletas"));

  radioLink.println();
  radioLink.println(F("Tiempo (s), Altura (m), Temperatura (C), Ultravioletas"));
  
  while(puntero_lectura < 1020){
    /* recuperar datos de la eeprom */
    CanSatDatos dato_leido;
    EEPROM.get(puntero_lectura, dato_leido);
    
    /* tratar los datos recuperados */
    float altura_float = float(dato_leido.altura)/100.0;
    float temperatura_float = float(dato_leido.temperatura)/100.0;
    float indiceUV_float = float(dato_leido.uv)/10.0;
   
    /* mostar datos por puerto serie */
    Serial.print(contador_serie++); Serial.print(F(","));
    Serial.print(altura_float); Serial.print(F(","));
    Serial.print(temperatura_float);Serial.print(F(","));
    Serial.print(indiceUV_float);
    Serial.println("");

    /* mostar datos por puerto radio */
    radioLink.print(contador_radio++); radioLink.print(F(","));
    radioLink.print(altura_float); radioLink.print(F(","));
    radioLink.print(temperatura_float);radioLink.print(F(","));
    radioLink.print(indiceUV_float);
    radioLink.println("");
    
    /* incrementar el puntero de lectura de la eeprom */
    puntero_lectura +=5;
  }
}


/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//   PUERTO SERIE Y RADIO ENLACE
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

int atenderPeticionesEntrantes(int intervalo_miliseg) 
  // Se queda a la escucha del puerto serie durante un intervalo de tiempo determinado
{

  if (relojEscucha.estado() == false){
      relojEscucha.begin(intervalo_miliseg); //Para mostrar los mensajes cada 3 segundos
    }
    
  char orden_recibida = ' ';
   while(!Serial.available() || !radioLink.available()) {
    
    if(Serial.available()){
      orden_recibida = Serial.read();
    }
    if(radioLink.available()){
      orden_recibida = radioLink.read();
    }
    if(orden_recibida == 'd' or orden_recibida == 'D'){ 
      //Serial.flush();
      listar_datos();
    }
    if(orden_recibida == 'l' or orden_recibida == 'L'){ 
      orden_lanzamiento = true;
    }
    if (relojEscucha.estado() == false){
      break;
    }
   }
  return 0;
}

void enviar_mensaje(String mensaje){

  radioLink.println("********************************************************************");
  radioLink.print("Mensaje enviado desde el Cansat de SpaceSix: ");
  radioLink.println(mensaje);
  radioLink.println("********************************************************************");
  radioLink.println("");
  Serial.println("=======================================================================");
  Serial.println(mensaje);
  Serial.println("=======================================================================");
  Serial.println("");
}


/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//   LANZAMIENTO
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

void sonidoLanzamiento(){

  int cont = 0;
  while(cont < 5){
    /* Generar tono para localizacion */
    tone(PIN_ALTAVOZ, 2100);  //frecuencia que emite un sonido bastante estridente
    delay (450);
    noTone(PIN_ALTAVOZ);
    delay (450);
    cont++;
  }
}

/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//   GPS
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

void comunicar_posicion()
{
   /* obtener datos del GPS */
      gps.get(); 
      /* Transmitir datos de POSICION GPS */
      radioLink.print("Longitud "); radioLink.print(gps.longitud,6);
      radioLink.print(F(" , "));
      radioLink.print("Latitud "); radioLink.println(gps.latitud,6);
      radioLink.println("");

      Serial.print("Longitud "); Serial.print(gps.longitud,6);
      Serial.print(F(" , "));
      Serial.print("Latitud "); Serial.println(gps.latitud,6);
      Serial.println("");
}


/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//   LOCALIZACION DURANTE EL RESCATE
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

void baliza_Rescate()
{
  while(orden_lanzamiento == false ){
    
    /* Generar tono para localizacion */
    for(int i =0; i < 5; i++){
      if (FLAG_pararZumbador == false) { //BORRAR EL IF.
      tone(PIN_ALTAVOZ, 2100);  //frecuencia que emite un sonido bastante estridente
      delay (450);
      noTone(PIN_ALTAVOZ);
      }
    }

    comunicar_posicion();
    
    enviar_mensaje("Cansat ha realizado su aterrizaje, presione D para ver los datos o L para un nuevo lanzamiento ");
   
    atenderPeticionesEntrantes(2000);
    if(orden_lanzamiento == true){
      puntero_eeprom = 0;
      FLAG_uso_eeprom = true;   
      Serial.println("Realizando un nuevo lanzamiento");
      //Reseteo el Arduino
      contador = 0;
      digitalWrite(PIN_RESET, LOW); 
    }
  }
}
