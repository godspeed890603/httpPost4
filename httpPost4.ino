#include <Wire.h>

#include <Wire.h>

//#include <Dhcp.h>
//#include <Dns.h>
//#include <Ethernet.h>
//#include <EthernetClient.h>
//#include <EthernetServer.h>
//#include <EthernetUdp.h>

#include <MemoryFree.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_Sensor.h>
#include <enc28j60.h>
#include <EtherCard.h>
#include <DS3231.h>
#include <net.h>
#include <stdint.h>
#include <avr/wdt.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>


//宣告I2C LCD 2004
// Set the LCD address to 0x3F for a 20 chars and 4 line display
LiquidCrystal_I2C lcd(0x3F, 20, 4);

//宣告DHT21
#define DHT21_01_PIN 3     // what digital pin we're connected to
#define DHT21_02_PIN 4     // what digital pin we're connected to
#define DHTTYPE DHT21   // DHT 21 (AM2301)
DHT dht1(DHT21_01_PIN, DHTTYPE);
DHT dht2(DHT21_02_PIN, DHTTYPE);

//網路卡宣告
#define BUF_SIZE 512
#define ENC28J60_CS_PIN 10 //Enc28j60 Enable Pin

//define Alarm Pin
#define RELAY_CTL_PIN 5 //Alarm Enable Pin


// Init the DS3231 using the hardware interface
DS3231  rtc(SDA, SCL);//I2C formation
Time  t;// Init a Time-data structure

//variable for network
byte mac[] = { 0x00, 0x04, 0xA3, 0x21, 0xCA, 0x38 };   // Nanode MAC address.
uint8_t ip[] = { 172, 16, 11, 100 };          // The fallback board address.
uint8_t dns[] = { 0, 0, 0, 0 };        // The DNS server address.
uint8_t gateway[] = { 172, 16, 11, 254 };    // The gateway router address.
uint8_t subnet[] = { 255, 255, 255, 0 };    // The subnet mask.
byte Ethernet::buffer[BUF_SIZE];
//byte fixed;                                 // Address fixed, no DHCP
uint8_t ip_remote[] = { 172, 27, 10, 92 };          // The fallback board address.
const char website[] PROGMEM = "172.27.10.92";

//timer variable for application
static uint32_t timer_Enc28j60=0;//for 網路卡
static uint32_t timer_DisplayDate_Time=0;// for日期時間
static uint32_t timer_Temp_humi=0;// for溫濕度
static uint32_t timer_Mem=0;// for溫濕度
static long timer_Upload_Report=0;// for溫濕度
//read flag
int readFlg=0;

//upload data
float uT1=0.0;
float uH1=0.0;
float uT2=0.0;
float uH2=0.0;

//Network card initailize
void initialEnc28J60(){//網路卡
  //網路卡設定
  /* Check that the Ethernet controller exists */
  //啟用控制板的ENC28J60
    Serial.println(F("Initialising the Ethernet controller"));
    if (ether.begin(sizeof Ethernet::buffer, mac, ENC28J60_CS_PIN) == 0) {
        Serial.println(F( "Ethernet controller NOT initialised"));
        while (true)
            /* MT */ ;
    }
    
//設定網路卡IP Start
    /* Get a DHCP connection */
//    Serial.println("Attempting to get an IP address using DHCP");
//    fixed = false;
//    if (ether.dhcpSetup()) {
//        ether.printIp("Got an IP address using DHCP: ", ether.myip);
//    }

//    /* If DHCP fails, start with a hard-coded address */
//    else {
        ether.staticSetup(ip, gateway,dns,subnet);
        ether.printIp(F("Using fixed address: "), ether.myip);
//        fixed = true;
//    }
//設定網路卡IP End
  
  //Set Server IP
 // ether.copyIp(ether.hisip, ip_remote);
  ether.copyIp(ether.hisip, ip_remote);
  ether.printIp(F("SRV: "), ether.hisip);
  
  // call this to report others pinging us
  ether.registerPingCallback(gotPinged);
  timer_Enc28j60 = 0; // start timing out right away
  //Serial.println();  
}

