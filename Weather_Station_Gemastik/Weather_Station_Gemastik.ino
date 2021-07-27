#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "DHT.h"
#include <U8g2lib.h>
#include <U8x8lib.h>
#include <qrcode.h>
#include <SPI.h>
#include <Servo.h>

//Servo
Servo doork;

#define SIZE  3

U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // OLEDs without Reset of the Display
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // All Boards without Reset of the Display

QRCode qrcode;

const char* ssid = "ICE.OFFICIAL";
const char* password = "elektro.indo";

//URL API
//String url = "http://192.168.43.243/Skut_Bandung/public/api/";
String url = "http://192.168.43.44/Skut_Bandung/public/api/";
//String url = "http://192.168.43.198/Skut_Bandung/public/api/";
String id_perangkat = "45170845";
char* perangkat = "45170845";

//DHT
#define DHTPIN D4 // D4
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);

#define indikator D5 //D5

//Sensor Jarak
#define trigPin D6//D5
#define echoPin D7//D6

int jumlah_pengunjung;

void setup() {
  Serial.begin(115200);
  u8x8.begin();
  u8x8.setPowerSave(0);
  u8g2.begin();
  show_qrcode(perangkat);
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(9,0,"Skut");
  u8x8.drawString(9,1,"Bandung");
  u8x8.drawString(9,3,"Noctur");
  u8x8.drawString(9,4,"nailed");
  u8x8.drawString(9,6,"STTB");
  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {

    delay(1000);
    Serial.println("Connecting..");

  }
  dht.begin();
  doork.attach(D8);
  //Sensor Jarak
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(indikator, OUTPUT);

  
}

void loop() {
    
    //u8x8.drawString(8,7,"Gemastik");
//  u8x8.drawString(0,1,"Nocturnailed");
//  u8x8.refreshDisplay();    // only required for SSD1606/7  
//  delay(2000);

//  String rx_buf;
//  rx_buf = Serial.readString();
//
//  if (0 == memcmp(rx_buf.c_str(), "qrcode:", 7)) {
//    show_qrcode(&rx_buf.c_str()[7]);
//  }
  doork.write(0);
  if (WiFi.status() == WL_CONNECTED) {
    //-----------------------------------------------------------
    float temp = dht.readTemperature();
    float humd = dht.readHumidity();
    
    String jsoncuaca;
    HTTPClient httpcuaca;
    httpcuaca.begin(url+"cuaca"); 
    httpcuaca.addHeader("Content-Type", "application/json");
    const size_t CAPACITY = JSON_OBJECT_SIZE(8);
    StaticJsonDocument<CAPACITY> dock;
  
    JsonObject objk = dock.to<JsonObject>();       
    objk["id_destinasi"] = id_perangkat;   
    objk["suhu"] = String(temp);
    objk["kelembapan"] = String(humd);
  
    serializeJson(dock, jsoncuaca);
    Serial.println(jsoncuaca);
                          
    httpcuaca.POST(String(jsoncuaca));
                          
    Serial.println("Post Akses");
    Serial.println(httpcuaca.getString());
    //digitalWrite(indikator, HIGH);
    delay(1000);
    digitalWrite(indikator, LOW);
    delay(500);
    //digitalWrite(indikator, HIGH);
    delay(1000);
    digitalWrite(indikator, LOW);
    delay(500);
    //digitalWrite(indikator, HIGH);
    delay(1000);
    digitalWrite(indikator, LOW);
    delay(500);
    //-----------------------------------------------------------
    httpcuaca.end();

    long durasi, jarak;
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    durasi = pulseIn(echoPin, HIGH);
    jarak = (durasi/2) / 29.1;
  
    Serial.print("Distance: ");
    Serial.print(jarak);
    Serial.println(" CM");
    
    //Akses Keluar
    if(jarak < 30){
      HTTPClient http;

      http.begin(url+"destinasi/"+id_perangkat);//Akses api database
      int httpCode = http.GET();
  
      if (httpCode > 0) {
         //Parsing data JSON
         char json[500];
         String payload = http.getString();
         payload.toCharArray(json, 1000);
         StaticJsonDocument<500> doc;
         deserializeJson(doc, json);
         Serial.println(payload);

         String datas = doc["destinasi"][0];
         jumlah_pengunjung = doc["destinasi"][0]["jumlah_pengunjung"];
      }
      http.end();
      //-------------------------------------------
      String jsonOutput;
      HTTPClient httpsend;
      httpsend.begin(url+"destinasi/"+id_perangkat); 
      httpsend.addHeader("Content-Type", "application/json");
                      
      const size_t CAPACITY = JSON_OBJECT_SIZE(1);
      StaticJsonDocument<CAPACITY> docc;
                      
      JsonObject obj = docc.to<JsonObject>();
      if(jumlah_pengunjung == 0){
        obj["jumlah_pengunjung"] = 0;
        
        serializeJson(docc, jsonOutput);
        Serial.println(jsonOutput);
                          
        httpsend.PUT(String(jsonOutput));
                          
        Serial.println("Put Akses");
        Serial.println(httpsend.getString());
      }else{
        obj["jumlah_pengunjung"] = jumlah_pengunjung - 1;
        
        serializeJson(docc, jsonOutput);
        Serial.println(jsonOutput);
                          
        httpsend.PUT(String(jsonOutput));
                          
        Serial.println("Put Akses");
        Serial.println(httpsend.getString());
        doork.write(90);
        delay(1000);
      }
      //--------------------------------------------
      httpsend.end();
      digitalWrite(indikator, HIGH);
      delay(1000);
      digitalWrite(indikator, LOW);
      doork.write(90);
      
    }else{
      digitalWrite(indikator, LOW);
      doork.write(0);
    }
  }
  digitalWrite(indikator, LOW);
}

void show_qrcode(char *str_qrcode)
{
  uint8_t qrcodeData[qrcode_getBufferSize(SIZE)];
  qrcode_initText(&qrcode, qrcodeData, SIZE , ECC_LOW, str_qrcode);

  Serial.println(str_qrcode);

  // start draw
  u8g2.firstPage();
  do {
    // get the draw starting point,128 and 64 is screen size
    uint8_t x0 = 3;
    uint8_t y0 = 3;
    
    // get QR code pixels in a loop
    for (uint8_t y = 0; y < qrcode.size; y ++) {
      for (uint8_t x = 0; x < qrcode.size; x ++) {
        // Check this point is black or white
        if (qrcode_getModule(&qrcode, x, y)) {
          u8g2.setColorIndex(1);
        } else {
          u8g2.setColorIndex(0);
        }
        // draw the QR code pixels, double it
        u8g2.drawPixel(x0 + x * 2    , y0 + y * 2);
        u8g2.drawPixel(x0 + x * 2 + 1, y0 + y * 2);
        u8g2.drawPixel(x0 + x * 2    , y0 + y * 2 + 1);
        u8g2.drawPixel(x0 + x * 2 + 1, y0 + y * 2 + 1);
      }
    }
  } while (u8g2.nextPage());
}
