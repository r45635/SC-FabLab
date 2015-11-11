/*
 Copyright (C) 2015 Vincent Cruvellier <vincent(at)cruvellier(dot)eu>
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */
#include <inttypes.h>
#define PAYLOAD_VERSION 0;  // Demo Use
// Version 0.0 - Demo Version

#define PACK __attribute__((packed))  // Important declaration to make sure cross-platform compilation of structure are equivalent

// PayloadType: Enumeration of Payload Type to be managed 
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//   DO NOT CHANGE THE ORDER, NEVER REMOVE OR REPLACE - ONLY APPEN PAYLOAD TYPE at THE END
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
enum PayloadType {
  SENSOR_STATION,    // Generic Station Payload 
  SENSOR_DHT,  			// DHT21,DHT22 Sensor Type
  SENSOR_SOIL, 			// Soil Moisture Sensor Type
  SENSOR_DSB        // WaterProof DSB Sensor type
}
PACK;

typedef uint8_t vn_payload_type;    // Type of Payload see PayloadType
typedef uint8_t vn_payload_version; // Version of Payload, Not used.
typedef uint16_t vn_payload_CRC;    // 16bits CRC for the payload, Not Used as Nrf24 CRC & ACK options is used during tranmission

// SENSOR_STATION
typedef struct {
  uint16_t    stationId;             // Sensor Node Identifier
  uint16_t    powerVoltage;        // AnalogValue from Power Supply
  uint8_t     status;
}
PACK vn_sensor_station_t;

// SENSOR_DHT
typedef struct {
  float     humidity;           // Humidity in Percentage
  float     temperature;        // Temperature in Celsius
  float     dewpoint;           // Dewpoint in Celsius
  uint8_t   status;
}
PACK vn_sensor_dht_t;

// SENSOR_SOIL
typedef struct {
  uint16_t  analogValue;					// Raw ANALOG Value, no interpretation
  uint8_t   status;
}
PACK vn_sensor_soil_t;

// SENSOR_DSB
typedef struct {
  float  temperature;          // Temperature in Celsius
  uint8_t   status;
}
PACK vn_sensor_dsb_t;

// Payload Structure
struct Payload {
  vn_payload_type      type;
  vn_payload_version   version;
  uint8_t              stationId;				
  uint8_t              node;
  union {
    vn_sensor_station_t   SENSOR_STATION;
    vn_sensor_dht_t 	    SENSOR_DHT;
    vn_sensor_soil_t	    SENSOR_SOIL;
    vn_sensor_dsb_t       SENSOR_DSB;

  }
  PACK data;
#ifdef CRC_PAYLOAD
  vn_payload_CRC payloadCRC;
#endif
}
PACK;
