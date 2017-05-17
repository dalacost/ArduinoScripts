/*
   LACOSOX MP2.5 TEST
   lectura experimental de la concentración de material 
   particulado 2.5 + Temperatura, Presión atmosférica, 
   Humedad almosférica utilizando el hardware
   Arduino + DSM501+ ESP8266 ESP-01S Wireless Wifi 
   + Strinity Sensor cobber (TSL2561, BMP180, AM2321). 
   Actualmente almacenando datos a: 
   https://thingspeak.com/channels/217483


   El codigo ha sido comprimido y optimizado para que trabaje
   correctamente en 32KB de memoria en arduino UNO. 

   Requiere configuraciòn externa (por serial) de la tarjeta 
   wifi ESP-01S con la red a la que deseas acceder. 
   
*/
/*
  3V3 to 3.3V
  GND to GND
  SDA a A4
  SCL a A5
*/
#include <SoftwareSerial.h>
#include <string.h>
#include <SPI.h>

#include "SFE_TSL2561.h"
#include <SFE_BMP180.h>
#include <Adafruit_AM2315.h>

#include <Wire.h>
#include "ThingSpeak.h"


SFE_TSL2561 light_sensor;
SFE_BMP180 bmp180_sensor;
Adafruit_AM2315 am2315_sensor;

#define ALTITUDE 1655.0 // Altitude of SparkFun's HQ in Boulder, CO. in meters

SoftwareSerial BT1(5, 4); // RX | TX

const char* server = "api.thingspeak.com";
String apiKey = "XXXXX";

int pin = 8;//DSM501A input D8
unsigned long duration;
unsigned long starttime;
unsigned long endtime;
unsigned long sampletime_ms = 10000;
unsigned long lowpulseoccupancy = 0;
float ratio2 = 0;
float concentration2 = 0;

// convert from particles/0.01 ft3 to μg/m3
// 0.01ft3 = 0.0002831685m3 or 35.3147ft3  = 1m3
double pcs2ugm3 (double concentration_pcs)
{
  double pi = 3.14159;
  double vol25 = (4 / 3) * pi * pow ((0.44 * pow (10, -6)), 3);
  return concentration_pcs * 3531.47 * (1.65 * pow (10, 12)) * vol25;
}

void setup()
{
  Serial.begin(9600);
  BT1.begin(115200);
  pinMode(8, INPUT);
  starttime = millis();

  Serial.println("Lacosox station ");
  Serial.println("-------------------");

  Serial.print("luz:");Serial.println(light_sensor.begin());
  Serial.print("BMP180:");Serial.println(bmp180_sensor.begin());
  Serial.print("am2315:");Serial.println(am2315_sensor.begin());

  BT1.println("AT+RST");
  delay(1000);
  BT1.println("AT+RFPOWER=82");


}

String get_MP25() {
    lowpulseoccupancy = 0;
    starttime = millis();
    float concentration_ugm3 = 0.0;
    boolean salir = false;

    do {
       duration = pulseIn(pin, LOW);
       lowpulseoccupancy += duration;
       endtime = millis();

    if ((endtime - starttime) > sampletime_ms)
      {
       ratio2 = (lowpulseoccupancy / 10.0 ) / (endtime - starttime); // Porcentaje 0 - 100 lowpulseoccupancy is in microseconds
       concentration2 = (1.1 * pow(ratio2, 3) - 3.8 * pow(ratio2, 2) + 520 * ratio2 + 0.62); // pcs/0.01cf
       concentration_ugm3 = pcs2ugm3(concentration2);

       salir = true;
     }
    } while (!salir);
  

 return String(concentration_ugm3);
}


