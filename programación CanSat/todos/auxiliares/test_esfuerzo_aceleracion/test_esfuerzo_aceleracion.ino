#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <SoftwareSerial.h>

#define ANTENA_RX           5 
#define ANTENA_TX           6 

/* creación de un puerto serie para Emisor de radiofrecuencia */
SoftwareSerial radioLink( ANTENA_TX , ANTENA_RX); 

Adafruit_MPU6050 mpu;

float aceleracion_max = 0.0;
float velocidad_angular_max = 0.0;
bool cambio_datos = false;
bool en_depuracion = 0;

void setup(void) {
   /* inicializamos el puerto serie para el emisor RF */
  radioLink.begin(9600);
  
  Serial.begin(9600);
  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit MPU6050 test!");

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }

  Serial.println("");
  delay(100);
}

void loop() {

  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  if (en_depuracion){
    /* Print out the values */
    Serial.print("Acceleration X: ");
    Serial.print(a.acceleration.x);
    Serial.print(", Y: ");
    Serial.print(a.acceleration.y);
    Serial.print(", Z: ");
    Serial.print(a.acceleration.z);
    Serial.println(" m/s^2");
  
    Serial.print("Rotation X: ");
    Serial.print(g.gyro.x);
    Serial.print(", Y: ");
    Serial.print(g.gyro.y);
    Serial.print(", Z: ");
    Serial.print(g.gyro.z);
    Serial.println(" rad/s");
  
    Serial.print("Temperature: ");
    Serial.print(temp.temperature);
    Serial.println(" degC");
  
    Serial.println("");
    delay(500);
  }
  
  
  if (a.acceleration.x > aceleracion_max){
    aceleracion_max = a.acceleration.x;
    cambio_datos = true;
  }
  if (a.acceleration.y > aceleracion_max){
    aceleracion_max = a.acceleration.y;
    cambio_datos = true;
  }
  if (a.acceleration.z > aceleracion_max){
    aceleracion_max = a.acceleration.z;
    cambio_datos = true;
  }
  if (g.gyro.x > velocidad_angular_max ){
    velocidad_angular_max = g.gyro.x;
    cambio_datos = true;
  }
  if (g.gyro.y > velocidad_angular_max ){
    velocidad_angular_max = g.gyro.y;
    cambio_datos = true;
  }
  if (g.gyro.z > velocidad_angular_max ){
    velocidad_angular_max = g.gyro.z;
    cambio_datos = true;
  }

  if(cambio_datos){
    Serial.println("Aceleracion Maxima ");
    Serial.print(aceleracion_max);
    Serial.println(" m/s^2");
  
    Serial.println("Velocidad Angular Maxima ");
    Serial.print(velocidad_angular_max);
    Serial.println(" rad/s");
    Serial.println("");
    Serial.println("");

    radioLink.println("Aceleracion Maxima ");
    radioLink.print(aceleracion_max);
    radioLink.println(" m/s^2");
  
    radioLink.println("Velocidad Angular Maxima ");
    radioLink.print(velocidad_angular_max);
    radioLink.println(" rad/s");
    radioLink.println("");
    radioLink.println("");
    cambio_datos = false;
  }

}
