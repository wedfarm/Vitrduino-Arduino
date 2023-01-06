#include <Arduino.h>
#include <DHT.h>
#include <Wire.h>
#include <BH1750.h>
#include <ML8511.h>
#include <SPI.h>
#include <RF24.h>
#include <SoftwareSerial.h>
SoftwareSerial esp01(3,2);
#define pinOut A2
#define pinRef3V3 A3
#define pinTMP 4
#define DEBUG true 
#define typDHT21 DHT21

BH1750 sensorLUX;

ML8511 sensorUV(pinOut, pinRef3V3);

DHT sensorTMP(pinTMP, typDHT21);

const int POCET = 3; // pocet mereni

float prumerTMP[POCET];
float prumerHUM[POCET];
float prumerLUX[POCET];
float prumerUV[POCET];

float tep;
float vlh;
float lux;
float uv;

const unsigned int eventInterval = 14000;
unsigned long previousTime = 0;

RF24 radio(7, 8); // CE, CSN

const byte prijimac[] = "00001";
const byte vysilac[] = "00002";

int writeNum = 0;

void setup() {
  delay(1000);
  Wire.begin();
  Serial.begin(9600);
  esp01.begin(115200);

  pinMode(5, OUTPUT);

  sensorLUX.begin();
  sensorTMP.begin();

  radio.begin();
  radio.openReadingPipe(1, vysilac);
  radio.openWritingPipe(prijimac);
  radio.setPALevel(RF24_PA_MIN);
  
  delay(1000);
  sendData("AT+CWQAP\r\n", 1500, DEBUG);
  delay(1000);
  sendData("AT+RST\r\n", 2000, DEBUG);                     
  delay(3000);
  sendData("AT+CWMODE=1\r\n", 1500, DEBUG);
  delay(1000);
  sendData("AT+CIPMUX=0\r\n", 1500, DEBUG);
  delay(1000);
  sendData("AT+CWJAP=\"vitrduino\",\"raspberry\"\r\n", 2000, DEBUG);
  delay(5000);
  sendData("AT+CIFSR\r\n", 1500, DEBUG);
  delay(3000);
  
}

void loop() {
  pauza();
  radioLoopBack();
}

//nacte hodnoty ze sensoru do pole tolikrat kolik je hodnota POCET, poté se vytvoří 
void readValues(){
  for (int i = 0; i < POCET; i++){
      tep = sensorTMP.readTemperature()*1.022;
      vlh = sensorTMP.readHumidity()*0.814;
      lux = sensorLUX.readLightLevel();
      uv = sensorUV.getUV();

      prumerTMP[i] = tep;
      prumerHUM[i] = vlh;
      prumerLUX[i] = lux;
      prumerUV[i] = uv;
      }
      
      tep = prumer(prumerTMP, POCET);
      vlh = prumer(prumerHUM, POCET);
      lux = prumer(prumerLUX, POCET);
      uv = prumer(prumerUV, POCET);
    }

//funkce pro prumer namerenych hodnot ze vsech sensoru, const POCET udava pocet mereni
float prumer(float* prumerDat, const int POCET){
    float soucet = 0;
    for (int i = 0; i < POCET; i++) {
        soucet = soucet + prumerDat[i];
    }
    return soucet / POCET;
  }

//Prirazeni hodnot, prirazeni hodnot do pole,
void sendValues(){
  String getrequest="GET /insertData?temp="+String(tep)+"&hum="+String(vlh)+"&bmp="+String("0")+"&uv="+String(uv)+"&lux="+String(lux)+"&pass="+String("4xnA8kvLbgb2beGk")+"&boardid="+String("2")+" HTTP/1.1\r\nHost: 10.20.1.1\r\n\r\n";
  sendData("AT+CIPSTART=\"TCP\",\"10.20.1.1\",5000\r\n", 1500, DEBUG);
  sendData("AT+CIPSEND="+String(getrequest.length())+"\r\n", 1500, DEBUG);
  sendData(getrequest, 1500, DEBUG);
}

//Description: Function used to send data to ESP8266.
//Params: command - the data/command to send; timeout - the time to wait for a response; debug - print to Serial window?(true = yes, false = no)
//Returns: The response from the esp8266 (if there is a reponse)
String sendData(String command, const int timeout, boolean debug)
{   
    String response = "";
    esp01.print(command);
    long int time = millis();
    while( (time+timeout) > millis())
    {
      while(esp01.available())
      {
        char c = esp01.read();
        response+=c;    
      }
    }
    if(debug)  
    {
      Serial.print(response);
    }
    return response;
}

void pauza(){
  unsigned long currentTime = millis();
  digitalWrite(5, HIGH);
  //načte aktuální čas v milisec od začátku kodu
  if (currentTime - previousTime >= eventInterval) {
    digitalWrite(5, LOW); 
    //provede funkce
    readValues();
    sendValues();
    //zapíše currentTime do previousTime a pak to znova porovná
    previousTime = currentTime;
  }
}

void radioLoopBack(){
  delay(5);
  radio.startListening();
  if(radio.available()) {
      radio.read(&writeNum, sizeof(writeNum));
      Serial.println(writeNum);
      }
  delay(5);
  radio.stopListening();
  if(!radio.available() && writeNum == 1) {
    float hodnoty[] = {1,tep,vlh,lux,uv};
    radio.write(&hodnoty, sizeof(hodnoty));
  }
  writeNum = 0;
  }