void loop() {
/*
  String B = "." ;
  if (BT1.available())
  { char c = BT1.read() ;
    Serial.print(c);
  }
  if (Serial.available())
  { char c = Serial.read();

    if (c == 'u')
    {
*/
      String mp25 = get_MP25();
      double luxx = getlux();
      double tempp = get_temp();
      double pres = get_presion(tempp);
      float humedad = am2315_sensor.readHumidity();

      Serial.print(" MP25: "); Serial.print(mp25);
      Serial.print(" lux: "); Serial.print(luxx);
      Serial.print(" Temp: "); Serial.print(tempp); Serial.print(" C");
      Serial.print(" Presion: "); Serial.print(pres); Serial.print("mb");
      Serial.print(" Humedad: "); Serial.print(humedad); Serial.println("%");

      String cmd = "AT+CIPSTART=\"TCP\",\"";
      cmd += server;
      cmd += "\",80";
      BT1.println(cmd);
      delay(2000);
      if (BT1.find("Error")) {
        Serial.println("Error en wifi");
        return;
      }

  String     post = "field1="; 
      post += mp25;
      post += "&field2=";
      post += tempp;
      post += "&field3=";  
      post += humedad;
      post += "&field4=";  
      post += pres;
      post += "&field5=";  
      post += luxx;
      post += "\r\n";
      
      //Serial.println(post);

      cmd = "POST /update HTTP/1.1\n"; 
     cmd += "Host: api.thingspeak.com\n"; 
     cmd += "Connection: close\n"; 
     cmd += "X-THINGSPEAKAPIKEY: "+apiKey+"\n"; 
     cmd += "Content-Type: application/x-www-form-urlencoded\n"; 
     
     String cmd2 = "Content-length: ";
     cmd2 += String(post.length())+"\n\n";

     Serial.print(cmd);
     Serial.print(cmd2);
     Serial.println(post);

     int largo = cmd.length()+cmd2.length()+post.length();
     Serial.println(largo);
     
      BT1.print("AT+CIPSEND="+String(largo)+"\r\n");
      
      if (BT1.find(">")) {
        BT1.print(cmd);
        BT1.print(cmd2);
        BT1.print(post);
      }
      else {
        BT1.println("AT+CIPCLOSE");
      }

/*
    }
    BT1.print(c);
  }
*/

  delay(60000);
  
}



double get_temp() {
  char status;
  double T;

  status = bmp180_sensor.startTemperature();

  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);

    status = bmp180_sensor.getTemperature(T);
  } else Serial.println("error starting temperature measurement\n");

  return T;
}

double get_presion(double T) {
  char status;
  double P;

  status = bmp180_sensor.startPressure(3); // 3, alta presición

  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);

    // Retrieve the completed temperature measurement:
    // Note that the measurement is stored in the variable T.
    // Function returns 1 if successful, 0 if failure.

    status = bmp180_sensor.getPressure(P, T);
    if (status == 0)
    {
      Serial.println("error retrieving pressure measurement\n");
    }
  } else Serial.println("error starting pressure measurement\n");

  return P;
}


double getlux()
{
  double final_lux;

  boolean gain = 0;     // Gain setting, 0 = X1, 1 = X16;
  unsigned int ms = 17;  // Integration ("shutter") time in milliseconds


  // If time = 0, integration will be 13.7ms
  // If time = 1, integration will be 101ms
  // If time = 2, integration will be 402ms
  // If time = 3, use manual start / stop to perform your own integration


  light_sensor.setPowerUp();
  light_sensor.setTiming(gain, 3, ms);

  // This sketch uses the TSL2561's built-in integration timer.
  // You can also perform your own manual integration timing
  // by setting "time" to 3 (manual) in setTiming(),
  // then performing a manualStart() and a manualStop() as in the below
  // commented statements:

  ms = 13.7;
  light_sensor.manualStart();
  delay(ms);
  light_sensor.manualStop();

  // There are two light sensors on the device, one for visible light
  // and one for infrared. Both sensors are needed for lux calculations.


  unsigned int data0, data1;

  if (light_sensor.getData(data0, data1))
  {
    // getData() returned true, communication was successful

    //   Serial.print("data0(VISIBLE): ");
    //  Serial.print(data0);
    // Serial.print(" data1(INFRAROJA): ");
    // Serial.print(data1);

    // The getLux() function will return 1 if the calculation
    // was successful, or 0 if one or both of the sensors was
    // saturated (too much light). If this happens, you can
    // reduce the integration time and/or gain.

    double lux;    // Resulting lux value
    boolean good;  // True if neither sensor is saturated

    // Perform lux calculation:

    good = light_sensor.getLux(gain, ms, data0, data1, lux);
    final_lux = lux;
  }
  else
  {
    byte error = light_sensor.getError();
    Serial.println("Eror en el sensor de luz.");
    final_lux = 0.0;
  }

  light_sensor.setPowerDown();

  return final_lux;

}
