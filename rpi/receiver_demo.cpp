// **************************************************************************************
// NRF24 Payload Receive Example
//    Exemple of Typed Payload definition that can be used both from Arduino and Raspberry World
//    This program needs to be stored in  ~/nrf24/rf24libs/RF24/examples_RPi 
//	  This program needs a sensor_payload.h file in order to work correctly.
//    This program needs an access to mysql database
//    This program uses mysql and nrf24 lib, In order to compile this programme please use following 
//
//	g++ -Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -Wall -I../ -lrf24-bcm receiver_demo.cpp `mysql_config --cflags` `mysql_config --libs` -o receiver_demo
//
//  a binary file receiver_demo will be generated.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include "./RF24.h"
#include "sensor_payload.h"


#include <mysql/mysql.h>

using namespace std;
// RF24 Initialization Section
RF24 radio(RPI_V2_GPIO_P1_22,  BCM2835_SPI_CS0, BCM2835_SPI_SPEED_8MHZ);
//NRF24 pipes to be listen
//const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL};
//const uint64_t pipes[2] = {0xF0E0E0E0E1LL, 0xF0E0E0E0D1LL };
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL }; // Channel to use

#define NRF24_CHANNEL 12
#define NRF24_SPEED  RF24_250KBPS  // RF24_250KBPS for 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS for 2Mbps

// a Payload typed variable (defined in sensor_payload.h)
Payload payload = Payload();
// Mysql Necessary information
#define DATABASE_NAME  "STATION_DATA"
#define DATABASE_USERNAME "PI"
#define DATABASE_PASSWORD "RASPBERRY"

MYSQL *mysql1;  		// Var to Mysql Connection
char SQLstring[100];    // Var to build the SQL string to be executed
uint8_t pipeNo;			// Var Pipe number from receiver
int ack_id = 99;

//*****************************************
//*****************************************
//********** CONNECT TO DATABASE **********
//*****************************************
//*****************************************/
void mysql_connect (void)
{
	//initialize MYSQL object for connections
	mysql1 = mysql_init(NULL);
	if(mysql1 == NULL) {	
		fprintf(stderr, "ABB : %s\n", mysql_error(mysql1));
		return;
	}
	//Connect to the database
	if(mysql_real_connect(mysql1, "localhost", DATABASE_USERNAME, DATABASE_PASSWORD, DATABASE_NAME, 0, NULL, 0) == NULL) { 
		fprintf(stderr, "%s\n", mysql_error(mysql1)); 
	}
	else { 
		printf("Database connection successful.\r\n");
	}
}

//**********************************************
//**********************************************
//********** DISCONNECT FROM DATABASE **********
//**********************************************
//**********************************************/
void mysql_disconnect (void)
{
	mysql_close(mysql1);
	printf( "Disconnected from database.\r\n");
}


void setup(void){
	//Prepare the radio module
	printf("\nPreparing NRF24L01 interface\n");
	radio.begin();
	radio.enableDynamicPayloads();
	radio.setRetries( 5, 15);
	radio.setChannel(NRF24_CHANNEL);
	radio.setDataRate(NRF24_SPEED);
	radio.enableAckPayload();
	//radio.disableCRC();
	//radio.setAutoAck(true);
    radio.openWritingPipe(pipes[1]);
    radio.openReadingPipe(1,pipes[0]);
	radio.openReadingPipe(2,pipes[1]);
	radio.printDetails();
	printf("\nPreparing MySQL interface.\n");
	// Connect to MySQL
	mysql_connect();
	if ((mysql1 != NULL)) { // If Succesfull MYSQL then automatically creat if not exits the necessary data table
		sprintf(SQLstring,"CREATE TABLE IF NOT EXISTS SENSOR_DHT (timestamp DATETIME, humidity FLOAT, temperature FLOAT, dewpoint FLOAT);");
		if (!mysql_query(mysql1, SQLstring)) { printf("SQL SENSOR_DHT Table is Ok: %s\n",SQLstring); }  else { printf("SQL SENSOR_DHT NOk: %s\n",SQLstring); printf("%s\n", mysql_error(mysql1)); }
		//ALTER TABLE `SENSOR_DHT` ADD PRIMARY KEY(`timestamp`):
		sprintf(SQLstring,"CREATE TABLE IF NOT EXISTS SENSOR_SOIL (timestamp DATETIME, value INTEGER);");
		if (!mysql_query(mysql1, SQLstring)) { printf("SQL SENSOR_SOIL Table is Ok: %s\n",SQLstring); }  else { printf("SQL SENSOR_SOIL NOk: %s\n",SQLstring); printf("%s\n", mysql_error(mysql1)); }
		sprintf(SQLstring,"CREATE TABLE IF NOT EXISTS SENSOR_DSB (timestamp DATETIME, temperature FLOAT);");
		if (!mysql_query(mysql1, SQLstring)) { printf("SQL SENSOR_DSB Table is Ok: %s\n",SQLstring); }  else { printf("SQL SENSOR_DSB NOk: %s\n",SQLstring); printf("%s\n", mysql_error(mysql1)); }
		sprintf(SQLstring,"CREATE TABLE IF NOT EXISTS SENSOR_STATION (timestamp DATETIME, id INTEGER, vcc INTEGER);");
		if (!mysql_query(mysql1, SQLstring)) { printf("SQL SENSOR_STATION Table is Ok: %s\n",SQLstring); }  else { printf("SQL SENSOR_STATION NOk: %s\n",SQLstring); printf("%s\n", mysql_error(mysql1)); }
		}
	radio.startListening();
	printf("\nNow Listening...\n");

}

