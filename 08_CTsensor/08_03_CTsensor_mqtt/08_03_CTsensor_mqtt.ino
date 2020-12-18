#include <WiFi.h>
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient
#include <Preferences.h>

Preferences objEprom;
#define PARAM_KEY "myParams"
char *ssidC="SSID";
char *passC="PASSPHRESE"; 
char *servC="192.168.1.2";//server address

const char* topic = "myTopic";
const String clientId = "myMqttPub" ;

WiFiClient myWifiClient;
const int mqttPort = 1883;
PubSubClient mqttClient(myWifiClient);


#define PIN_MIC_IN 33
#define MICROSEC_INTERRUPT_INTERVAL 100 //100=10kSa/s //22kHz=45
#define MAX_SIZE  8*1024 //1024//16*1024 
#define NUM_PREREAD_SIZE (MAX_SIZE/4) //2048
unsigned int guiStream[MAX_SIZE];
volatile unsigned int guiCountReadAdc=0;
volatile unsigned int guiPreRead=0;
float gfValAve=0;

#define FREQ_POWER_LINE 50
#define N_SAMPLE_POWER_CYCLE (int)(1000*1000/( MICROSEC_INTERRUPT_INTERVAL * FREQ_POWER_LINE ))
#define PI (double)(3.14159265358979323846)

float gfCosTable[N_SAMPLE_POWER_CYCLE];
float gfSinTable[N_SAMPLE_POWER_CYCLE];


#define COEF_IIR_ALPHA (float)0.8
#define COEF_IIR_BETA (1-COEF_IIR_ALPHA)

#define PARAM_RESISTOR 300
#define PARAM_VOLTAGE_RMS 100
#define PARAM_ADC_MAX (float)3.3
#define PARAM_RATIO_CT 3000
#define PARAM_ADC_BITWIDH 12
//#define FACT_POW (3.3*3000/300*100/(float)(1<<12) )
#define FACT_POW (PARAM_ADC_MAX*PARAM_RATIO_CT*PARAM_VOLTAGE_RMS/(PARAM_RESISTOR*(float)(1<<PARAM_ADC_BITWIDH)) )
#define FACT_CUR (PARAM_ADC_MAX*PARAM_RATIO_CT/(PARAM_RESISTOR*(float)(1<<PARAM_ADC_BITWIDH)) )

hw_timer_t *handletimer0 = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timer2Mux = portMUX_INITIALIZER_UNLOCKED; 
portMUX_TYPE timer0Mux = portMUX_INITIALIZER_UNLOCKED;

hw_timer_t * handleTimer2 = NULL; 
volatile int gviCountWatchDog=0;

#define NUM_1SEC_MICROSEC (1000000)
#define NUM_SEC_WDTIMER (10)
#define NUM_WDTIMER_MICROSEC (NUM_SEC_WDTIMER*NUM_1SEC_MICROSEC)
#define NUM_MAX_WATCHDOG (6) // 60sec 


void IRAM_ATTR isr_timer0() {
    portENTER_CRITICAL_ISR(&timer0Mux);
    guiStream[guiCountReadAdc] = analogRead(PIN_MIC_IN);
    if (guiCountReadAdc < (MAX_SIZE-0) ){
     guiCountReadAdc += 1;
    }
    portEXIT_CRITICAL_ISR(&timer0Mux);
    xSemaphoreGiveFromISR(timerSemaphore, NULL);
}

void IRAM_ATTR irq_Timer2()
{
  portENTER_CRITICAL_ISR(&timer2Mux);
  gviCountWatchDog++;
  if (gviCountWatchDog > NUM_MAX_WATCHDOG)
  {
    selfRestartProcess();
  }

  portEXIT_CRITICAL_ISR(&timer2Mux);
}//irq_Timer02()

void init_TimerInterrupt0()
{
  timerSemaphore = xSemaphoreCreateBinary();
  handletimer0 = timerBegin(0, 80, true);
  timerAttachInterrupt(handletimer0, &isr_timer0, true);
  timerAlarmWrite(handletimer0,  MICROSEC_INTERRUPT_INTERVAL , true);//us
  timerAlarmEnable(handletimer0);
}//init_TimerInterrupt()



void init_TimerInterrupt2()
{
  gviCountWatchDog =0;
  handleTimer2 = timerBegin(2, 80, true); // Num_timer ,  counter per clock 
  timerAttachInterrupt(handleTimer2, &irq_Timer2, true);
  timerAlarmWrite(handleTimer2, NUM_WDTIMER_MICROSEC, true); //[us] per 80 clock @ 80MHz
  timerAlarmEnable(handleTimer2);
}//init_TimerInterrupt()


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
  mqttClient.setServer(servC, mqttPort);
  while( ! mqttClient.connected() )
  {
    Serial.println("Connecting to MQTT Broker...");
    //String clientId = "myMqttPub" ;
    if ( mqttClient.connect(clientId.c_str()) )
    {
      Serial.println("connected"); 
    }
  }
}

