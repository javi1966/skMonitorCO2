#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "MQ135.h"

/*MQ-7 normal air output  150
     Isopropile alcohol 750
     Lighter Gas        900
     Benzine            800
     Breath1            170
     Breath2            160

  MQ-135 normal air output 130
      Isopropile alcohol 700
      Lighter Gas        760
      Benzine            450
      Breath1            150
      Breath2            140

*/

#define LED_VERDE    D0
#define ALTAVOZ      D1
#define LED          D2
#define DO_MQ7       D7
#define LED_ROJO     D8
#define ANALOGPIN    A0

#define JSON_BUFF_DIMENSION 2500


//const char ssid[]     = "Wireless-N";
//const char password[] = "z123456z";
const char ssid[] = "MOVISTAR_B855";             //  your network SSID (name)
const char password[] = "AD2890A7F423CBF3BB79";
const char* host = "api.thingspeak.com";


const int timerUpdate = 60 * 60; //5 minutos
const byte interruptPin = 13;
float sensorMQ7 = 0;
bool bActualiza = true;
bool bAlarma = false;
float temperatura=0;
float humedad=0;

WiFiClient client;
AsyncWebServer server(80);

MQ135 gasSensor = MQ135(ANALOGPIN);

//*********************************************************************************
void aviso() {
  for (int i = 0; i < 10; i++) { //run loop for all values of i 0 to __
    tone(ALTAVOZ, 800); //play tones in order from the frequency array
    delay(200);   //play all above tones for the corresponding duration in the duration array
    tone(ALTAVOZ, 1000);
    delay(200);   //play all above tones for the corresponding duration in the duration array
    tone(ALTAVOZ, 1500);
    delay(200);   //play all above tones for the corresponding duration in the duration array
    noTone(ALTAVOZ);   //turn off speaker for 30ms before looping back to the next note
    delay(10);
  }
}

//****************************************************************************

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);

  digitalWrite(LED, LOW);
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(LED_VERDE, LOW);

  delay(1000);

  noInterrupts();
  timer0_isr_init();
  timer0_attachInterrupt(TimingISR);
  timer0_write(ESP.getCycleCount() + 80000000L); // 80MHz == 1sec
  interrupts();

  //pinMode(interruptPin, INPUT_PULLUP);
  //attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, FALLING);


  Serial.println(" ");
  Serial.println("Iniciando ...");

  pinMode(ANALOGPIN, INPUT);
  pinMode(DO_MQ7, INPUT);
  //pinMode(LED, OUTPUT);
  pinMode(ALTAVOZ, OUTPUT);

  digitalWrite(LED, LOW);
  digitalWrite(ALTAVOZ, LOW);

  //delay(1000);
  digitalWrite(LED, HIGH);

  //Cambio nombre
  WiFi.hostname("MonitorCO2");

  //Conexion WIFI
  WiFi.begin(ssid, password);
  WiFi.config(IPAddress(192, 168, 1, 223), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0), IPAddress(8, 8, 8, 8));

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("*");

    if (++timeout > 100)
    {
      Serial.println("Sin Conexion WIFI");
      while (1) {
        digitalWrite(LED, HIGH);
        delay(100);
        digitalWrite(LED, LOW);
        delay(100);
      }
      Serial.println("Reset SW");
      ESP.reset();
    }

  }


  Serial.println(WiFi.localIP());
  Serial.printf("Chip ID = %08X", ESP.getChipId());
  Serial.println("");

  server.on("/nivelco2", HTTP_GET, [](AsyncWebServerRequest * request) {

    float ppm = gasSensor.getCorrectedPPM(temperatura, humedad);
    Serial.println(ppm);



     if (ppm < 0)
        ppm = 0;


      if (ppm < 450) {

        digitalWrite(LED_VERDE, HIGH);
        digitalWrite(LED_ROJO, LOW);
      }
      else if (  ppm > 450 && ppm < 700 ) {
        digitalWrite(LED_VERDE, HIGH);
        digitalWrite(LED_ROJO, HIGH);
      }

      else if (  ppm > 700  ) {
        digitalWrite(LED_VERDE, LOW);
        digitalWrite(LED_ROJO, HIGH);
      }

    
    String salJSON = "{\"CO2\":";
    salJSON += String(ppm, 2);
    salJSON += "}";

    AsyncWebServerResponse *response = request->beginResponse(200, "text/json", salJSON);

    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);

    digitalWrite(LED, LOW); //flashing led
    delay(1000);
    digitalWrite(LED, HIGH);


  });

  server.begin();
  Serial.println("HTTP server started");

  delay(1000);
}



