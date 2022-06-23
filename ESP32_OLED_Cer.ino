/**************************************************************************
 This is an example for our Monochrome OLEDs based on SSD1306 drivers

 Pick one up today in the adafruit shop!
 ------> http://www.adafruit.com/category/63_98

 This example is for a 128x32 pixel display using I2C to communicate
 3 pins are required to interface (two I2C and one reset).

 Adafruit invests time and resources providing this open
 source code, please support Adafruit and open-source
 hardware by purchasing products from Adafruit!

 Written by Limor Fried/Ladyada for Adafruit Industries,
 with contributions from the open source community.
 BSD license, check license.txt for more information
 All text above, and the splash screen below must be
 included in any redistribution.
 **************************************************************************/
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include "NRF24L01.h"
#include "API.h"
#include <HTTPClient.h>
#include <EEPROM.h>
#include <WiFiMulti.h>


WiFiMulti wifiMulti;

#ifndef LED_BUILTIN
#define LED_BUILTIN 13 // pin number is specific to your esp32 board
#endif

#define WiFi_timeout 15000  // 15sec Wifi connection timeout
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define SHDN 33

#define NRF_CS 14
#define NRF_CE 12
#define NRF_TX_ADR_WIDTH    5
#define NRF_TX_PLOAD_WIDTH  6   //3

/*
NRF24_CE    x   D12
NRF24_CSN   x   D14
NRF24_SCK   x   D18
NRF24_MISO  x   D19
NRF24_MOSI  x   D23
NRF24_Vcc   x   ESP32_3.3V
NRF24_GND   x   ESP32_GND  
*/


#define WIFI_SSID0 "neptune"
#define WIFI_PASS0 "1010101010"
#define WIFI_SSID1 "mars"
#define WIFI_PASS1 "1nn0v@t10n"
#define WIFI_SSID2 "KK04"
#define WIFI_PASS2 "22224444"

#define EEPROM_MAX_ADDR 512
#define LED 2
String HTTPDATA;

const char* fingerprint = "39 C7 07 19 7A E8 4E 67 6C 8A B1 D7 ED 00 74 11 5F 4E 9C BE";   //*.cpf.co.th


unsigned char NRF_TX_ADDRESS[NRF_TX_ADR_WIDTH]  = {0x34,0x43,0x10,0x10,0x01};
unsigned char rx_buf[NRF_TX_PLOAD_WIDTH];
unsigned char tx_buf[NRF_TX_PLOAD_WIDTH];
bool NRF_RX_Flag = false;
int intHTTPfailed = 0;



String setting_rx_mode = "NRF_MODE"; // mode 0 : LoRa , 1 : NRF
String setting_ssid1; 
String setting_pw1; 
String setting_ssid2; 
String setting_pw2; 
String setting_sever = "https://iotlogger.cpf.co.th";
String setting_port = "443"; 
String setting_url = "https://iotlogger.cpf.co.th/feedcenter/logger.php"; 
String setting_freq;
String setting_id = "0004";
String setting_interval;

bool bNoTime = true;
bool WiFi_TX_Flag = false;

int OLEDcurrentline = 0;

unsigned long interval=7200000;
// Tracks the time since last event fired
unsigned long previousMillis=0;
 





Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES     10 // Number of snowflakes in the animation example

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16
static const unsigned char PROGMEM logo_bmp[] =
{ 0b00000000, 0b11000000,
  0b00000001, 0b11000000,
  0b00000001, 0b11000000,
  0b00000011, 0b11100000,
  0b11110011, 0b11100000,
  0b11111110, 0b11111000,
  0b01111110, 0b11111111,
  0b00110011, 0b10011111,
  0b00011111, 0b11111100,
  0b00001101, 0b01110000,
  0b00011011, 0b10100000,
  0b00111111, 0b11100000,
  0b00111111, 0b11110000,
  0b01111100, 0b11110000,
  0b01110000, 0b01110000,
  0b00000000, 0b00110000 };

