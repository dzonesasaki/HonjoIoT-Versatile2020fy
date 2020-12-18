#include <WiFi.h>
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient

char *ssidC="SSID";
char *passC="PASSWORD";
char* servC="192.168.1.1";

const char* gcTopic = "myTopic";
const String gsClientId = "myMqttPub" ;

WiFiClient myWifiClient;
const int myMqttPort = 1883;
PubSubClient myMqttClient(myWifiClient);

uint32_t gu32Count=0;

#define NUM_1SEC_MICROSEC (1000000)
#define NUM_SEC_TIMER (10)
#define NUM_TIMER_MICROSEC (NUM_SEC_TIMER*NUM_1SEC_MICROSEC)
hw_timer_t * handleTimer2 = NULL; 

portMUX_TYPE timerMux2 = portMUX_INITIALIZER_UNLOCKED; 

volatile int gviCountWatchDog=0;
#define NUM_MAX_WATCHDOG (6) // 60sec 


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

void connectMqtt()
{
  myMqttClient.setServer(servC, myMqttPort);
  while( ! myMqttClient.connected() )
  {
    Serial.println("Connecting to MQTT Broker...");
    //String clientId = "myMqttPub" ;
    if ( myMqttClient.connect(gsClientId.c_str()) )
    {
      Serial.println("connected"); 
    }
  }
}

void doPubMqtt(char * cValue)
{
  if(!myMqttClient.connected())
  { 
    if (myMqttClient.connect(gsClientId.c_str()) )
    {
      Serial.println("reconnected to MQTT broker"); 
    }
    else
    {
      Serial.println("failure to reconnect to MQTT broker");
      return;
    }
  }
  //myMqttClient.loop();
  boolean bFlagSucceed=myMqttClient.publish(gcTopic,cValue);
  //free(cValue);
  if(bFlagSucceed)
   {
    Serial.print("done: ");
    Serial.println(cValue);
  }
  else
    Serial.println("failure");
  // run to check: mosquitto_sub -t myTopic
}

void setup() {
  Serial.begin(115200);
  gviCountWatchDog=0; 
  init_TimerInterrupt2();
  initWifiClient();
  connectMqtt();

}

void loop(){
 Serial.println("start loop");
  portENTER_CRITICAL(&timerMux2);
    gviCountWatchDog=0; //clear Counter
  portEXIT_CRITICAL(&timerMux2);

  float fSensValue=gu32Count;

  //char cValue[11];
  char *cValue;
  cValue = (char *)malloc(12);
  sprintf(cValue, "%1.5e", fSensValue);

  doPubMqtt(cValue);
  free(cValue);
  
  delay(2000);
  gu32Count++;

}
