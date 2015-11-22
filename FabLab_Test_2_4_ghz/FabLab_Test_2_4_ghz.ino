
/******************************************************************************* 
* Example testing sketch for:
 - DHT humidity/temperature sensors
 - DSB18B20 Water Proof Temperature Sensor
 - Soil Moisture
 - NRF24L01 Emission
 SC Fab Lab
 Written by Vincent(dot)Cruvellier(@)gmail(dot)com, public domain 
********************************************************************************/

#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <DHT.h>      // DHT library
#include <OneWire.h>  // OneWire Library
#include <DallasTemperature.h>  // DallasTemperature Library
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include "sensor_payload.h"   // Payload Definition, enclosed in MaStation Git Repository

#define SOILPIN           A0    // Signal Pin For Soil Moisture
#define SOILPIN_POWER     9     // SignalPower Pin For Soil Moisture
#define DHTPIN            2     // Signal Pin for DHT
#define DHTPIN_POWER      4     // Signal Power Pin for DHT
#define DSBPIN            5     // Signal Pin for DSB 
                              // Note: Make sure to have 47k pull up between Signal and VDD.
#define DSBPIN_POWER      6     // Signal Power Pin for DSB                         
#define NRF24L01_CE_PIN   7    // nRF24L01 SPI CE Pin 
#define NRF24L01_CSN_PIN  8   // nRF24L01 SPI CSN Pin 
#define NRFPIN_POWER      10  // Signal Pin Power for NRF24
                              // Note: add a capacitor 22µF on this Pin & GND
#define DHTTYPE DHT21   // DHT 21 (AM2301)

/********************************************************************************
* Initialisation of the differents module
*  - DSB => need OneWire Connection, DallasTemperature lib
*  - DHT => need DHT lib
*  - RF24 => Use SPI, need of nrf24 lib
*/
// DSB INIT
OneWire ds(DSBPIN);
byte DSB_addr[8]; // use to store DSB adress, done at init
// DHT INIT
DHT dht(DHTPIN, DHTTYPE, 3); // AVR_SPEED=3 for 8Mhz Else 6 For 16Mhz
// RF24 INIT
RF24 radio(NRF24L01_CE_PIN, NRF24L01_CSN_PIN);

/**********************************************************************************
 - Paylaod init
 - RF24 Configuration
*/
Payload payload = (Payload) {
  SENSOR_STATION
};
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL }; // Channel to use
#define STATION_ID 1 // Unique Identifier of actual Station
#define PAYLOAD_NODE 1 // Only one node for this sation demo
#define NRF24_CHANNEL 12
#define NRF24_SPEED  RF24_250KBPS  // RF24_250KBPS for 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS for 2Mbps

int start_counter = 10; // Send data all minutes during 10 minutes
int normal_counter = 15; // Send data all 15 minutes

/********************************************************************************
* long readVcc() 
*  - return the internal Vcc value in mV
*/
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

/********************************************************************************
* void power_on (int powerPin )
*  - set High level to powerPin
*/
void power_on (int powerPin) {
  pinMode(powerPin,OUTPUT);
  digitalWrite(powerPin,HIGH);
  delay(200); // delay for power sloop
}

/********************************************************************************
* void power_off (int powerPin)
*  - set Low level to powerPin
*/
void power_off (int powerPin) {
  pinMode(powerPin,OUTPUT);
  digitalWrite(powerPin,LOW);
}


/*********************************************************************************
* void print_dht()
*  - print dht sensor values
*  - send Payload values
*/
void print_dht() { 
  delay(200); // let time to warm up the DHT
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
  // Prepare & Send the Payload
  payload.type = SENSOR_DHT;
  payload.data.SENSOR_DHT.humidity = h;
  payload.data.SENSOR_DHT.temperature = t;
  payload.data.SENSOR_DHT.dewpoint = hi;
  payload.data.SENSOR_DHT.status = 0;
  doSendMsg();  
}

/*********************************************************************************
* void init_DSB_addr_dsb()
*  Seek for a DSB onewire module, set addr for futur use
*/
void init_DSB_addr_dsb() {

  if ( !ds.search(DSB_addr)) {
    Serial.println("No more DSB_addresses.");
    Serial.println();
    ds.reset_search();
    delay(250);
  }
  // Display the ROM addr content
  Serial.print("ROM =");
  for( int i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(DSB_addr[i], HEX);
  }
  Serial.print('  =>  ');
 
  // the first ROM byte indicates which chip
  switch (DSB_addr[0]) {
    case 0x10:
      Serial.println("  Chip = DS18S20");  // or old DS1820
      break;
    case 0x28:
      Serial.println("  Chip = DS18B20");
      break;
    case 0x22:
      Serial.println("  Chip = DS1822");
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return;
  }  
}

