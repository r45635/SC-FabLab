
/* Example testing sketch for:
 - DHT humidity/temperature sensors
 - DSB18B20 Water Proof Temperature Sensor
 - Soil Moisture
 - NRF24L01 Emission
 SC Fab Lab
 Written by Vincent, public domain 
*/

#include <DHT.h>      // DHT library
#include <OneWire.h>  // OneWire Library
#include <DallasTemperature.h>  // DallasTemperature Library
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include "sensor_payload.h"   // Payload Definition, enclosed in MaStation Git Repository
#include "Narcoleptic.h"

#define SOILPIN   A0    // Signal Pin For Soil Moisture
                        // Nice Example here : 
#define DHTPIN    2     // Signal Pin for DHT
#define DSBPIN    5     // Signal Pin for DSB 
                        // Note: Make sure to have 47k pull up between Signal and VDD.
#define NRF24L01_CE_PIN   7    // nRF24L01 SPI CE Pin 
#define NRF24L01_CSN_PIN  8   // nRF24L01 SPI CSN Pin 

#define DHTTYPE DHT21   // DHT 21 (AM2301)

OneWire oneWire(DSBPIN);
DallasTemperature dsb(&oneWire);
DHT dht(DHTPIN, DHTTYPE, 3); // AVR_SPEED=3 for 8Mhz Else 6 For 16Mhz

RF24 radio(NRF24L01_CE_PIN, NRF24L01_CSN_PIN);
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL }; // Channel to use
#define NRF24_CHANNEL 12
#define NRF24_SPEED  RF24_250KBPS  // RF24_250KBPS for 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS for 2Mbps

Payload payload = (Payload) {
  SENSOR_STATION
};
#define STATION_ID 1 // Unique Identifier of actual Station


long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}


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
  payload.type = SENSOR_DHT;
  payload.data.SENSOR_DHT.humidity = h;
  payload.data.SENSOR_DHT.temperature = t;
  payload.data.SENSOR_DHT.dewpoint = hi;
  payload.data.SENSOR_DHT.status = 0;
  doSendMsg();  
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
  // Prepare and send the Payload
  payload.type = SENSOR_DSB;
  payload.data.SENSOR_DSB.temperature = tempC;
  payload.data.SENSOR_DSB.status = 0;
  doSendMsg();   
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
  // Prepare and send the Payload
  payload.type = SENSOR_SOIL;
  payload.data.SENSOR_SOIL.analogValue = soilvalue;
  payload.data.SENSOR_SOIL.status = 0;
  doSendMsg(); 
}
  

bool doSendMsg()
{
  radio.stopListening();
  radio.openWritingPipe(pipes[0]);
  bool done = radio.write(&payload, sizeof(payload));
  if (done) {
    printf("OK %d ", payload.type);
  }
  else {
    printf("NOK ");
  }
  printf("Tr %d bytes / max %u -> [%u/%u/%u]\n", sizeof(payload), radio.getPayloadSize(), done, true, radio.isAckPayloadAvailable());
  radio.startListening();
}

void setup() {
  Serial.begin(57600); 
  Serial.println("SC Fab Lab Test !");
  dht.begin(); // DHT Initialisation
  dsb.begin(); // DSB Initialisation
  delay(2000); // Necessary Warm up
  printf_begin(); // Necessary functio initialisation
  // NRF24 init
  radio.begin(); // Setup and configure 2.4ghz rf radio
  radio.setRetries(5, 15);
  radio.setChannel(NRF24_CHANNEL);
  radio.setDataRate(NRF24_SPEED);  
  //radio.setAutoAck(1);                    // Ensure autoACK is enabled
  radio.enableDynamicPayloads();
  //radio.disableCRC(); //CRC enabled by default
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1, pipes[1]);
  radio.startListening();
  printf("//NRF24 Module Sensor Enabled.\n");
  radio.printDetails();
  payload.type =  SENSOR_STATION;
  payload.version = PAYLOAD_VERSION; // Version de Payload from sensor_payload.h
  payload.node = 1; // Only one node in this station
  payload.data.SENSOR_STATION.stationId = 100;
  payload.data.SENSOR_STATION.powerVoltage = 5 ;
  payload.data.SENSOR_STATION.status = 100;
  doSendMsg();  
}


void loop() {
  // Wait a few seconds between measurements.
  //delay(180000);
   Narcoleptic.delay(300000); // milisecond
  print_dht();
  print_dsb();
  print_soil();
  Serial.println();
  payload.type =  SENSOR_STATION;
  payload.node = 1;
  payload.data.SENSOR_STATION.stationId = 99;
  uint16_t vcc = (uint16_t) readVcc();
  payload.data.SENSOR_STATION.powerVoltage = vcc;
  payload.data.SENSOR_STATION.status = 99;
  doSendMsg();
}
