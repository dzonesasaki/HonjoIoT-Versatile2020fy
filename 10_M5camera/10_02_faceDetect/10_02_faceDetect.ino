#include <WiFi.h>
//#include <WiFiClient.h>
#include <HTTPClient.h>
#include <Preferences.h>
Preferences objEprom;
#include <esp_camera.h>
#include "fb_gfx.h"
#include "fd_forward.h"

#define URL1 "http://"
#define URL2 ":1880/mypost02"

char *ssidC="SSID";
char *passC="PASS"; 
char *servC="192.168.1.2";//server address
HTTPClient myHttp;


#define STRING_BOUNDARY "123456789000000000000987654321"
#define STRING_MULTIHEAD02 "Content-Disposition: form-data; name=\"uploadFile\"; filename=\"./testfig2.jpg\""
#define STRING_MULTIHEAD03 "Content-Type: image/jpg"


void getEprom(void){
  if(objEprom.begin("myParams", true)==true) // readonly mode
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


// for m5camera model B
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    15
#define XCLK_GPIO_NUM     27
#define SIOD_GPIO_NUM     22 //25
#define SIOC_GPIO_NUM     23

#define Y9_GPIO_NUM       19
#define Y8_GPIO_NUM       36
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       39
#define Y5_GPIO_NUM        5
#define Y4_GPIO_NUM       34
#define Y3_GPIO_NUM       35
#define Y2_GPIO_NUM       32
#define VSYNC_GPIO_NUM    25 //22
#define HREF_GPIO_NUM     26
#define PCLK_GPIO_NUM     21

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


void initCam(void){
  // ref to https://www.mgo-tec.com/blog-entry-esp32-ov2640-ssd1331-test1.html/3
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.jpeg_quality = 10;
  config.frame_size = FRAMESIZE_QVGA;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    while(1){}
    return;
  }
}



int32_t httppost( uint8_t * ui8BufJpg, uint32_t iNumDat ){

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

  uint32_t iNumTotalLen = iNumMHead + iNumMTail + iNumDat;

  uint8_t *uiB = (uint8_t *)ps_malloc(sizeof(uint8_t)*iNumTotalLen);

  for(int uilp=0;uilp<iNumMHead;uilp++){
    uiB[0+uilp]=stMHead[uilp];
  }
  for(int uilp=0;uilp<iNumDat;uilp++){
    uiB[iNumMHead+uilp]=ui8BufJpg[uilp];
  }
  for(int uilp=0;uilp<iNumMTail;uilp++){
    uiB[iNumMHead+iNumDat+uilp]=stMTail[uilp];
  }

  int32_t httpResponseCode = (int32_t)myHttp.POST(uiB,iNumTotalLen);
  myHttp.end();
  free(uiB);
  return (httpResponseCode);
}



#define FACE_COLOR_WHITE  0x00FFFFFF
#define FACE_COLOR_BLACK  0x00000000
#define FACE_COLOR_RED    0x000000FF
#define FACE_COLOR_GREEN  0x0000FF00
#define FACE_COLOR_BLUE   0x00FF0000
#define FACE_COLOR_YELLOW (FACE_COLOR_RED | FACE_COLOR_GREEN)
#define FACE_COLOR_CYAN   (FACE_COLOR_BLUE | FACE_COLOR_GREEN)
#define FACE_COLOR_PURPLE (FACE_COLOR_BLUE | FACE_COLOR_RED)

static void draw_face_boxes(dl_matrix3du_t *image_matrix, box_array_t *boxes, int face_id){
  int x, y, w, h, i;
  uint32_t color = FACE_COLOR_YELLOW;
  if(face_id < 0){
    color = FACE_COLOR_RED;
  } else if(face_id > 0){
    color = FACE_COLOR_GREEN;
  }
  fb_data_t fb;
  fb.width = image_matrix->w;
  fb.height = image_matrix->h;
  fb.data = image_matrix->item;
  fb.bytes_per_pixel = 3;
  fb.format = FB_BGR888;
  for (i = 0; i < boxes->len; i++){
    // rectangle box
    x = (int)boxes->box[i].box_p[0];
    y = (int)boxes->box[i].box_p[1];
    w = (int)boxes->box[i].box_p[2] - x + 1;
    h = (int)boxes->box[i].box_p[3] - y + 1;
    fb_gfx_drawFastHLine(&fb, x, y, w, color);
    fb_gfx_drawFastHLine(&fb, x, y+h-1, w, color);
    fb_gfx_drawFastVLine(&fb, x, y, h, color);
    fb_gfx_drawFastVLine(&fb, x+w-1, y, h, color);
#if 0
    // landmark
    int x0, y0, j;
    for (j = 0; j < 10; j+=2) {
      x0 = (int)boxes->landmark[i].landmark_p[j];
      y0 = (int)boxes->landmark[i].landmark_p[j+1];
      fb_gfx_fillRect(&fb, x0, y0, 3, 3, color);
    }
#endif
  }
}


void setup() {
  Serial.begin(115200);
  //getEprom();
  initWifiClient();
  myHttp.setReuse(true);
  initCam();
}


void loop() {

  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("failure: Camera capture");
    delay(1000);
    return;
  }

  uint16_t widthImage=fb->width;
  uint16_t heightImage=fb->height;
  int num_components = 3;
  uint32_t lengthImage = widthImage * heightImage * num_components;
  int quality = 80;
  dl_matrix3du_t *image_matrix = dl_matrix3du_alloc(1, widthImage, heightImage, 3);
  uint8_t *out_buf = image_matrix->item;
  bool bFlug888Conv = fmt2rgb888(fb->buf, lengthImage , PIXFORMAT_JPEG, out_buf);

  esp_camera_fb_return(fb);

  if(!bFlug888Conv){
    Serial.println("failure: fmt2rgb888");
    delay(1000);
    return;
  }
  mtmn_config_t mtmn_config = mtmn_init_config();
  box_array_t *net_boxes = face_detect(image_matrix, &mtmn_config);

  int face_id = 0;  
  if (net_boxes){
    draw_face_boxes(image_matrix, net_boxes, face_id);
    Serial.println("done: draw_face_boxes");
//  free(net_boxes->score);
//  free(net_boxes->box);
//  free(net_boxes->landmark);
//  free(net_boxes);
  }
  else{
    Serial.println("no faces");
  }

  uint8_t * ui8BufJpg;
  uint32_t iNumDat ;

  bool bFlugJpegConv = fmt2jpg(image_matrix->item,lengthImage, widthImage, heightImage,PIXFORMAT_RGB888 ,quality , &ui8BufJpg, &iNumDat);
  dl_matrix3du_free(image_matrix);
  int iRetHttp = httppost( ui8BufJpg , iNumDat);
  free(ui8BufJpg);
  if (iRetHttp==200)
  {
    Serial.print("*");
  }
  else
  {
    Serial.println("");
    Serial.print("failure: http post. return code: ");
    Serial.println(iRetHttp);
  }
    delay(50);
}
