#include <ESP32Servo.h> // https://github.com/madhephaestus/ESP32Servo
#include <WiFi.h>
#include <WebServer.h>

// ref to https://github.com/madhephaestus/ESP32Servo/blob/master/examples/Multiple-Servo-Example-Arduino/Multiple-Servo-Example-Arduino.ino
// ref to example/SimpleWifiWerver.ino

// Wifi AP settings
const char gcc_SSID[] = "SSID";
const char gcc_PASSWORD[] = "PASSWORD";

// These are all GPIO pins on the ESP32
// Recommended pins include 2,4,12-19,21-23,25-27,32-33 
#define PIN_SERVO_PWM 25

// Published values for SG90 servos; adjust if needed
#define WIDTH_PWM_MICRO_SEC_MIN ((int)500)
#define WIDTH_PWM_MICRO_SEC_MAX ((int)2400)

// create four servo objects 
Servo objServo01;

int giAngleDeg = 0;      // position in degrees

WebServer myWebServer(80);


void init_WifiClient(void)
{
  Serial.print("Connecting to ");
  Serial.println(gcc_SSID);
  
  WiFi.begin( gcc_SSID, gcc_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}//init_WifiClient()


void doRotServoOneShot(void)
{
  if ( giAngleDeg < 0 )
  {
    giAngleDeg = 0;
  }
  if ( giAngleDeg > 180 )
  {
    giAngleDeg = 180;
  }
  objServo01.write(giAngleDeg);
}

void doRotServoKnob(void)
{
  objServo01.attach(PIN_SERVO_PWM, WIDTH_PWM_MICRO_SEC_MIN, WIDTH_PWM_MICRO_SEC_MAX);
  for (int iAng=0;iAng<=180;iAng++)
  {
    objServo01.write(iAng);
    delay(15);
  }
  for (int iAng=180;iAng>0;iAng--)
  {
    objServo01.write(iAng);
    delay(15);
  }
 objServo01.detach();
}


void handleRootGet() {
  String html = "";
  html += "<h1>Servo motor</h1>";
  html += "<h1> push hear -> <a href=\"/turn\">Rotate </a></h1>";

  myWebServer.send(200, "text/html",html);
}


void handle2ndGet() {
  String html = "";
  html += "<h1>turning Servo motor</h1>";
  html += "<h1> push hear -> <a href=\"/turn\">Rotate </a></h1>";
  myWebServer.send(200, "text/html",html);
  doRotServoKnob();
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += myWebServer.uri();
  message += "\nMethod: ";
  message += (myWebServer.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += myWebServer.args();
  message += "\n";
  for (uint8_t i=0; i<myWebServer.args(); i++){
    message += " " + myWebServer.argName(i) + ": " + myWebServer.arg(i) + "\n";
  }
  myWebServer.send(404, "text/plain", message);
}


void setup() {
  Serial.begin(115200);

  objServo01.setPeriodHertz(50);      // Standard 50Hz servo
  objServo01.attach(PIN_SERVO_PWM, WIDTH_PWM_MICRO_SEC_MIN, WIDTH_PWM_MICRO_SEC_MAX);

  init_WifiClient();

  myWebServer.on("/", HTTP_GET,handleRootGet);
  myWebServer.on("/turn", HTTP_GET,handle2ndGet);
  myWebServer.onNotFound(handleNotFound);

  myWebServer.begin();
  Serial.println("HTTP server started");
}

void loop() {
  myWebServer.handleClient();
}
