/*
 * ----------------------------------
 *             MFRC522      Node     
 *             Reader/PCD   MCU      
 * Signal      Pin          Pin      
 * ----------------------------------
 * RST/Reset   RST          D1 (GPIO5)        
 * SPI SS      SDA(SS)      D2 (GPIO4)       
 * SPI MOSI    MOSI         D7 (GPIO13)
 * SPI MISO    MISO         D6 (GPIO12)
 * SPI SCK     SCK          D5 (GPIO14)
 * 3.3V        3.3V         3.3V
 * GND         GND          GND
 */


#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>//I2C LCD

//Library RFID
#include <SPI.h>//Serial Peripheral Interface
#include <MFRC522.h>

//Library NTP (Network Time Protocol)
#include <NTPClient.h>//Server NTP sebagai Client Nodemcu ESP8266
#include <WiFiUdp.h>//Port UDP ke Server NTP
#include <Servo.h>

//Servo
Servo doork;

LiquidCrystal_I2C lcd(0x27,16,2);//SDA dan SDL

const char* ssid = "ICE.OFFICIAL";
const char* password = "elektro.indo";

//Pengaturan pin data dan reset RFID
constexpr uint8_t RST_PIN = 2;     // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = 15;     // Configurable, see typical pin layout above

//RFID
MFRC522 mfrc522(SS_PIN, RST_PIN);

int readsuccess;
byte readcard[4];
char str[32] = "";
String StrUID;

//URL API
//String url = "http://192.168.43.243/Skut_Bandung/public/api/";
String url = "http://192.168.43.44/Skut_Bandung/public/api/";
//String url = "http://192.168.43.198/Skut_Bandung/public/api/";
String id_perangkat = "45170845";
//String id_perangkat = "46170846";
//int harga_perangkat = 15000;
int harga_perangkat = 20000;

int saldo_tampung;

//NTP (Network Time Protocol)
/*
UTC +07.00 : 7 * 60 * 60 : 25200 Waktu Indonesia Barat (WIB)
UTC +08.00 : 8 * 60 * 60 : 28800 Waktu Indonesia Tengah (WITA)
UTC +09.00 : 9 * 60 * 60 : 32400 Waktu Indonesia Timur (WIT)
*/

const long utcOffsetInSeconds = 25200;  // set offset

char daysOfTheWeek[7][12] = {"Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jum'at", "Sabtu"};  // Indonesian
//char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};  // English

// Define NTP Client to get time
/*
Area_____________________________________________________HostName
Worldwide_______________________________________________ pool.ntp.org
Asia____________________________________________________ asia.pool.ntp.org
Europe__________________________________________________ europe.pool.ntp.org
North America___________________________________________ north-america.pool.ntp.org
Oceania_________________________________________________ oceania.pool.ntp.org
South America___________________________________________ south-america.pool.ntp.org
*/

WiFiUDP ntpUDP;// port UDP untuk mengambil data di Network Time Protokol
NTPClient timeClient(ntpUDP, "asia.pool.ntp.org", utcOffsetInSeconds);//Dari Server NTP sebagai Client pada ESP8266

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  //LCD Display
  lcd.begin();
  lcd.home();
  //lcd.init();                      
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Skut Bandung");
  lcd.setCursor(0,1);
  lcd.print("Koneksikan Alat");

  //RFID
  SPI.begin(); // Inisialisasi SPI bus
  mfrc522.PCD_Init(); // Inisialisasi MFRC522

  doork.attach(D8);

  while (WiFi.status() != WL_CONNECTED) {

    delay(1000);
    Serial.println("Connecting..");

  }
  
  //NTP (Network Time Protocol)
  timeClient.begin();
}