//Display LCD Monitor
void lcdDisplay(uint8_t x, uint8_t y, char *msg ){
  lcd.setCursor(x, y);
  lcd.print(msg);  
}

// called when the client request is complete
static void my_callback (byte status, word off, word len) {
  Serial.println(F(">>>"));
  Ethernet::buffer[off+300] = 0;
  Serial.println((const char*) Ethernet::buffer + off);
  Serial.println(F("..."));
}


//// called when a ping comes in (replies to it are automatic)
static void gotPinged (byte* ptr) {
  ether.printIp(F(">>> ping from: "), ptr);
}


//reply client ping
void pingTargetAndReplyPing(){
  //ether.packetLoop(ether.packetReceive());
  // go receive new packets ,這一行不能四處亂用,會到至PING不到
  word len = ether.packetReceive(); 
  
  //這一行是回覆別人的PING,沒這一行..會PING不到
  // respond to incoming pings
  word pos = ether.packetLoop(len); 
 
  //  // report whenever a reply to our outgoing ping comes back
  if (len > 0 && ether.packetLoopIcmpCheckReply(ether.hisip)) {
     Serial.print(F("  "));
     Serial.print((micros() - timer_Enc28j60) * 0.001, 3);
     Serial.println(F(" ms"));
  }
    
  //  // ping a remote server once every few seconds
  if (micros() - timer_Enc28j60 >= (60*1000000)) {
    ether.printIp(F("Pinging: "), ether.hisip);
    ether.clientIcmpRequest(ether.hisip);
   timer_Enc28j60 = micros();
  }
}


//Dispaly Date & Time..
void dispalyClock(){
  char year[4] ;
  char date[4] ;
  char hh[2] ;
  char mm[2] ;
  char ss[2] ;
  char msg[18];
  int  time_1sec=1000;

  if (millis() - timer_DisplayDate_Time >= (1*time_1sec)) {
    // Get data from the DS3231
    t = rtc.getTime();
    
    //lcd.clear();
    //lcd.setCursor(0, 2);
    char readData[10];
     
    itoa(t.year,readData,10);
    strcpy(msg, readData);
    strcat(msg, "-" );

    memset(readData, " ", 10);
    if(t.mon < 10){
      strcat(msg, "0" );
    }    
    itoa(t.mon, readData, 10);
    strcat(msg, readData);
    strcat(msg, "-" );

    memset(readData, " ", 10);
    if(t.date < 10){
      strcat(msg, "0" );
    }
    itoa(t.date, readData, 10);
    strcat(msg,readData );
    strcat(msg, " " );

    memset(readData, " ", 10);
    if(t.hour < 10){
      strcat(msg, "0" );
    }
    itoa(t.hour, readData, 10);
    strcat(msg,readData );
    strcat(msg, ":" );

    memset(readData, " ", 10);
    if(t.min < 10){
      strcat(msg, "0" );
    }
    itoa(t.min, readData, 10);
    strcat(msg,readData );
    strcat(msg, ":" );

    memset(readData, " ", 10);
    if(t.sec < 10){
      strcat(msg,"0");
    }
    itoa(t.sec, readData, 10);
    strcat(msg,readData );
    //lcd.print(msg);
    lcdDisplay(0, 2, msg);
     //Serial.println(msg);
    timer_DisplayDate_Time=  millis()  ;
  }
}

//Read temp and humi for DHT21_01
void ReadDHT21_01(float *t1,float *h1){
  char msg[30];
  char readData[5];
  float error_T_H_Data= 99.9;
  
  // Reading temperature or humidity takes about 250 milliseconds!
  float h = dht1.readHumidity();
  
  // Read temperature as Celsius (the default)
  //memset(readData_T, " ", 5);
  float t = dht1.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor1!"));
    t = error_T_H_Data ;
    h = error_T_H_Data;
    //return;
  }

   //Dht1溫濕度補償
  if(!((t==99.9) || ( h==99.9))){
    //h = (h - 2.0); //revise
    h = (h + 3.5); //revise
    t = (t - 0.9); //revise
  }
  //data for return
    *h1=h;
    *t1=t;
      
    //Serial.println(F("pointer Humi1"));
    //Serial.println(*h1);
    //Serial.println(F("pointer Tmp1"));
    //Serial.println(*t1);

    strcpy(msg, "S1 T:");
    dtostrf(t, 4, 1, readData);  
    strcat(msg, readData );
    memset(readData, " ", 5);
    strcat(msg, "C H:");
    dtostrf(h, 4, 1, readData );
    strcat(msg, readData );
    strcat(msg, "%");

    //lcd.setCursor(0, 0);
    //lcd.print(msg);
    lcdDisplay(0, 0, msg);

}

