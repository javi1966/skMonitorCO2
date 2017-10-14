#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>


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


#define ALTAVOZ      D1
#define LED          D2
#define DO_MQ7       D7
#define ANALOGPIN    A0
#define ONE_WIRE_BUS D3

//const char ssid[]     = "Wireless-N";
//const char password[] = "z123456z";
const char ssid[] = "WLAN_BF";             //  your network SSID (name)
const char password[] = "Z404A03CF9CBF";
const char* host = "api.thingspeak.com";
String writeAPIKey = "5HN547LGT51ENDP6";

const int timerUpdate = 60 * 3; //5 minutos
const byte interruptPin = 13;
int sensorMQ7 = 0;
bool bActualiza = true;
bool bAlarma = false;
float Temperatura = 0;


WiFiClient client;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

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
  digitalWrite(LED, LOW);
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

  //Conexion WIFI
  WiFi.begin(ssid, password);

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
    }

  }

  //primera lectura
  sensorMQ7 = analogRead(ANALOGPIN);
  sensors.requestTemperatures();
  Temperatura = sensors.getTempCByIndex(0);

  if (client.connect(host, 80)) {

    // Construct API request body
    String body = "field1=";
    body +=  String(sensorMQ7);
    body += "&field2=";
    body +=  String(Temperatura);
    Serial.println(body);

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + writeAPIKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(body.length());
    client.print("\n\n");
    client.print(body);
    client.print("\n\n");

  }

  client.stop();


}

void loop() {
  // put your main code here, to run repeatedly:

  if (!digitalRead(DO_MQ7)) {                 //pooling
    Serial.println("Alarma Butano");
    aviso();
  }

  /* if (bAlarma) {
     Serial.println("Alarma Butano");
     aviso();
     bAlarma = false;
    }*/


  if (bActualiza) {


    sensorMQ7 = analogRead(ANALOGPIN);
    sensors.requestTemperatures();
    Temperatura = sensors.getTempCByIndex(0);
    Serial.print("MQ7: ");
    Serial.println(sensorMQ7);
    Serial.print("T: ");
    Serial.println(Temperatura);

    digitalWrite(LED, LOW); //flashing led
    delay(500);
    digitalWrite(LED, HIGH);

    if (client.connect(host, 80)) {

      // Construct API request body
      String body = "field1=";
      body +=  String(sensorMQ7);
      body += "&field2=";
      body +=  String(Temperatura);

      Serial.println(body);

      client.print("POST /update HTTP/1.1\n");
      client.print("Host: api.thingspeak.com\n");
      client.print("Connection: close\n");
      client.print("X-THINGSPEAKAPIKEY: " + writeAPIKey + "\n");
      client.print("Content-Type: application/x-www-form-urlencoded\n");
      client.print("Content-Length: ");
      client.print(body.length());
      client.print("\n\n");
      client.print(body);
      client.print("\n\n");

    }

    client.stop();

    bActualiza = false;
  }

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