void doPubMqtt(char * cValue)
{
  if(!mqttClient.connected())
  { 
    if (mqttClient.connect(clientId.c_str()) )
    {
      Serial.println("reconnected to MQTT broker"); 
    }
    else
    {
      Serial.println("failure to reconnect to MQTT broker");
      return;
    }
  }
  //mqttClient.loop();
  boolean bFlagSucceed=mqttClient.publish(topic,cValue);
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


void getEprom(void){
  if(objEprom.begin(PARAM_KEY, true)==true) // readonly mode
  {
    String ssidTa = objEprom.getString("ssid");
    String passTa = objEprom.getString("wifipass");
    String servTa = objEprom.getString("serv");
    objEprom.end();

    ssidC = (char *)malloc(sizeof(char)*ssidTa.length());
    passC = (char *)malloc(sizeof(char)*passTa.length());
    servC = (char *)malloc(sizeof(char)*servTa.length());

    if (servTa.length()<1)
    {
      Serial.println("failed reading EPROM ");
      return;
    }

    ssidTa.toCharArray(ssidC, ssidTa.length()+1 );
    passTa.toCharArray(passC, passTa.length()+1 );
    servTa.toCharArray(servC, servTa.length()+1 );

    Serial.println(String(ssidC));
    Serial.println(String(passC));
    Serial.println(String(servC));
    Serial.println("read EPROM ... done");
  }
  else
  {
    Serial.println("failed reading EPROM ");
  }
  objEprom.end();
}

void makeTable(){
float fOmega=0;
  for(int uilp=0;uilp<N_SAMPLE_POWER_CYCLE;uilp++){
    fOmega = (float)(2*PI*(float)uilp/(float)N_SAMPLE_POWER_CYCLE);
    gfCosTable[uilp] = cos(fOmega);
    gfSinTable[uilp] = sin(fOmega);
  }
}


float calc_mean(void){
  float fMean=0;
  for(unsigned int uilp =0;uilp<(MAX_SIZE-1);uilp++){
    fMean += (float)guiStream[uilp];
  }//end for
  fMean = fMean/(float)(MAX_SIZE-1);
  return(fMean);
}

float calc_rms(float fMean){
  float fDiff=0;
  float fSum=0;
  for(unsigned int uilp =0;uilp<(MAX_SIZE-1);uilp++){
    fDiff = (float)guiStream[uilp]-fMean;
    fSum += fDiff*fDiff;
  }//end for
  float fRms = sqrt(fSum/(float)(MAX_SIZE-1));
  return(fRms);
}


#define N_DFT_LOOP_INT (int)((MAX_SIZE-1)/N_SAMPLE_POWER_CYCLE)
#define N_DFT_LOOP_TAIL (int)(MAX_SIZE-1 - N_DFT_LOOP_INT*N_SAMPLE_POWER_CYCLE)

float calc_fundamental(float fMean){
  float fSumR=0;
  float fSumI=0;
  float fTmp=0;
  float fSumP=0;
  unsigned int uiinx=0;
  for(unsigned int uic = 0;uic < N_DFT_LOOP_INT ; uic++){
    for(unsigned int uilp =0;uilp<(N_SAMPLE_POWER_CYCLE);uilp++){
      fTmp = ((float)guiStream[uiinx]-fMean);
      fSumR += gfCosTable[uilp]*fTmp;
      fSumI += gfSinTable[uilp]*fTmp;
      uiinx++;
    }//end for uilp
  }//end for uic
  for(unsigned int uilp =0;uilp<(N_DFT_LOOP_TAIL);uilp++){
    fTmp = ((float)guiStream[uiinx]-fMean);
    fSumR += gfCosTable[uilp]*fTmp;
    fSumI += gfSinTable[uilp]*fTmp;
    uiinx++;
  }//end for uilp
  float fFund = sqrt( (fSumR*fSumR+ fSumI*fSumI)*2)/(float)(MAX_SIZE-1)  ;
  return(fFund);
}


void setup() {
  Serial.begin(115200);
  pinMode(PIN_MIC_IN, INPUT);

  guiCountReadAdc=0;
  guiPreRead=0;
  gfValAve=0;
  makeTable();

  gviCountWatchDog=0; 
  init_TimerInterrupt2();
  //Serial.println("get EPROM");
  //getEprom();
  Serial.println("wifi connect");
  initWifiClient();


  init_TimerInterrupt0();

  unsigned int uiFlag=0;
  while(uiFlag==0)
  {
    if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE) {
      portENTER_CRITICAL(&timer0Mux);
      if(guiCountReadAdc>NUM_PREREAD_SIZE){
        uiFlag=1;
        guiPreRead=1;
      }
        portEXIT_CRITICAL(&timer0Mux);
    }//endif xSemaphore
  }
  Serial.println("start sampling");

  connectMqtt();
}

void loop()
{
  unsigned int uiCounterVal=0;
  gviCountWatchDog=0;  
  if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE) {
    portENTER_CRITICAL(&timer0Mux);
    uiCounterVal = guiCountReadAdc;
    portEXIT_CRITICAL(&timer0Mux);
  }//endif xSemaphore

  if ( uiCounterVal >= (MAX_SIZE-1) ){
    timerAlarmDisable((hw_timer_t *)&isr_timer0);
    float fMean=0;
    float fDiff=0;
    float fSum=0;
    fMean = calc_mean();
    float fRms = calc_rms(fMean);
    float fFund = calc_fundamental(fMean);
    float fPow = FACT_POW * fFund;
    Serial.print(fPow);
    Serial.print(" , ");
    Serial.println(fRms*FACT_CUR);

    char *cValue;
    cValue = (char *)malloc(25);
    sprintf(cValue, "%3.3f , %3.3f", fRms*FACT_CUR, fPow );

    doPubMqtt(cValue);
    free(cValue);

    guiCountReadAdc=0;
    timerAlarmEnable((hw_timer_t *)&isr_timer0);
  }//endif counter
}