//Read temp and humi for DHT21_02
void ReadDHT21_02(float *t2,float *h2){

  //char readData_T[5];
  //char readData_H[5];
  char msg[30];
  char readData[5];
  float error_T_H_Data= 99.9;

  // Reading temperature or humidity takes about 250 milliseconds!
  float h = dht2.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht2.readTemperature();
     
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor2!"));
    t =error_T_H_Data ; 
    h = error_T_H_Data ; 
    //return;
  }
  //Dht2溫濕度補償
  if(!((t==99.9) || ( h==99.9))){
    //h = (h - 2.0); //revise
    h = (h + 6.5); //revise
    t = (t -0.2); //revise
  }
  //data for return
    *h2=h;
    *t2=t;

  //Serial.println("pointer Humi2");
  //Serial.println(h2);
  //Serial.println("pointer Tmp2");
  //Serial.println(t2);
  //Serial.println("real tmp");
  //Serial.println(t);

  
  strcpy(msg, "S2 T:");
  dtostrf(t, 4, 1, readData);  
  strcat(msg, readData );

  memset(readData, " ", 5);
  strcat(msg, "C H:");
  dtostrf(h, 4, 1, readData);
  strcat(msg, readData );
  strcat(msg, "%");

  //lcd.setCursor(0, 1);
  //lcd.print(msg);
  lcdDisplay(0, 1, msg);
 }

 //fuction for Data read...
void readTempC(){
     long  time_1sec=1000;
     float t1=0.0;
     float t2=0.0;
     float h1=0.0;
     float h2=0.0;

     //long time_Period_Upload=80;
     long time_Period_Upload=300;
     long time_Period_ReadData=10;

//     time_1sec=1000;

  // 要求匯流排上的所有感測器進行溫度轉換
  if (millis() - timer_Temp_humi >= ( time_Period_ReadData * time_1sec)) {

    if (readFlg==0){
       Serial.println(F("ReadDHT21_01"));
      ReadDHT21_01( &t1, &h1);
    //Serial.println(F("pointer Humi1"));
    //Serial.println(h1);
    //Serial.println(F("pointer Tmp1"));
    //Serial.println(t1);
      uT1=t1;
      uH1=h1;    
      readFlg=1;
    } else if(readFlg==1){
     Serial.println(F("ReadDHT21_02"));
     ReadDHT21_02( &t2, &h2);
    //Serial.println("pointer Humi2");
    //Serial.println(h2);
    //Serial.println("pointer Tmp2");
    //Serial.println(t2);
      uT2=t2;
      uH2=h2;
       readFlg=0;
    }
  
      if ((timer_Upload_Report * time_1sec) > (time_Period_Upload * time_1sec)){
        //Serial.println("upload");
          //uploadDataReport(t1,h1,t2,h2); 
          uploadDataReport(uT1,uH1,uT2,uH2); 
        //Serial.println("end");
          //Next 300 Sec(5min)
                uT1=0.0;
                uH1=0.0;
                uT2=0.0;
                uH2=0.0;
          timer_Upload_Report=0;    
      }else{
          timer_Upload_Report = timer_Upload_Report+5;
      }
      timer_Temp_humi = millis();
     //Serial.println(timer_Upload_Report);
     //Temp alarm Check..if out of condition..Relay ON!
     alarmCheck(t1,t2);
  }
}

