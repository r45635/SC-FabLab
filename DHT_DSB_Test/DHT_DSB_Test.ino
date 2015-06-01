/* Example testing sketch for:
 - DHT humidity/temperature sensors
 - DSB18B20 Water Proof Temperature Sensor

 SC Fab Lab
 Written by Vincent, public domain 
*/

#include <DHT.h>      // DHT library
#include <OneWire.h>  // OneWire Library
#include <DallasTemperature.h>  // DallasTemperature Library

#define SOILPIN   A0    // Signal Pin For Soil Moisture
                        // Nice Example here : 
#define DHTPIN    2     // Signal Pin for DHT
#define DSBPIN    5     // Signal Pin for DSB 
                     // Note: Make sure to have 47k pull up between Signal and VDD.

#define DHTTYPE DHT21   // DHT 21 (AM2301)
#define AVR_SPEED 6 // For 16Mhz
#define AVR_SPEED 3 // For 8Mhz

OneWire oneWire(DSBPIN);
DallasTemperature dsb(&oneWire);
DHT dht(DHTPIN, DHTTYPE, AVR_SPEED); 

void print_dht() {
  float h = dht.readHumidity();
  // Read temperature as Celsius
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Compute heat index
  // Must send in temp in Fahrenheit!
  float hi = dht.computeHeatIndex(f, h);
  Serial.print("DHT:\t");
  Serial.print("Temperature: "); 
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print(f);
  Serial.print(" *F\t");
  Serial.print("Humidity: "); 
  Serial.print(h);
  Serial.print(" %\t");  
  Serial.print("Heat index: ");
  Serial.print(hi);
  Serial.println(" *F");
}

void print_dsb() {
  dsb.requestTemperatures();
  float tempC = dsb.getTempCByIndex(0);
  float tempF = dsb.toFahrenheit(tempC);
  
  Serial.print("DSB:\t");
  Serial.print("Temperature: "); 
  Serial.print(tempC);
  Serial.print(" *C ");
  Serial.print(tempF);
  Serial.print(" *F\n");  
}

void print_soil() {
  int soilvalue = analogRead(SOILPIN);
  Serial.print("SOIL:\t");
  Serial.print(soilvalue);
  Serial.print("\t");
  if (soilvalue > 1000 ) Serial.println("Soil Moisture Sensor in Air ?"); 
  else if (soilvalue > 700) Serial.println("Dry soil !");
  else if (soilvalue > 300) Serial.println("Humid soil !");
  else Serial.println("Sensor in Water !");
}
  

void setup() {
  Serial.begin(57600); 
  Serial.println("SC Fab Lab Test !");
  dht.begin(); // DHT Initialisation
  dsb.begin(); // DSB Initialisation
  delay(2000); // Necessary Warm up
  
}



void loop() {
  // Wait a few seconds between measurements.
  delay(2000);
  print_dht();
  print_dsb();
  print_soil();
  Serial.println();
}