/*********************************************************************************
* void print_dsb()
*  - print DSB sensor values
*  - send Payload values
*/
void print_dsb() {
  
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];

  float celsius, fahrenheit;
   
  if (OneWire::crc8(DSB_addr, 7) != DSB_addr[7]) {
      Serial.println("CRC is not valid!");
      init_DSB_addr_dsb(); // exit and request a seek for DSB before
      return;
  }
 
  // the first ROM byte indicates which chip
  switch (DSB_addr[0]) {
    case 0x10:
      type_s = 1;
      break;
    case 0x28:
      type_s = 0;
      break;
    case 0x22:
      type_s = 0;
      break;
    default:
      Serial.println("DSB Error");
      return;
  } 

  ds.reset();
  ds.select(DSB_addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(DSB_addr);    
  ds.write(0xBE);         // Read Scratchpad

  /*Serial.print("  Data = ");
  Serial.print(present, HEX);
  Serial.print(" ");*/
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    /*Serial.print(data[i], HEX);
    Serial.print(" ");*/
  }
  /*Serial.print(" CRC=");
  Serial.print(OneWire::crc8(data, 8), HEX);
  Serial.println();*/

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  
  float tempC = celsius; //dsb.getTempCByIndex(0);
  float tempF = fahrenheit; //dsb.toFahrenheit(tempC);
  //power_off(DSBPIN_POWER);  // Power Off Module
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

/*********************************************************************************
* void print_soil()
*  - print SOIL sensor values
*  - send Payload values
*/
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

/*********************************************************************************
* void print_station()
*  - print Station sensor values
*  - send Payload values
*/
void print_station() {  
  payload.type =  SENSOR_STATION;
  uint16_t vcc = (uint16_t) readVcc();
  payload.data.SENSOR_STATION.powerVoltage = vcc;
  payload.data.SENSOR_STATION.status = 0;
  Serial.print("STATION:\t");
  Serial.print(payload.stationId);
  Serial.print("\t");
  Serial.print("Vcc:");
  Serial.print(vcc);
  Serial.print("\t\n");
  // Prepare and send the Payload
  doSendMsg();
}

/*********************************************************************************
* bool doSendMsg
*  - Open NRF24L01 writing pipe
*  - send Payload
*/
bool doSendMsg()
{
  radio.stopListening();
  radio.openWritingPipe(pipes[0]);
  bool done = radio.write(&payload, sizeof(payload),0);
  if (done) {
    printf("OK %d ", payload.type);
  }
  else {
    printf("NOK ");
  }
  printf("Tr %d bytes/ max %u -> [%u/%u/%u]\n", sizeof(payload), radio.getPayloadSize(), done, true, radio.isAckPayloadAvailable());
  radio.startListening();
}

/*********************************************************************************
* void mywatchdogenable()
*  - set watch dog & sleep function
*/
void mywatchdogenable() 
{ 
  MCUSR = 0; 
  WDTCSR = _BV (WDCE) | _BV (WDE); 
  WDTCSR = _BV (WDIE) | _BV (WDP3) | _BV (WDP0); //délai de 8 secondes 
  wdt_reset(); 
  //ADCSRA = 0; //désactive ADC 
  set_sleep_mode (SLEEP_MODE_PWR_DOWN); 
  sleep_enable(); 
  MCUCR = _BV (BODS) | _BV (BODSE); 
  MCUCR = _BV (BODS); 
  sleep_cpu(); 
  sleep_disable(); 
}

/*********************************************************************************
* void setupNRF24()
*  - Initialize NRF24 Moduke and configure it
*/
void setupNRF24() {
  // NRF24 init
  radio.begin(); // Setup and configure 2.4ghz rf radio
  radio.powerUp(); //alimente le module nrf24l01+ 
  radio.setRetries(15, 15);
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
  payload.stationId = STATION_ID;
  payload.type =  SENSOR_STATION;
  payload.version = PAYLOAD_VERSION; // Version de Payload from sensor_payload.h
  payload.node = PAYLOAD_NODE; // Only one node in this station 
}

/*********************************************************************************
* void setup()
*  - setup() !
*/
void setup() {
  Serial.begin(57600); 
  Serial.println("SC Fab Lab Test !");
  power_on(DHTPIN_POWER);  // power on module
  power_on(DSBPIN_POWER);  // power on module
  power_on(SOILPIN_POWER);  // power on module
  power_on(NRFPIN_POWER);  // power on module  
  delay(5000); // Necessary Warm up
  dht.begin(); // DHT Initialisation
  printf_begin(); // Necessary functio initialisation
  setupNRF24(); // Setup NRF24
  init_DSB_addr_dsb();  // set DSB addr
  print_station(); // Send hello to the Word !
}

/*********************************************************************************
* void loop()
*  - loop() !
*/
void loop() {
  // Wait a few seconds between measurements.
  power_on(DHTPIN_POWER);  // power on module
  power_on(DSBPIN_POWER);  // power on module
  power_on(SOILPIN_POWER);  // power on module
  power_on(NRFPIN_POWER);  // power on module    
  delay(5000);  // warm up
  setupNRF24();
  delay(1000);
  print_station();  
  print_dht();
  print_dsb();
  print_soil();
  print_station(); 
  delay(2000);
  Serial.println("Go to Bed ..."); 
  radio.stopListening();
  radio.powerDown();
  delay(1000);
  power_off(NRFPIN_POWER);  // Power Off Module  
  power_off(DHTPIN_POWER);  // Power Off Module
  power_off(DSBPIN_POWER);  // Power Off Module
  power_off(SOILPIN_POWER);  // Power Off Module
  power_adc_disable();
  //power_all_disable ();   // turn off all modules
  if (start_counter > 0) {
    for (int i=0; i < 1; i++) {//mise en veille pendant 8 secondes 
      mywatchdogenable();
    } 
    start_counter--;
  } else {
    for (int i=0; i < (8*normal_counter); i++) {//mise en veille pendant 64 secondes 
      mywatchdogenable();
    } 
  }
  power_adc_enable();
  power_all_enable();
  Serial.println("Wake up !!!"); 
}
