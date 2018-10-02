/*
   LACOSOX MP2.5 TEST

   lee la contaminación por MP2.5 del sensor sharpp GP2Y1014 y lo envia a Thingspeak.
 
   Actualmente almacenando datos a: 
   https://thingspeak.com/channels/217483
   
*/
#include <SoftwareSerial.h>
#include <string.h>
#include <SPI.h>

int measurePin = A0;
int ledPower = 2;

unsigned int samplingTime = 280;
unsigned int deltaTime = 40;
unsigned int sleepTime = 9680;//9680;

int voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;
float footDensity = 0;
float daniloteDensity= 0;

//SoftwareSerial Serial1(19, 18); // RX | TX
const char* server = "api.thingspeak.com";
String apiKey = "XXXXXXXX";

unsigned int tiempo_muestreo=60000; // cada 1 min. 
unsigned int tiempo_por_muestreo=30000; // cada muestra tomara 30 segundos de datos. 
unsigned int tiempo_entre_datos=1000; // tiempo de descanso entre cada dato. 

void setup(){
  Serial.begin(9600);
  Serial1.begin(9600);
  
  pinMode(ledPower,OUTPUT);

  Serial1.println("AT+RST");
  delay(1000);
}

float get_voltage() {
  digitalWrite(ledPower,LOW);
  delayMicroseconds(samplingTime);

  voMeasured = analogRead(measurePin);

  delayMicroseconds(deltaTime);
  digitalWrite(ledPower,HIGH);
  delayMicroseconds(sleepTime);

  calcVoltage = (voMeasured/1023.0)*5.0;
  return calcVoltage;
}

float get_voltage_avg(unsigned long sampletime_ms) {
  unsigned long  starttime = millis();
  unsigned long endtime = 0;
  unsigned int contador = 0;
  boolean salir = false;
  float voltage_avg = 0.0;

  do {
       endtime = millis();
       contador ++;
       
       voltage_avg= voltage_avg + get_voltage();
       Serial.print(".");
       // Serial.print("Voltage avg_t:");
  //Serial.print(voltage_avg);
  //Serial.print("V   ");

   //Serial.print("contador = ");
   //Serial.println(contador);
       
      if ((endtime - starttime) > sampletime_ms)
      {
         voltage_avg = voltage_avg/contador;
         Serial.print("Muestras = ");
         Serial.println(contador);
         salir = true;
      }
   delay(tiempo_entre_datos);

  }while (!salir);
  
  return voltage_avg;
}

float get_MP25(float voltage) {
  
  dustDensity = 0.172*voltage-0.0999;
  footDensity = (voltage-0.0356)*120000;
  daniloteDensity = (voltage-0.800)/6;

  if (dustDensity < 0)
  {
    dustDensity = 0.00;
  }
  
  return dustDensity;
}



void loop(){

 Serial.println("Tomando muestras...");

 float raw_voltage = get_voltage_avg(tiempo_por_muestreo);
 float mp25_ugm3_avg=get_MP25(raw_voltage)*1000;
 
 Serial.print("Voltage AVG:");
  Serial.print(raw_voltage);
  Serial.print("V   ");

  Serial.print("MP2.5:");
  Serial.print(mp25_ugm3_avg);
  Serial.println("ug/m3:");


  String cmd = "AT+CIPSTART=\"TCP\",\"";
      cmd += server;
      cmd += "\",80";
      Serial1.println(cmd);
      delay(2000);
      if (Serial1.find("Error")) {
        Serial.println("Error en wifi");
        return;
      }
  String     post = "field8="; 
      post += mp25_ugm3_avg;
      post += "\r\n";
      
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
     
      Serial1.print("AT+CIPSEND="+String(largo)+"\r\n");
      
      if (Serial1.find(">")) {
        Serial1.print(cmd);
        Serial1.print(cmd2);
        Serial1.print(post);
        Serial.println(">comandos enviados.");
      }
      else {
        Serial1.println("AT+CIPCLOSE");
        Serial.println(">conexion cerrada.");
      }

  Serial.println("siguiente muestra en: "+String(tiempo_muestreo)+"ms");
  delay(tiempo_muestreo);
}


