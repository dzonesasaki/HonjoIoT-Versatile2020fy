#define PIN_MIC_IN 33 
#define MICROSEC_INTERRUPT_INTERVAL 100 //100us:10kSa/s , //22kHz=45
#define MAX_SIZE  1024 //16*1024 //131072 // 128*1024
#define NUM_PREREAD_SIZE (MAX_SIZE/4) //
unsigned int guiStream[MAX_SIZE];
volatile unsigned int guiCountReadAdc=0;
volatile unsigned int guiPreRead=0;
unsigned int gfValAve=0;

#define COEF_IIR_ALPHA (float)1 //0.2
#define COEF_IIR_BETA (1-COEF_IIR_ALPHA)
hw_timer_t *timer1 = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED; //rer to https://techtutorialsx.com/2017/10/07/esp32-arduino-timer-interrupts/

void IRAM_ATTR onTimer1() {
    portENTER_CRITICAL_ISR(&timerMux);

    guiStream[guiCountReadAdc] = analogRead(PIN_MIC_IN);

    if (guiCountReadAdc >= (MAX_SIZE-0) )//
    {
        //guiCountReadAdc = 0;
    }
    else
    {
        guiCountReadAdc += 1;
    }

    portEXIT_CRITICAL_ISR(&timerMux);
    xSemaphoreGiveFromISR(timerSemaphore, NULL);
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_MIC_IN, INPUT);

    timerSemaphore = xSemaphoreCreateBinary();
    timer1 = timerBegin(0, 80, true);
    timerAttachInterrupt(timer1, &onTimer1, true);
    timerAlarmWrite(timer1,  MICROSEC_INTERRUPT_INTERVAL , true);//us
    timerAlarmEnable(timer1);

    guiCountReadAdc=0;
    guiPreRead=0;
    gfValAve=0;

    unsigned int uiFlag=0;
    while(uiFlag==0)
    {
      if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE) {
          portENTER_CRITICAL(&timerMux);
          if(guiCountReadAdc>NUM_PREREAD_SIZE){
            uiFlag=1;
            guiPreRead=1;
          }
          portEXIT_CRITICAL(&timerMux);
      }//endif xSemaphore
    }
    
    //Serial.println("start sampling");

}

void loop()
{
    unsigned int uiCounterVal=0;
     
    if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE) {
        portENTER_CRITICAL(&timerMux);
        uiCounterVal = guiCountReadAdc;
        portEXIT_CRITICAL(&timerMux);
    }//endif xSemaphore

    if ( uiCounterVal >= (MAX_SIZE-1) ){
        timerAlarmDisable(timer1);
        for(unsigned int uilp =0;uilp<(MAX_SIZE-1);uilp++){
            //guiValAve = COEF_IIR_ALPHA*guiStream[uilp]+COEF_IIR_BETA*guiValAve;
            gfValAve *= COEF_IIR_BETA;
            gfValAve += COEF_IIR_ALPHA*(float)guiStream[uilp];// *(float)3.3/(float)4096;
            Serial.println(gfValAve);
        }//end for
        //Serial.println("end of data");
        guiCountReadAdc=0;
        timerAlarmEnable(timer1);
    }//endif counter
}