void setup() {
  pinMode(SHDN, OUTPUT);
  pinMode(LED,OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(SHDN, HIGH);
  delay(4000);
  Serial.begin(115200);

  delay(500);
  digitalWrite(LED,HIGH);
  delay(500);
  digitalWrite(LED,LOW);
  delay(500);
  digitalWrite(LED,HIGH);
  delay(500);
  digitalWrite(LED,LOW);
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();

  // Draw a single pixel in white
  display.drawPixel(10, 10, SSD1306_WHITE);

  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  display.display();
  delay(2000);
  // display.display() is NOT necessary after every single drawing command,
  // unless that's what you want...rather, you can batch up a bunch of
  // drawing operations and then update the screen all at once by calling
  // display.display(). These examples demonstrate both approaches...

  //testdrawline();      // Draw many lines

  testdrawrect();      // Draw rectangles (outlines)

  // Invert and restore display, pausing in-between
  display.invertDisplay(true);
  delay(1000);
  display.invertDisplay(false);
  delay(1000);
  display.clearDisplay();

  WiFiInitial();
  
  NRFInitial();
  
}

 
const char* root_ca= \
"-----BEGIN CERTIFICATE-----\n" \
"MIIGEzCCA/ugAwIBAgIQfVtRJrR2uhHbdBYLvFMNpzANBgkqhkiG9w0BAQwFADCB\n" \
"iDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0pl\n" \
"cnNleSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNV\n" \
"BAMTJVVTRVJUcnVzdCBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTgx\n" \
"MTAyMDAwMDAwWhcNMzAxMjMxMjM1OTU5WjCBjzELMAkGA1UEBhMCR0IxGzAZBgNV\n" \
"BAgTEkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4GA1UEBxMHU2FsZm9yZDEYMBYGA1UE\n" \
"ChMPU2VjdGlnbyBMaW1pdGVkMTcwNQYDVQQDEy5TZWN0aWdvIFJTQSBEb21haW4g\n" \
"VmFsaWRhdGlvbiBTZWN1cmUgU2VydmVyIENBMIIBIjANBgkqhkiG9w0BAQEFAAOC\n" \
"AQ8AMIIBCgKCAQEA1nMz1tc8INAA0hdFuNY+B6I/x0HuMjDJsGz99J/LEpgPLT+N\n" \
"TQEMgg8Xf2Iu6bhIefsWg06t1zIlk7cHv7lQP6lMw0Aq6Tn/2YHKHxYyQdqAJrkj\n" \
"eocgHuP/IJo8lURvh3UGkEC0MpMWCRAIIz7S3YcPb11RFGoKacVPAXJpz9OTTG0E\n" \
"oKMbgn6xmrntxZ7FN3ifmgg0+1YuWMQJDgZkW7w33PGfKGioVrCSo1yfu4iYCBsk\n" \
"Haswha6vsC6eep3BwEIc4gLw6uBK0u+QDrTBQBbwb4VCSmT3pDCg/r8uoydajotY\n" \
"uK3DGReEY+1vVv2Dy2A0xHS+5p3b4eTlygxfFQIDAQABo4IBbjCCAWowHwYDVR0j\n" \
"BBgwFoAUU3m/WqorSs9UgOHYm8Cd8rIDZsswHQYDVR0OBBYEFI2MXsRUrYrhd+mb\n" \
"+ZsF4bgBjWHhMA4GA1UdDwEB/wQEAwIBhjASBgNVHRMBAf8ECDAGAQH/AgEAMB0G\n" \
"A1UdJQQWMBQGCCsGAQUFBwMBBggrBgEFBQcDAjAbBgNVHSAEFDASMAYGBFUdIAAw\n" \
"CAYGZ4EMAQIBMFAGA1UdHwRJMEcwRaBDoEGGP2h0dHA6Ly9jcmwudXNlcnRydXN0\n" \
"LmNvbS9VU0VSVHJ1c3RSU0FDZXJ0aWZpY2F0aW9uQXV0aG9yaXR5LmNybDB2Bggr\n" \
"BgEFBQcBAQRqMGgwPwYIKwYBBQUHMAKGM2h0dHA6Ly9jcnQudXNlcnRydXN0LmNv\n" \
"bS9VU0VSVHJ1c3RSU0FBZGRUcnVzdENBLmNydDAlBggrBgEFBQcwAYYZaHR0cDov\n" \
"L29jc3AudXNlcnRydXN0LmNvbTANBgkqhkiG9w0BAQwFAAOCAgEAMr9hvQ5Iw0/H\n" \
"ukdN+Jx4GQHcEx2Ab/zDcLRSmjEzmldS+zGea6TvVKqJjUAXaPgREHzSyrHxVYbH\n" \
"7rM2kYb2OVG/Rr8PoLq0935JxCo2F57kaDl6r5ROVm+yezu/Coa9zcV3HAO4OLGi\n" \
"H19+24rcRki2aArPsrW04jTkZ6k4Zgle0rj8nSg6F0AnwnJOKf0hPHzPE/uWLMUx\n" \
"RP0T7dWbqWlod3zu4f+k+TY4CFM5ooQ0nBnzvg6s1SQ36yOoeNDT5++SR2RiOSLv\n" \
"xvcRviKFxmZEJCaOEDKNyJOuB56DPi/Z+fVGjmO+wea03KbNIaiGCpXZLoUmGv38\n" \
"sbZXQm2V0TP2ORQGgkE49Y9Y3IBbpNV9lXj9p5v//cWoaasm56ekBYdbqbe4oyAL\n" \
"l6lFhd2zi+WJN44pDfwGF/Y4QA5C5BIG+3vzxhFoYt/jmPQT2BVPi7Fp2RBgvGQq\n" \
"6jG35LWjOhSbJuMLe/0CjraZwTiXWTb2qHSihrZe68Zk6s+go/lunrotEbaGmAhY\n" \
"LcmsJWTyXnW0OMGuf1pGg+pRyrbxmRE1a6Vqe8YAsOf4vmSyrcjC8azjUeqkk+B5\n" \
"yOGBQMkKW+ESPMFgKuOXwIlCypTPRpgSabuY0MLTDXJLR27lk8QyKGOHQ+SwMj4K\n" \
"00u/I5sUKUErmgQfky3xxzlIPK1aEn8=\n" \
"-----END CERTIFICATE-----\n";
 


void loop() {
  
  unsigned char  status= SPI_Read(STATUS);
  unsigned long currentMillis = millis();
// How much time has passed, accounting for rollover with subtraction!
  if ((unsigned long)(currentMillis - previousMillis) >= interval) {
      // It's time to do something!
      ESP.restart();
  }


 
  //Serial.println(status);
  
  if(status&RX_DR)                                                 // if receive data ready (TX_DS) interrupt
  { 
      Serial.println(status);
  
      //ESP.wdtFeed();
      
      SPI_Read_Buf(RD_RX_PLOAD, rx_buf, NRF_TX_PLOAD_WIDTH);             // read playload to rx_buf
  
      bool c=false; // check nrf connect if receive ffffff c=false
      for(int i = 0;i<NRF_TX_PLOAD_WIDTH;i++)
      {
        if(rx_buf[i]!=0xff)
        {
          c=true;
        }
      }
      if(c==false)
      {
        return;
      }
  
      NRF_RX_Flag = true;
      
      if (OLEDcurrentline >= 8)
      {
        //didplayclear();
        display.clearDisplay();
        display.display();
        OLEDcurrentline = 0;
        display.setCursor(0, 0);     // Start at top-left corner        
      }
  
      //Serial.print(": ");
      for(int i=0; i<NRF_TX_PLOAD_WIDTH; i++)
      //for(int i=0; i<NRF_TX_PLOAD_WIDTH-2; i++)
      {
          //ESP.wdtFeed();
          Serial.print(Hex2String(rx_buf[i]));                              // print rx_buf
          display.print(Hex2String(rx_buf[i]));  
      }
      display.println("");
      display.display();
      Serial.println("");
      
      HTTPDATA = HTTPDATA + Hex2String(rx_buf[0])+ Hex2String(rx_buf[1])+ Hex2String(rx_buf[2])+ Hex2String(rx_buf[3]) + "10" + Hex2String(rx_buf[1]) + Hex2String(rx_buf[4]) + Hex2String(rx_buf[5]);
      HTTPDATA = setting_id + HTTPDATA;
      Serial.println(setting_url + "?data=" + HTTPDATA);

    

      if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
        HTTPClient http;
        http.begin(setting_url + "?data=" + HTTPDATA, root_ca); //Specify the URL and certificate
        int httpCode = http.GET();                                                  //Make the request
     
        if (httpCode > 0) { //Check for the returning code
          String payload = http.getString();
          Serial.println(httpCode);
          Serial.println(payload);
        }
     
        else {
          Serial.println("Error on HTTP request");
        }
     
        http.end(); //Free the resources
      }
      HTTPDATA = "";
      OLEDcurrentline++;                 
      SPI_RW_Reg(FLUSH_RX,0);                                         // clear RX_FIFO
      SPI_RW_Reg(WRITE_REG+STATUS,status);                             // clear RX_DR or TX_DS or MAX_RT interrupt flag
  }
}



