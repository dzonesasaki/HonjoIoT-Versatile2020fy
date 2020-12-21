#include <WiFi.h>
#include <HTTPClient.h>
HTTPClient myHttp;
#include <Preferences.h>
Preferences objEprom;

#include "driver/i2s.h" //https://github.com/espressif/arduino-esp32/blob/master/tools/sdk/include/driver/driver/i2s.h


#define URL1 "http://"
#define URL2 ":1880/mywav01"

char *ssidC; //="SSID";
char *passC; //="PASSPHRESE"; 
char *servC; //="192.168.1.2";//server address


// pin config
#define PIN_I2S_BCLK -1
#define PIN_I2S_WS 0
#define PIN_I2S_DIN 34

#define PIN_LED_STICKC 10
#define PIN_BTN1_STICKC 37

//#define STATE_N_L_R_PULL_DOWN 0 //no offset , mono:  PullUp = 0, PullDown=1
//#define N_BYTE_STEP_LR (STATE_N_L_R_PULL_DOWN*4)

// sampling info
#define HZ_SAMPLE_RATE 44100
#define N_SEC_REC  1  // second
// hardware and software parameter
#define N_SAMPLE_IN_MILLI_SEC 44
#define N_ITTER_READ_BUF  (N_SEC_REC * N_SAMPLE_IN_MILLI_SEC ) 
//#define N_LEN_BUF_2BYTE 1000 
#define N_LEN_BUF_2BYTE 1000 
//#define N_BYTES_STRACT_PAR_SAMPLE 8
//#define N_BYTES_STRACT_PAR_SAMPLE 4
#define N_BYTES_STRACT_PAR_SAMPLE 2
#define N_LEN_BUF_ALL_BYTES  (N_LEN_BUF_2BYTE*N_BYTES_STRACT_PAR_SAMPLE)
#define N_SKIP_SAMPLES (11*2)
#define N_BUF_SIDE 2

char gi8BufAll[N_BUF_SIDE][N_LEN_BUF_ALL_BYTES];
//int16_t gi16strmData[N_ITTER_READ_BUF*N_LEN_BUF_2BYTE];
volatile uint8_t gui8Side = 0;
volatile uint8_t gui8flagReadDone = 0;
xTaskHandle gxHandle;

#define STRING_BOUNDARY "123456789000000000000987654321"
#define STRING_MULTIHEAD02 "Content-Disposition: form-data; name=\"uploadFile\"; filename=\"./testfig2.wav\""
#define STRING_MULTIHEAD03 "Content-Type: audio/wav"


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


void getData2(uint16_t *pu16Strm){
  int32_t tmp32=0;
  int16_t tmp16=0;
  size_t uiGotLen=0;
  esp_err_t erReturns;
  //uint32_t * ptmp32;
  uint16_t * ptmp16;

  //dummy for wakeup
  //skipNoisySound();

  uint8_t ui8Side = 0;
  gui8flagReadDone =0;
  //erReturns = i2s_read(I2S_NUM_0, (char *)gi8BufAll[ui8Side], N_LEN_BUF_ALL_BYTES, &uiGotLen,portMAX_DELAY);
  //xTaskCreate(taskI2sReading, "taskI2sReading", 2048, NULL, 1, &gxHandle);
  
  for (int j = 0; j < N_ITTER_READ_BUF ; j++) 
  {
    while(gui8flagReadDone==0){}
    gui8flagReadDone =0;
    ui8Side = (gui8Side+1)&1;
    ptmp16 = (uint16_t *)&gi8BufAll[ui8Side][0];
    for (int i = 0; i < N_LEN_BUF_2BYTE ; i++) {
      tmp16 = ((int16_t)(ptmp16[i]));
      pu16Strm[i + N_LEN_BUF_2BYTE*j] = tmp16;
    } // i
    //ui8Side = (ui8Side+1)&1;

    // I2S : https://docs.espressif.com/projects/esp-idf/en/v3.3/api-reference/peripherals/i2s.html
    //erReturns = i2s_read(I2S_NUM_0, (char *)gi8BufAll[ui8Side], N_LEN_BUF_ALL_BYTES, &uiGotLen,portMAX_DELAY);
    //erReturns must be ESP_OK  //https://docs.espressif.com/projects/esp-idf/en/v3.3/api-reference/system/esp_err.html
    //uiGotLen must be == N_LEN_BUF_ALL_BYTES

  } // j

  //vTaskDelete(gxHandle);
  Serial.print(uiGotLen);
  Serial.print(" , ");
  Serial.print(erReturns);
  Serial.println("");
}


void getEprom(void){
  if(objEprom.begin("myParams", true)==true) // readonly mode
  {
    String ssidTa = objEprom.getString("ssid");
    String passTa = objEprom.getString("wifipass");
    String servTa = objEprom.getString("serv");
    objEprom.end();

    ssidC = (char *)malloc(sizeof(char)*(ssidTa.length()+1));
    passC = (char *)malloc(sizeof(char)*(passTa.length()+1));
//    servC = (char *)malloc(sizeof(char)*(servTa.length()+1));

    if (servTa.length()<1)
    {
      Serial.println("failed reading EPROM ");
      return;
    }

    ssidTa.toCharArray(ssidC, ssidTa.length()+1 );
    passTa.toCharArray(passC, passTa.length()+1 );
//    servTa.toCharArray(servC, servTa.length()+1 );

    Serial.println(String(ssidC));
    Serial.println(String(passC));
    Serial.println(String(servC));
    Serial.println("read EPROM ... done");
  }
  else
  {
    objEprom.end();  
    Serial.println("failed reading EPROM ");
  }
}

