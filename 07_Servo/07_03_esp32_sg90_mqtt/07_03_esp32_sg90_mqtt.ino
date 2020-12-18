#include <ESP32Servo.h> // https://github.com/madhephaestus/ESP32Servo
#include <WiFi.h>
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient

char *ssidC="SSID";
char *passC="PASSWORD";
char *servC="192.168.1.1";

const char* gcTopic = "myTopic";
const String gsClientId = "myMqttSub" ;

WiFiClient myWifiClient;
const int myMqttPort = 1883;
PubSubClient myMqttClient(myWifiClient);
#define MQTT_QOS 0


#define NUM_1SEC_MICROSEC (1000000)
#define NUM_SEC_TIMER (10)
#define NUM_TIMER_MICROSEC (NUM_SEC_TIMER*NUM_1SEC_MICROSEC)
hw_timer_t * handleTimer2 = NULL; 

portMUX_TYPE timerMux2 = portMUX_INITIALIZER_UNLOCKED; 

volatile int gviCountWatchDog=0;
#define NUM_MAX_WATCHDOG (6) // 60sec 

// ref to https://github.com/madhephaestus/ESP32Servo/blob/master/examples/Multiple-Servo-Example-Arduino/Multiple-Servo-Example-Arduino.ino

// create four servo objects 
Servo servo1;

// Published values for SG90 servos; adjust if needed
int minUs = 500;
int maxUs = 2400;

// These are all GPIO pins on the ESP32
// Recommended pins include 2,4,12-19,21-23,25-27,32-33 
int servo1Pin = 25;

int pos = 0;      // position in degrees


void IRAM_ATTR irq_Timer2()
{
  portENTER_CRITICAL_ISR(&timerMux2);
  gviCountWatchDog++;
  if (gviCountWatchDog > NUM_MAX_WATCHDOG)
  {
    selfRestartProcess();
  }

  portEXIT_CRITICAL_ISR(&timerMux2);
}//irq_Timer02()

void init_TimerInterrupt2()
{
  gviCountWatchDog =0;
  handleTimer2 = timerBegin(2, 80, true); // Num_timer ,  counter per clock 
  timerAttachInterrupt(handleTimer2, &irq_Timer2, true);
  timerAlarmWrite(handleTimer2, NUM_TIMER_MICROSEC, true); //[us] per 80 clock @ 80MHz
  timerAlarmEnable(handleTimer2);
}//init_TimerInterrupt2()

void selfRestartProcess(void)
{
  //memory copy to flash
  ESP.restart(); // reboot
}//selfRestartProcess()



void initWifiClient(void){
  Serial.print("Connecting to ");
  Serial.println(ssidC);
  uint16_t tmpCount =0;
  
  WiFi.begin( ssidC, passC);
  while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");
  tmpCount++;
  if(tmpCount>128)
  {
    //gFlagWiFiConnectFailure = true;
    Serial.println("failed  ");
    return;
  }
}
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void connectMqttSub()
{
  myMqttClient.setServer(servC, myMqttPort);
  reconnectMqttSub();
  myMqttClient.setCallback(gotMessageViaMqtt);
}

void reconnectMqttSub()
{
  while( ! myMqttClient.connected() )
  {
    Serial.println("Connecting to MQTT Broker...");
    //String clientId = "myMqttPub" ;
    if ( myMqttClient.connect(gsClientId.c_str()) )
    {
      myMqttClient.subscribe(gcTopic,MQTT_QOS);
      Serial.println("done! connected to Broker and subscribed. "); 
    }
  }
}

void gotMessageViaMqtt(char * pctoic, byte * pbpayload, uint32_t uiLen){
  Serial.print(pctoic);
  Serial.print(" : \t");
  for (uint32_t i = 0; i < uiLen; i++) {
    Serial.print((char)pbpayload[i]);
  }
  Serial.println();
  moveServo(170);
  // run to check: mosquitto_pub -t myTopic -m HelloThisIsTestMssg
}



void setup() {
  Serial.begin(115200);
  servo1.setPeriodHertz(50);      // Standard 50hz servo
  servo1.attach(servo1Pin, minUs, maxUs);

  gviCountWatchDog=0; 
  init_TimerInterrupt2();
  initWifiClient();
  connectMqttSub();

  servo1.write(10);
}

void loop() {
  portENTER_CRITICAL(&timerMux2);
    gviCountWatchDog=0; //clear Counter
  portEXIT_CRITICAL(&timerMux2);
  reconnectMqttSub();
  myMqttClient.loop();
  delay(200);
}

void moveServo(int16_t i16Rot){
    for (pos = 10; pos <= i16Rot; pos += 1) { // sweep from 0 degrees to 180 degrees
    // in steps of 1 degree
    servo1.write(pos);
    delay(15);             // waits 15ms for the servo to reach the position
  }
  delay(100);
  
  for (pos = i16Rot; pos >= 10; pos -= 1) { // sweep from 180 degrees to 0 degrees
    // in steps of 1 degree
    servo1.write(pos);
    delay(15);             // waits 15ms for the servo to reach the position
  }
  delay(100);
}