void checkecho_200ok(String line)
{
    String newString = line.substring(line.length() - 6, line.length());
    if(newString =="200 OK")
    {
      Serial.println("  ***** Found 200 OK *****");
      WiFi_TX_Flag = true;
    }
}




void testdrawrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2; i+=2) {
    display.drawRect(i, i, display.width()-2*i, display.height()-2*i, SSD1306_WHITE);
    display.display(); // Update screen with each newly-drawn rectangle
    delay(1);
  }

  delay(2000);
}


void NRFInitial(){
  Serial.print("NRF status : ");
  
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner

  display.print("NRF : ");display.display();
  delay(500);
  pinMode(NRF_CS, OUTPUT);  
  pinMode(NRF_CE, OUTPUT);
  digitalWrite(NRF_CE, LOW);      // chip enable
  digitalWrite(NRF_CS, HIGH);                 // Spi disable  
  SPI.begin();
  delay(50);
  
  SPI_RW_Reg(FLUSH_RX,0);                                         // clear RX_FIFO
  SPI_RW_Reg(FLUSH_TX,0);                                         // clear RX_FIFO
  unsigned char status=SPI_Read(STATUS);
  SPI_RW_Reg(WRITE_REG+STATUS,status|RX_DR|TX_DS|MAX_RT);         // clear RX_DR or TX_DS or MAX_RT interrupt flag  
  status=SPI_Read(STATUS);
  //ESP.wdtFeed();
  
  Serial.println(status,HEX);
  display.print(status,HEX);display.display();

  RX_Mode();                        // set RX mode
  delay(2000);
  //ESP.wdtFeed();
  
  display.clearDisplay();
  display.display();
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
}