void initWifiClient(void){
  Serial.print("Connecting to ");
  Serial.println(ssidC);

  WiFi.begin( ssidC, passC);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

int32_t httppostWav2(void  ){

  uint32_t iNumDat = 2*N_ITTER_READ_BUF*N_LEN_BUF_2BYTE ;

  String stMyURL="";
  stMyURL+=URL1;
  stMyURL+=String(servC);
  stMyURL+=URL2;
  myHttp.begin(stMyURL);
    
  String stConType ="";
  stConType +="multipart/form-data; boundary=";
  stConType +=STRING_BOUNDARY;
  myHttp.addHeader("Content-Type", stConType);

  String stMHead="";
  stMHead += "--";
  stMHead += STRING_BOUNDARY;
  stMHead += "\r\n";
  stMHead += STRING_MULTIHEAD02;
  stMHead += "\r\n";
  stMHead += STRING_MULTIHEAD03;
  stMHead += "\r\n";
  stMHead += "\r\n";
  uint32_t iNumMHead = stMHead.length();

  String stMTail="";
  stMTail += "\r\n";
  stMTail += "--";
  stMTail += STRING_BOUNDARY;
  stMTail += "--";
  stMTail += "\r\n";
  stMTail += "\r\n";  
  uint32_t iNumMTail = stMTail.length();

  uint32_t iNumTotalLen = iNumMHead + iNumMTail + iNumDat +44;

  uint8_t *uiB = (uint8_t *)malloc(sizeof(uint8_t)*iNumTotalLen);

  for(int uilp=0;uilp<iNumMHead;uilp++){
    uiB[0+uilp]=stMHead[uilp];
  }
  
  CreateWavHeader((byte *)&uiB[iNumMHead],iNumDat/2);

  getData2((uint16_t *)&uiB[iNumMHead+44]);

  for(int uilp=0;uilp<iNumMTail;uilp++){
    uiB[iNumMHead+iNumDat+44+uilp]=stMTail[uilp];
  }

  int32_t httpResponseCode = (int32_t)myHttp.POST(uiB,iNumTotalLen);
  myHttp.end();
  free(uiB);
  return (httpResponseCode);
}



//ref to https://github.com/MhageGH/esp32_SoundRecorder/blob/master/esp32_I2S_recorder/Wav.cpp
void CreateWavHeader(byte* header, int waveDataSize){
  header[0] = 'R';
  header[1] = 'I';
  header[2] = 'F';
  header[3] = 'F';
  unsigned int fileSizeMinus8 = waveDataSize + 44 - 8;
  header[4] = (byte)(fileSizeMinus8 & 0xFF);
  header[5] = (byte)((fileSizeMinus8 >> 8) & 0xFF);
  header[6] = (byte)((fileSizeMinus8 >> 16) & 0xFF);
  header[7] = (byte)((fileSizeMinus8 >> 24) & 0xFF);
  header[8] = 'W';
  header[9] = 'A';
  header[10] = 'V';
  header[11] = 'E';
  header[12] = 'f';
  header[13] = 'm';
  header[14] = 't';
  header[15] = ' ';
  header[16] = 0x10;  // linear PCM
  header[17] = 0x00;
  header[18] = 0x00;
  header[19] = 0x00;
  header[20] = 0x01;  // linear PCM
  header[21] = 0x00;
  header[22] = 0x01;  // monoral
  header[23] = 0x00;
  header[24] = 0x44;  // sampling rate 44100
  header[25] = 0xAC;
  header[26] = 0x00;
  header[27] = 0x00;
  header[28] = 0x88;  // Byte/sec = 44100x2x1 = 88200
  header[29] = 0x58;
  header[30] = 0x01;
  header[31] = 0x00;
  header[32] = 0x02;  // 16bit monoral
  header[33] = 0x00;
  header[34] = 0x10;  // 16bit
  header[35] = 0x00;
  header[36] = 'd';
  header[37] = 'a';
  header[38] = 't';
  header[39] = 'a';
  header[40] = (byte)(waveDataSize & 0xFF);
  header[41] = (byte)((waveDataSize >> 8) & 0xFF);
  header[42] = (byte)((waveDataSize >> 16) & 0xFF);
  header[43] = (byte)((waveDataSize >> 24) & 0xFF);
}


void setup() {
  Serial.begin(115200);
  getEprom();
  initWifiClient();
  myHttp.setReuse(true);

  I2S_Init();
  //skipNoisySound();
  pinMode(PIN_LED_STICKC,OUTPUT);
  pinMode(PIN_BTN1_STICKC,INPUT);

  xTaskCreate(taskI2sReading, "taskI2sReading", 2048, NULL, 1, &gxHandle);

  Serial.println("---push button");
  Serial.flush();

}

volatile uint32_t gu32Cnt=0;
void loop() {

  if(gu32Cnt < 0x10000)
    digitalWrite(PIN_LED_STICKC,0);
  else
    digitalWrite(PIN_LED_STICKC,1);

  gu32Cnt++;
  gu32Cnt &= 0x000fffff;
  //skipNoisySound();
  //Serial.println(digitalRead(PIN_BTN1_STICKC));
  if(digitalRead(PIN_BTN1_STICKC)==0){

    Serial.println("---start---");
    digitalWrite(PIN_LED_STICKC,0);
    //int iRetHttp = httppostWav( (uint8_t *)&gi16strmData[0] , );
    int iRetHttp = httppostWav2();
    if (iRetHttp==200)
    {
      Serial.println("---post done---");
    }
    else
    {
      Serial.println("");
      Serial.print("failure: http post. return code: ");
      Serial.println(iRetHttp);
    }
    digitalWrite(PIN_LED_STICKC,1);
    Serial.println("---rec done---");
  }

}
