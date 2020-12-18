#include <DHT.h>

const int PIN_DHT = 33;
DHT dht( PIN_DHT, DHT11 );

void setup() {
  Serial.begin(115200);
  Serial.println("DHT11");
  dht.begin();
}

void loop() {

  delay(3000);

  bool isFahrenheit = false;
  float percentHumidity = dht.readHumidity();
  float temperature = dht.readTemperature( isFahrenheit );

  if (isnan(percentHumidity) || isnan(temperature)) {
    Serial.println("ERROR");
    return;
  }

  float heatIndex = dht.computeHeatIndex(
    temperature, 
    percentHumidity, 
    isFahrenheit);

  String s = "Temp: ";
  s += String(temperature, 1);
  s += "[c] Humidity: ";
  s += String(percentHumidity, 1);
  s += "[%] HI: ";
  s += String(heatIndex, 1);

  Serial.println(s);
  
}