//Uplaod Data to Database
void uploadDataReport(float t1,float h1,float t2,float h2){


  char DataPost[200];
  char readData_t1[5];
  char readData_h1[5];
  char readData_t2[5];
  char readData_h2[5];
  char buffer_post[200];
  
  
   memset(DataPost, " ", 200); 

   memset(readData_t1, " ", 5);//DHT21_01 t
   dtostrf(t1, 4, 1, readData_t1);//DHT21_01 t   
   memset(readData_t2, " ", 5);//DHT21_02 t
   dtostrf(t2, 4, 1, readData_t2); //DHT21_02 t
   memset(readData_h1, " ", 5);//DHT21_01 h
   dtostrf(h1, 4, 1, readData_h1);//DHT21_01 h 
   memset(readData_h2, " ", 5);//DHT21_01 h   
   dtostrf(h2, 4, 1, readData_h2);//DHT21_01 h 
   
   // Serial.println(F("pointer Humi1"));
   // Serial.println(h1);
   // Serial.println(F("pointer Tmp1"));
   // Serial.println(t1);


   //Data upload prepare
   sprintf(DataPost,"ROOM_NO=L2&TEMP1=%s&TEMP2=%s&HDY1=%s&HDY2=%s",readData_t1,readData_t2,readData_h1,readData_h2);  
   Serial.println(DataPost);
   snprintf(buffer_post, 200, "%s", DataPost);
   ether.browseUrl(PSTR("/TH_MON/TH_SAVE.PHP?"),buffer_post, website, my_callback);
}


//temp over seeting,hard alarm,,
void alarmCheck(float t1 ,float t2){
    float over_Temp_Value = 25;
    
    
    if (((t1 > over_Temp_Value) || (t2 > over_Temp_Value)) ){
        digitalWrite(RELAY_CTL_PIN, LOW);   // turn the LED on (HIGH is the voltage level)
     }else{   
        digitalWrite(RELAY_CTL_PIN, HIGH);    // turn the LED off by making the voltage High    
     }
}

//check HardWare memery usage
void getFreeMem(){
  char msg[30];

  long time_Period_ReadMem=10;

  int freeMemSize=0;
  int  time_1sec=1000;

  if (millis() - timer_Mem >= (time_Period_ReadMem * time_1sec)) {
    memset(msg, " ", 30);
    freeMemSize = freeMemory();
    sprintf(msg, "Free Mem: %d ",freeMemSize);
    
    //lcd.setCursor(0, 3);
    //lcd.print(msg);
    lcdDisplay(0, 3, msg);
    timer_Mem=millis();
  }
  
}

void resetTimer(){
  if (timer_DisplayDate_Time > millis() && timer_DisplayDate_Time >0) 
      timer_DisplayDate_Time=0;
      
  if (timer_Temp_humi > millis()  && timer_Temp_humi>0) 
      timer_Temp_humi=0; 

  if (timer_Mem > millis() && timer_Mem >0) 
      timer_Mem=0; 

   if (timer_Enc28j60 > micros() && timer_Enc28j60>0)
       timer_Enc28j60=0;
}

void setup() {
  // 控制板設定
  Serial.begin(9600);
  
  //DHT initialize
  dht1.begin();
  dht2.begin();

  // Initialize the rtc object
  rtc.begin();

  // initialize the LCD
  lcd.begin();
  lcd.backlight();
  lcd.clear();
 
  //網路卡設定
 initialEnc28J60();

 // initialize digital pin for Relay as an output.
  pinMode(RELAY_CTL_PIN, OUTPUT);
  digitalWrite(RELAY_CTL_PIN, HIGH);
 
//監控Loop,如果超過兩秒(當機的意思),控制板自動重新開機
  wdt_enable(WDTO_2S);
}


void loop() {
  //if millis() overflow, alltimer reset to 0
  resetTimer();
  
  // Wait one second before repeating :)
  dispalyClock();
  
  //網路卡確認遠端SERVER & 回覆遠端Ping
  pingTargetAndReplyPing();
  
  //Read temperature and humidity
   readTempC();

  //get free mem
   getFreeMem();
  //清除自動重新開機的時間
  wdt_reset();
}







