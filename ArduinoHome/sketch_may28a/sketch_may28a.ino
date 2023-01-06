
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <RF24.h>
#include <SparkFunHTU21D.h>
#include <Adafruit_BMP085.h>
#include <U8g2lib.h>
#include <ezButton.h>
#include <SoftwareSerial.h>
SoftwareSerial esp01(3,2);
#define DEBUG true 

ezButton button(4);

U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

unsigned long previousTime = 0;

RF24 radio(7, 8); // CE, CSN

const byte prijimac[] = "00001";
const byte vysilac[] = "00002";

float hodnoty[5];

HTU21D sensorTMP;

Adafruit_BMP085 sensorBMP;

const int POCET = 3; // pocet mereni

float prumerTMP[POCET];
float prumerHUM[POCET];
float prumerBMP[POCET];

float tep;
float vlh;
int bmp;

const int led = 5;

void setup() {
  delay(1000);
  Wire.begin();
  button.setDebounceTime(100);
  Serial.begin(9600);
  esp01.begin(115200);

  pinMode(led, OUTPUT);
  
  sensorTMP.begin();
  sensorBMP.begin();

  u8g2.begin();
  u8g2.setFont(u8g2_font_8x13_tf);

  radio.begin();
  radio.openReadingPipe(1, prijimac); // 00001
  radio.openWritingPipe(vysilac); // 00002
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
  pauza(14000);
  radioLoopBack();
}

//nacte hodnoty ze sensoru do pole tolikrat kolik je hodnota POCET, poté se vytvoří 
void readValues(){
  for (int i = 0; i < POCET; i++){
      tep = sensorTMP.readTemperature();
      vlh = sensorTMP.readHumidity();
      bmp = sensorBMP.readPressure()/100;
      
      prumerTMP[i] = tep;
      prumerHUM[i] = vlh;
      prumerBMP[i] = bmp;
      }
      
      tep = prumer(prumerTMP, POCET);
      vlh = prumer(prumerHUM, POCET);
      bmp = prumer(prumerBMP, POCET);
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
  String getrequest="GET /insertData?temp="+String(tep)+"&hum="+String(vlh)+"&bmp="+String(bmp)+"&uv="+String("0")+"&lux="+String("0")+"&pass="+String("4xnA8kvLbgb2beGk")+"&boardid="+String("1")+" HTTP/1.1\r\nHost: 10.20.1.1\r\n\r\n";
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

int pauza(unsigned int eventInterval){
  unsigned long currentTime = millis();
  digitalWrite(led, HIGH);
  //načte aktuální čas v milisec od začátku kodu
  if (currentTime - previousTime >= eventInterval) {
    digitalWrite(led, LOW); 
    //provede funkce
    readValues();
    sendValues();
    showDisplay();
    //zapíše currentTime do previousTime a pak to znova porovná
    previousTime = currentTime;
  }
}

void radioLoopBack(){
  button.loop();
      if(button.isPressed()){
        radio.stopListening();
        int writeNum = 1;
        radio.write(&writeNum, sizeof(writeNum));
      }else{
        radio.startListening();
        if(radio.available()){
          radio.read(&hodnoty, sizeof(hodnoty));
          for(int i = 0; i<5;i++){
            Serial.println(hodnoty[i]);
            }
            showDisplay();
            pauza(15000);
            hodnoty[0]=0;         
        }
    } 
  }

void venku(void) {  
  u8g2.drawStr(0, 13, "Teplota");
  u8g2.setCursor(80, 13);
  u8g2.print(hodnoty[1]);
  u8g2.drawStr(0, 29, "Vlhkost");
  u8g2.setCursor(80, 29);
  u8g2.print(hodnoty[2]);
  u8g2.drawStr(0, 45, "Svetlo");
  u8g2.setCursor(80, 45);
  u8g2.print(hodnoty[3]);
  u8g2.drawStr(0, 61, "UV zareni");
  u8g2.setCursor(80, 61);
  u8g2.print(hodnoty[4]);
}
void vnitrek(void){
  u8g2.drawStr(0, 20, "Teplota");
  u8g2.setCursor(80, 20);
  u8g2.print(tep);
  u8g2.drawStr(0, 40, "Vlhkost");
  u8g2.setCursor(80, 40);
  u8g2.print(vlh);
  u8g2.drawStr(0, 60, "Tlak");
  u8g2.setCursor(80, 60);
  u8g2.print(bmp);
  }

void showDisplay(){
  u8g2.firstPage();  
  do {
    if(hodnoty[0] == 1){
      venku();
      }else{
        vnitrek();
        }
  } while( u8g2.nextPage() );
  delay(500);
  }