/*
************** LOOP Procedure **************
*/
void loop(void) {
	if (radio.available(&pipeNo)) { // Check payload available
		int len = radio.getDynamicPayloadSize();  // Size of the payload to read
		radio.read(&payload, len);	// Read the payload
		printf("%d:",pipeNo); 		// Display the Pipe Channel
		sprintf(SQLstring,";");		// Initialise SQLString to a default value "nothing"
		time_t clk = time(NULL);
        printf("%s", ctime(&clk));
		switch(payload.type) {    
		case SENSOR_DHT:
			printf("SENSOR_DHT:");
			printf("packet %d:%d Temp:%f Humidity:%f Dewpoint:%f status:%u \n", len, sizeof(payload), payload.data.SENSOR_DHT.temperature, payload.data.SENSOR_DHT.humidity, payload.data.SENSOR_DHT.dewpoint,payload.data.SENSOR_DHT.status);
			if (payload.data.SENSOR_DHT.status >= 0) {
				sprintf(SQLstring,"INSERT INTO SENSOR_DHT VALUES(NOW(),%5.1f,%5.1f,%5.1f)",payload.data.SENSOR_DHT.humidity,payload.data.SENSOR_DHT.temperature,payload.data.SENSOR_DHT.dewpoint);
			} else {
				printf("SENSOR_DHT Packet discarded: Status Error (%u)\n",payload.data.SENSOR_DHT.status);
			}
			break;
		case SENSOR_SOIL:
			printf("SENSOR_SOIL:");
			printf("packet %d:%d SOIL:%d status:%u \n", len, sizeof(payload), payload.data.SENSOR_SOIL.analogValue, payload.data.SENSOR_SOIL.status);	
			sprintf(SQLstring,"INSERT INTO SENSOR_SOIL VALUES(NOW(),%d)",payload.data.SENSOR_SOIL.analogValue);
			break;
		case SENSOR_DSB :
			printf("SENSOR_DSB:");
			printf("packet %d:%d Temp:%f status:%u \n", len, sizeof(payload), payload.data.SENSOR_DSB.temperature, payload.data.SENSOR_DSB.status);	
			sprintf(SQLstring,"INSERT INTO SENSOR_DSB VALUES(NOW(),%5.1f)",payload.data.SENSOR_DSB.temperature);
			break;
		case SENSOR_STATION:
			printf("SENSOR_STATION:");
			printf("packet %d:%d ID:%u PowerVoltage:%u status:%u \n", len, sizeof(payload), payload.data.SENSOR_STATION.stationId, payload.data.SENSOR_STATION.powerVoltage, payload.data.SENSOR_STATION.status);
			sprintf(SQLstring,"INSERT INTO SENSOR_STATION VALUES(NOW(),%d, %d)",payload.data.SENSOR_STATION.stationId, payload.data.SENSOR_STATION.powerVoltage);			
			break;
		default:
			printf("Unknown message.\n");
			break;
		}

		// Executing the SQL instruction. There is one payload per loop, therefore one SQL instructions per loop.
		if ((mysql1 != NULL) &&  (strcmp(SQLstring,";") != 0)) { // Check mySql connection availability and Sql Instructions validity
			if (!mysql_query(mysql1, SQLstring)) { 	
				//printf(" >SQL Ok: %s\n",SQLstring); 
			}  
			else { 
				printf("  >SQL NOk: %s\n",SQLstring); 	// Error Occured
				printf("%s\n", mysql_error(mysql1)); 	// Print mysql Error
			}
		}
	}
	delay(20);
}


/*
************** MAIN PROGRAM ENTRY **************
*/
int main(int argc, char** argv){
	setup();
	while(1)
	loop();

	return 0;
}
