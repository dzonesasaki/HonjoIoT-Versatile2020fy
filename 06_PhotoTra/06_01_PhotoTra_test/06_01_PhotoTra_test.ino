float gfAveragedVoltage;

void setup() {
  Serial.begin(115200);
  pinMode(33,INPUT);
  gfAveragedVoltage = 0;
}

void loop() {
  float fCurrentVoltage = analogRead(33) / (float)4096 * 3.3 ;
  gfAveragedVoltage = fCurrentVoltage  * 0.1 + gfAveragedVoltage * (1-0.1); 
  Serial.println(gfAveragedVoltage);
  delay(10);
}