void loop() {
  // put your main code here, to run repeatedly:

  /*if (!digitalRead(DO_MQ7)) {                 //pooling
    Serial.println("Alarma Co2");
    aviso();
  }*/

  /* if (bAlarma) {
     Serial.println("Alarma Butano");
     aviso();
     bAlarma = false;
    }*/


  if (bActualiza) {

    digitalWrite(LED, LOW); //flashing led
    delay(500);
    digitalWrite(LED, HIGH);

    if (WiFi.status() == WL_CONNECTED) {

      reqOpenweather();

      if(!temperatura ==0)

         sensorMQ7 = gasSensor.getCorrectedPPM(temperatura, humedad);

      if (sensorMQ7 < 0)
        sensorMQ7 = 0;


      if (sensorMQ7 < 450) {

        digitalWrite(LED_VERDE, HIGH);
        digitalWrite(LED_ROJO, LOW);
      }
      else if (  sensorMQ7 > 450 && sensorMQ7 < 700 ) {
        digitalWrite(LED_VERDE, HIGH);
        digitalWrite(LED_ROJO, HIGH);
      }

      else if (  sensorMQ7 > 700  ) {
        digitalWrite(LED_VERDE, LOW);
        digitalWrite(LED_ROJO, HIGH);
      }



      Serial.print("MQ7: ");
      Serial.println(sensorMQ7);

      HTTPClient http;

      String field = "field1=" + String(sensorMQ7);
      Serial.println(field);
      http.begin(host, 80, "https://api.thingspeak.com/update?api_key=JMP2SOPDOP2VL5KA&" + field);

      int httpCode = http.GET();

      if (httpCode == HTTP_CODE_OK) {

        //String payload = http.getString();
        Serial.print("Resultado: ");
        Serial.println(http.getString());

      }

      http.end();
    }//WL_CONNECTED

    bActualiza = false;
  }//bActualiza

}
//**************************************************************************
void handleInterrupt()
{
  bAlarma = true;

}
//**************************************************************************

void TimingISR() {

  static int cntTemp = 0;


  if (++cntTemp > timerUpdate)
  {
    bActualiza = true;

    cntTemp = 0;
  }

  timer0_write(ESP.getCycleCount() + 80000000L); // 80MHz == 1sec

}

//**********************************************************************************************************
void reqOpenweather() {


  HTTPClient http;  //Object of class HTTPClient
  http.begin("http://api.openweathermap.org/data/2.5/weather?q=sevilla,ES&APPID=c2ecf0a83c555c2054704fd94ff29f9e&units=metric");
  int httpCode = http.GET();
  //Check the returning code
  if (httpCode > 0) {

    Serial.println(http.getString());
    const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2 * JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(13) + 270;
    DynamicJsonDocument doc(capacity);

    deserializeJson(doc, http.getString());

    // DeserializationError error = deserializeJson(doc, http.getString());

    /* if (error) {
       Serial.print(F("deserializeJson() failed: "));
       Serial.println(error.c_str());
       return;
      }*/

    JsonObject main = doc["main"];
    temperatura = main["temp"];
    humedad = main["humidity"];

    Serial.println(temperatura);
    Serial.println(humedad);

    http.end();
  }

  else {
    // if no connction was made:
    Serial.println("connection failed");
    return;
  }
}
