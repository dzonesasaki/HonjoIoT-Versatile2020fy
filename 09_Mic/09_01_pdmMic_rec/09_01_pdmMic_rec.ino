// 1sec audio sampling
// standard output 16bit wave stream
// processor : esp32
// IDE : arduino
// device : SPM0405HD4H  https://akizukidenshi.com/download/SPM0405HD4H.pdf
// ref to 

#include "driver/i2s.h" //https://github.com/espressif/arduino-esp32/blob/master/tools/sdk/include/driver/driver/i2s.h

// pin config
#define PIN_I2S_BCLK -1
#define PIN_I2S_WS 26
#define PIN_I2S_DIN 34

//#define STATE_N_L_R_PULL_DOWN 0 //no offset , mono:  PullUp = 0, PullDown=1
//#define N_BYTE_STEP_LR (STATE_N_L_R_PULL_DOWN*4)

// sampling info
#define HZ_SAMPLE_RATE 44100
#define N_SEC_REC  1  // second
// hardware and software parameter
#define N_SAMPLE_IN_MILLI_SEC 44
#define N_ITTER_READ_BUF  (N_SEC_REC * N_SAMPLE_IN_MILLI_SEC ) 
#define N_LEN_BUF_2BYTE 1000 
//#define N_BYTES_STRACT_PAR_SAMPLE 8
//#define N_BYTES_STRACT_PAR_SAMPLE 4
#define N_BYTES_STRACT_PAR_SAMPLE 2
#define N_LEN_BUF_ALL_BYTES  (N_LEN_BUF_2BYTE*N_BYTES_STRACT_PAR_SAMPLE)
#define N_SKIP_SAMPLES (11*2)
#define N_BUF_SIDE 2

char gi8BufAll[N_BUF_SIDE][N_LEN_BUF_ALL_BYTES];
int16_t gi16strmData[N_ITTER_READ_BUF*N_LEN_BUF_2BYTE];
volatile uint8_t gui8Side = 0;
volatile uint8_t gui8flagReadDone = 0;
xTaskHandle gxHandle;

void I2S_Init(void) {
  esp_err_t erResult = ESP_OK;
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX| I2S_MODE_PDM),
    .sample_rate = HZ_SAMPLE_RATE,
    
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
    .communication_format = I2S_COMM_FORMAT_I2S ,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 2,
    .dma_buf_len = 128,
    .use_apll = false
  };
  erResult = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);

  i2s_pin_config_t pin_config;
  pin_config.bck_io_num = I2S_PIN_NO_CHANGE;
  pin_config.ws_io_num = PIN_I2S_WS;
  pin_config.data_out_num = I2S_PIN_NO_CHANGE;
  pin_config.data_in_num = PIN_I2S_DIN;

  erResult = i2s_set_pin(I2S_NUM_0, &pin_config);

  erResult = i2s_set_clk(I2S_NUM_0, HZ_SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
}

void skipNoisySound(){
  size_t uiGotLen=0;
  for (int j = 0; j < N_SKIP_SAMPLES ; j++) 
  {
    i2s_read(I2S_NUM_0, (char *)gi8BufAll[0], N_LEN_BUF_ALL_BYTES, &uiGotLen,portMAX_DELAY);
  }
}

void taskI2sReading(void *arg){
  size_t uiGotLen=0;
  while(1){
    esp_err_t erReturns = i2s_read(I2S_NUM_0, (char *)gi8BufAll[gui8Side], N_LEN_BUF_ALL_BYTES, &uiGotLen, portMAX_DELAY);
    gui8Side = (gui8Side+1)&1;
    gui8flagReadDone =1;
  }
}


void getData(){
  int32_t tmp32=0;
  int16_t tmp16=0;
  size_t uiGotLen=0;
  esp_err_t erReturns;
  //uint32_t * ptmp32;
  uint16_t * ptmp16;

  //dummy for wakeup
  skipNoisySound();

  uint8_t ui8Side = 0;
  gui8flagReadDone =0;
  //erReturns = i2s_read(I2S_NUM_0, (char *)gi8BufAll[ui8Side], N_LEN_BUF_ALL_BYTES, &uiGotLen,portMAX_DELAY);
  xTaskCreate(taskI2sReading, "taskI2sReading", 2048, NULL, 1, &gxHandle);
  
  for (int j = 0; j < N_ITTER_READ_BUF ; j++) 
  {
    while(gui8flagReadDone==0){}
    gui8flagReadDone =0;
    ui8Side = (gui8Side+1)&1;
    ptmp16 = (uint16_t *)&gi8BufAll[ui8Side][0];
    for (int i = 0; i < N_LEN_BUF_2BYTE ; i++) {
      tmp16 = ((int16_t)(ptmp16[i]));
      gi16strmData[i + N_LEN_BUF_2BYTE*j] = tmp16;
    } // i
    //ui8Side = (ui8Side+1)&1;

    // I2S : https://docs.espressif.com/projects/esp-idf/en/v3.3/api-reference/peripherals/i2s.html
    //erReturns = i2s_read(I2S_NUM_0, (char *)gi8BufAll[ui8Side], N_LEN_BUF_ALL_BYTES, &uiGotLen,portMAX_DELAY);
    //erReturns must be ESP_OK  //https://docs.espressif.com/projects/esp-idf/en/v3.3/api-reference/system/esp_err.html
    //uiGotLen must be == N_LEN_BUF_ALL_BYTES

  } // j

  vTaskDelete(gxHandle);
  Serial.print(uiGotLen);
  Serial.print(" , ");
  Serial.print(erReturns);
  Serial.println("");
}

void stdoutStrm(){
  for (int j = 0; j < N_ITTER_READ_BUF*N_LEN_BUF_2BYTE ; j++) 
  {
    Serial.println(gi16strmData[j]);
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("---start---");
  Serial.flush();
  I2S_Init();
  //skipNoisySound();
  getData();
  Serial.println("---rec done---");
  stdoutStrm();
  Serial.println("---done---");
}

void loop() {
  // put your main code here, to run repeatedly:
}