unsigned char SPI_Write_Buf(unsigned char reg, unsigned char *pBuf, unsigned char bytes)
{
  unsigned char status,i;
  digitalWrite(NRF_CS, LOW);           // CE chip enable CE high to enable RX device
  delayMicroseconds(2);
  status = SPI.transfer(reg);             // Select register to write to and read status unsigned char
  
  for(i=0;i<bytes; i++)             // then write all unsigned char in buffer(*pBuf)
  {
    SPI.transfer(*pBuf++);
  }
  delayMicroseconds(2);
  digitalWrite(NRF_CS, HIGH);  
  return(status);                  // return nRF24L01 status unsigned char
}


unsigned char SPI_Read(unsigned char reg)
{
  unsigned char reg_val;

  digitalWrite(NRF_CS, LOW);                   // Set CSN low, init SPI tranaction
  delayMicroseconds(2);  
  SPI.transfer(reg);             // Select register to write to and read status unsigned char
  reg_val = SPI.transfer(0);             // Select register to write to and read status unsigned char
  delayMicroseconds(2);
  digitalWrite(NRF_CS, HIGH);                  // Set CSN high again

  return(reg_val);                  // return nRF24L01 status unsigned char
}

unsigned char SPI_Read_Buf(unsigned char reg, unsigned char *pBuf, unsigned char bytes)
{
  unsigned char status,i;

  digitalWrite(NRF_CS, LOW);                   // Set CSN low, init SPI tranaction
  delayMicroseconds(2);  
  status = SPI.transfer(reg);             // Select register to write to and read status unsigned char

  for(i=0;i<bytes;i++)
  {
    pBuf[i] = SPI.transfer(0);    // Perform SPI_RW to read unsigned char from nRF24L01
  }
  delayMicroseconds(2);
  digitalWrite(NRF_CS, HIGH);                  // Set CSN high again

  return(status);                  // return nRF24L01 status unsigned char
}


unsigned char SPI_RW_Reg(unsigned char reg, unsigned char value)
{
  unsigned char status;

  digitalWrite(NRF_CS, LOW);                  // CSN low, init SPI transaction
  delayMicroseconds(2);  
  status = SPI.transfer(reg);             // select register
  SPI.transfer(value);                    // ..and write value to it..
  delayMicroseconds(2);  
  digitalWrite(NRF_CS, HIGH);                   // CSN high again
  
  return(status);                   // return nRF24L01 status unsigned char
}