void loop() {
  //NTP (Network Time Protocol)
  timeClient.update();
  doork.write(0);
  //LCD Display
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(daysOfTheWeek[timeClient.getDay()]);
  lcd.print(", ");
  lcd.print(timeClient.getFormattedTime());
  lcd.setCursor(0,1);
  lcd.print("Dekatkan Kartu!");

  if (WiFi.status() == WL_CONNECTED) {
      //Sukses pembacaan id
      readsuccess = getid();
      if(readsuccess){
        //Cetak pembacaan id
        Serial.println(StrUID);
        Serial.println("Berhasil");
          
        //Ubah tipe data id menjadi string
        String id_kartu = StrUID; 

        //LCD Display
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("UID : " + id_kartu);
        lcd.setCursor(0,1);
        lcd.print("Verifikasi Data");
        delay(1000);

        HTTPClient http;

        http.begin(url+"transaksi/"+id_kartu);//Akses api database
        int httpCode = http.GET();
  
        if (httpCode > 0) {
          //Parsing data JSON
          int nilai = 0;
          
          char json[500];
          String payload = http.getString();
          payload.toCharArray(json, 500);
          StaticJsonDocument<200> doc;
          deserializeJson(doc, json);
          Serial.println(payload);
          
          String datas = doc["transaksi"][nilai];
          String id = doc["transaksi"][nilai]["id"];
          String id_user = doc["transaksi"][nilai]["id_user"];
          String id_destinasi = doc["transaksi"][nilai]["id_destinasi"];
          String statusk = doc["transaksi"][nilai]["status"];
          
          Serial.println(datas);
          Serial.println(id_user);
          Serial.println(id_destinasi);
          Serial.println(statusk);
  
          delay(500);
          if(datas=="null"){
            Serial.println("Belum Melakukan Transaksi");
            //-----------------------------------------------------------
             String jsontransaksik;
             HTTPClient httptransaksik;
             httptransaksik.begin(url+"transaksi"); 
             httptransaksik.addHeader("Content-Type", "application/json");
             const size_t CAPACITY = JSON_OBJECT_SIZE(7);
             StaticJsonDocument<CAPACITY> dock;
                  
             JsonObject objk = dock.to<JsonObject>();
             objk["uid"] = id_kartu; 
             objk["id_destinasi"] = id_perangkat;   
             objk["status"] = "1";
                  
             serializeJson(dock, jsontransaksik);
             Serial.println(jsontransaksik);
                                          
             httptransaksik.POST(String(jsontransaksik));
                                          
             Serial.println("Post Akses");
             Serial.println(httptransaksik.getString());
             httptransaksik.end();
            HTTPClient httpuserx;
            httpuserx.begin(url+"users/"+id_kartu);//Akses api database
            int httpuserCodex = httpuserx.GET();
            if (httpuserCodex > 0) {
              //Parsing data JSON
              char jsonuserx[500];
              String payloaduserx = httpuserx.getString();
              payloaduserx.toCharArray(jsonuserx, 1000);
              StaticJsonDocument<500> docuserx;
              deserializeJson(docuserx, jsonuserx);
              Serial.println(payloaduserx);
    
              String datax = docuserx["user"][0];
              String usernamex = docuserx["user"][0]["username"];
              String uidx = docuserx["user"][0]["uid"];
              int saldox = docuserx["user"][0]["saldo"];
              Serial.println(datax);
              Serial.println(usernamex);
              Serial.println(uidx);
              Serial.println(saldox);
              
                if(datax=="null"){
                  //LCD Display
                  lcd.clear();
                  lcd.setCursor(0,0);
                  lcd.print("ID, "+id_kartu);
                  lcd.setCursor(0,1);
                  lcd.print("Belum Terdaftar");
                }else if(saldox < harga_perangkat){
                  //LCD Display
                  lcd.clear();
                  lcd.setCursor(0,0);
                  lcd.print("Halo, "+usernamex);
                  lcd.setCursor(0,1);
                  lcd.print("Saldo Kurang");
                  delay(500);
                  //LCD Display
                  lcd.clear();
                  lcd.setCursor(0,0);
                  lcd.print("Sisa Saldo :");
                  lcd.setCursor(0,1);
                  lcd.print("Rp."+String(saldox));
                }else{
                  if(id_kartu==uidx){
                    //LCD Display
                    lcd.clear();
                    lcd.setCursor(0,0);
                    lcd.print("Halo, "+usernamex);
                    lcd.setCursor(0,1);
                    lcd.print("Silahkan Masuk");
                    delay(500);
                    //LCD Display
                    lcd.clear();
                    lcd.setCursor(0,0);
                    lcd.print("Sisa Saldo :");
                    lcd.setCursor(0,1);
                    lcd.print("Rp."+String(saldox));
                    doork.write(90);
                    delay(500);
                  }else if(datax=="null"){
                    //LCD Display
                    lcd.clear();
                    lcd.setCursor(0,0);
                    lcd.print("ID, "+id_kartu);
                    lcd.setCursor(0,1);
                    lcd.print("Belum Terdaftar");
                  }
                  
                }
            }
            delay(1000);
            httpuserx.end();
            //-----------------------------------------------------------
            
          }else{
            if(id_perangkat==id_destinasi){
              HTTPClient httpuser;
              httpuser.begin(url+"user/"+id_user);//Akses api database
              int httpuserCode = httpuser.GET();
              if (httpuserCode > 0) {
                //Parsing data JSON
                char jsonuser[500];
                String payloaduser = httpuser.getString();
                payloaduser.toCharArray(jsonuser, 1000);
                StaticJsonDocument<500> docuser;
                deserializeJson(docuser, jsonuser);
                Serial.println(payloaduser);
    
                String datak = docuser["user"][0];
                String username = docuser["user"][0]["username"];
                int saldo = docuser["user"][0]["saldo"];
                Serial.println(datak);
                Serial.println(username);
                delay(500);
    
                if(statusk == "1"){
                  //LCD Display
                  lcd.clear();
                  lcd.setCursor(0,0);
                  lcd.print("ID Transaksi");
                  lcd.setCursor(0,1);
                  lcd.print("Sudah Terpakai");
                  delay(1000);
                }else if(statusk == "0"){
                  //LCD Display
                  lcd.clear();
                  lcd.setCursor(0,0);
                  lcd.print("Halo, "+username);
                  lcd.setCursor(0,1);
                  lcd.print("Silahkan Masuk");
                  delay(500);
                  //LCD Display
                  lcd.clear();
                  lcd.setCursor(0,0);
                  lcd.print("Sisa Saldo :");
                  lcd.setCursor(0,1);
                  lcd.print("Rp."+String(saldo));
                  doork.write(90);
                  delay(500);
                  //-------------------------------------------
                  String jsonOutput;
                  HTTPClient httpsend;
                  httpsend.begin(url+"transaksi/"+id); 
                  httpsend.addHeader("Content-Type", "application/json");
                      
                  const size_t CAPACITY = JSON_OBJECT_SIZE(1);
                  StaticJsonDocument<CAPACITY> docc;
                      
                  JsonObject obj = docc.to<JsonObject>();
                      
                  obj["status"] = "1";
                        
                  serializeJson(docc, jsonOutput);
                  Serial.println(jsonOutput);
                        
                  httpsend.PUT(String(jsonOutput));
                        
                  Serial.println("Put Akses");
                  Serial.println(httpsend.getString());
                  //--------------------------------------------
                   
                  delay(500);
                  httpsend.end();
                }
              }
              httpuser.end();
            }else{
              //LCD Display
              lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("ID Destinasi");
              lcd.setCursor(0,1);
              lcd.print("Tidak Sesuai");
              delay(500);
            }
          }
        }
         http.end(); 
      }
  }
  delay(500);
}

//Ambil id untuk verifikasi
int getid(){ 
  if(!mfrc522.PICC_IsNewCardPresent()){
    return 0;
  }
  if(!mfrc522.PICC_ReadCardSerial()){
    return 0;
  }
  
  Serial.println("Data Kartu:");
  
  for(int i=0;i<4;i++){
    readcard[i]=mfrc522.uid.uidByte[i]; //Kirim id card yang dibaca
    array_to_string(readcard, 4, str);
    StrUID = str;// id yang telah diolah
  }
  mfrc522.PICC_HaltA();
  return 1;
}
// --------------------------------------------------------------------
//Mengolah frekuensi tag kartu
void array_to_string(byte array[], unsigned int len, char buffer[])
{
    for (unsigned int i = 0; i < len; i++)
    {
        byte nib1 = (array[i] >> 4) & 0x0F;
        byte nib2 = (array[i] >> 0) & 0x0F;
        buffer[i*2+0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
        buffer[i*2+1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
    }
    buffer[len*2] = '\0';
}
