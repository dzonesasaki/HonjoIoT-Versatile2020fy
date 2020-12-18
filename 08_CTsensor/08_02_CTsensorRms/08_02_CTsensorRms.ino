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

hw_timer_t *timer1 = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED; //rer to https://techtutorialsx.com/2017/10/07/esp32-arduino-timer-interrupts/

void IRAM_ATTR onTimer1() {
    portENTER_CRITICAL_ISR(&timerMux);
    guiStream[guiCountReadAdc] = analogRead(PIN_MIC_IN);
    if (guiCountReadAdc < (MAX_SIZE-0) ){
     guiCountReadAdc += 1;
    }
    portEXIT_CRITICAL_ISR(&timerMux);
    xSemaphoreGiveFromISR(timerSemaphore, NULL);
}

void makeTable(){
	float fOmega=0;
	for(int uilp=0;uilp<N_SAMPLE_POWER_CYCLE;uilp++){
		fOmega = (float)(2*PI*(float)uilp/(float)N_SAMPLE_POWER_CYCLE);
		gfCosTable[uilp] = cos(fOmega);
		gfSinTable[uilp] = sin(fOmega);
	}
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
    makeTable();

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
        float fMean=0;
        float fDiff=0;
        float fSum=0;
        // for(unsigned int uilp =0;uilp<(MAX_SIZE-1);uilp++){
        //   fMean += (float)guiStream[uilp];
        // }//end for
        // fMean = fMean/(float)(MAX_SIZE-1);
		    fMean = calc_mean();
        // for(unsigned int uilp =0;uilp<(MAX_SIZE-1);uilp++){
        //   fDiff = (float)guiStream[uilp]-fMean;
        //   fSum += fDiff*fDiff;
        // }//end for
        // float fRms = sqrt(fSum/(float)(MAX_SIZE-1));
		    float fRms = calc_rms(fMean);
		    float fFund = calc_fundamental(fMean);

        //float fPow = fRms * FACT_POW * fFund;
        float fPow = FACT_POW * fFund;
        Serial.print(fPow);
        Serial.print(" , ");
        Serial.println(fRms*FACT_CUR);
        guiCountReadAdc=0;
        timerAlarmEnable(timer1);
    }//endif counter
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
//	for(unsigned int uilp =0;uilp<(MAX_SIZE-1);uilp++){
//        fTmp = (float)guiStream[uilp]-fMean;
//        fSumP += fTmp*fTmp;
//    }//end for uilp

  //float fFund = sqrt( (fSumR*fSumR+ fSumI*fSumI)*2/(float)(MAX_SIZE-1) / fSumP );
  float fFund = sqrt( (fSumR*fSumR+ fSumI*fSumI)*2)/(float)(MAX_SIZE-1)  ;
	return(fFund);
}