void RX_Mode(void)
{
    digitalWrite(NRF_CE, LOW);
    delayMicroseconds(2);
    SPI_Write_Buf(WRITE_REG + RX_ADDR_P0, NRF_TX_ADDRESS, NRF_TX_ADR_WIDTH); // Use the same address on the RX device as the TX device
    SPI_RW_Reg(WRITE_REG + EN_AA, 0x01);      // Enable Auto.Ack:Pipe0
    SPI_RW_Reg(WRITE_REG + EN_RXADDR, 0x01);  // Enable Pipe0
    SPI_RW_Reg(WRITE_REG + RF_CH, 100);        // Select RF channel 40
    SPI_RW_Reg(WRITE_REG + RX_PW_P0, NRF_TX_PLOAD_WIDTH); // Select same RX payload width as TX Payload width

    //SPI_RW_Reg(WRITE_REG + RF_SETUP, 0x07);   // TX_PWR:0dBm, Datarate:2Mbps, LNA:HCURR
    SPI_RW_Reg(WRITE_REG + RF_SETUP, 0x27);   // TX_PWR:0dBm, Datarate:256kbps, LNA:HCURR
    SPI_RW_Reg(WRITE_REG + CONFIG, 0x0f);     // Set PWR_UP bit, enable CRC(2 unsigned chars) & Prim:RX. RX_DR enabled..
    delayMicroseconds(2);
    digitalWrite(NRF_CE, HIGH);
}

String Hex2String(byte s)
{
   if(s <16){
     return("0"+String(s,HEX));
   }
   else{
     return(String(s,HEX));
   }
}



//=======================================================================================WiFi====================================================================
void WiFiInitial() {
  int cntCount = 0;
  byte mac[7];
  String IPaddr="";

  wifiMulti.addAP(WIFI_SSID0, WIFI_PASS0);
  wifiMulti.addAP(WIFI_SSID1, WIFI_PASS1);
  wifiMulti.addAP(WIFI_SSID2, WIFI_PASS2);



  //Connecting to the strongest WiFi connection
  /*
  if (wifiMulti.run(WiFi_timeout) == WL_CONNECTED)
  {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());  //print IP of the connected WiFi network
  }
  else  // if not WiFi not connected
  {
    Serial.println("WiFi not Connected");
  }
  */






  while (wifiMulti.run(WiFi_timeout) != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    cntCount++;
    if (cntCount > 100){
      ESP.restart();
      }
  }
  
  Serial.println(""); Serial.print("MAC:");
  display.clearDisplay();
  display.display();
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner  
  display.println("WiFi initial");
  display.print("MAC:");
  
  WiFi.macAddress(mac);

  for (int j = 0; j < 6; j++)
  {
    Serial.print(mac[j], HEX);
    display.print(mac[j], HEX);
    if (j < 5)
    {
      Serial.print(":");
      display.print(":");
    }
  }
  Serial.println();
  display.println("");
  WiFi.begin();
  Serial.println("Connecting");
  display.println("Connecting");
  
  int i = 0;

  delay(500);
  Serial.print('.');
  //Serial.print(i);
  display.print(".");
  display.display();
  display.display();
  delay(2000);
  
  i++;
  //ESP.wdtFeed();

  if (i>200) 
  { 
    Serial.println(""); Serial.println("Connect Fail!");
    display.println("");
    display.println("Connect Fail!");
  }
  else 
  {
    IPaddr = WiFi.localIP().toString();    

    Serial.println("");
    Serial.println("Connected");
    Serial.println("SSID:"+WiFi.SSID());
    Serial.println("IP: "+IPaddr);
    //ESP.wdtFeed();
    display.println("Connected");
    display.println("SSID:"+WiFi.SSID());
    display.println("IP: "+IPaddr);
    display.display();
    delay(5000);
    
    //ESP.wdtFeed();
    Serial.println("\r\nSending WiFi initial package");

    display.clearDisplay();
    display.display();
    display.println("Sending HTTP ini");
    Serial.println("\r\nSending HTTP ini");
    String strTest = setting_url+"?Initial="+setting_id;
    Serial.println(strTest);
    //HTTP_Connect(setting_url+"?Initial="+setting_id);

    Serial.println("Sending WiFi initial package finished");
    display.println("Completed");
    delay(2000);  
  }
}
