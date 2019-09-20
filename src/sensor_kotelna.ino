// libraries
#include "homeiot_shared.h"
//#include <SPI.h>
#include "RF24.h"
#include "max6675.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "U8glib.h"




// RF24
RF24 nRF(9, 10);  // init pins CE, CS
const unsigned long SEND_DELAY = 20UL * 1000UL;



// MAX6675
MAX6675 ktc(8, 7, 12); // init pins CLK, CS, SO




// OneWire sensors
#define ONE_WIRE_BUS 2
OneWire ds(ONE_WIRE_BUS);
DallasTemperature senzoryDS(&ds);
unsigned long lastInternalSensorValuesSent = 0;									// time from last internal sensor notification
unsigned long lastOutputSensorValuesSent = 0;									// time from last water output sensor notification
unsigned long lastReturnSensorValuesSent = 0;									// time from last wter return sensor notification
unsigned long lastGasKettleSensorValuesSent = 0;								// time from last gas kettle sensor notification
unsigned long lastSmokeSensorValuesSent = 0;									// time from last smoke pipe sensor notification
const unsigned long INTERNAL_SENSOR_VALUES_SEND_INTERVAL = 6UL * 60UL * 1000UL; // internal sensor notification interval
const unsigned long SENSOR_VALUES_SEND_INTERVAL = 2UL * 60UL * 1000UL;			// other sensor notifications interval
int oneWireDeviceCount = 0;
const uint8_t SENSOR_ZPATECKA[8] = { 0x28, 0xAA, 0xBE, 0xB9, 0x4E, 0x14, 0x01, 0x2E };
const uint8_t SENSOR_VYSTUP[8] = { 0x28, 0xAA, 0x51, 0xE5, 0x49, 0x14, 0x01, 0x2B };
const uint8_t SENSOR_INTERNI[8] = { 0x28, 0xAA, 0xB3, 0x0D, 0x4B, 0x14, 0x01, 0x68 };
const uint8_t SENSOR_PLYN[8] = { 0x28, 0xAA, 0x53, 0x39, 0x40, 0x14, 0x01, 0x35 };


// display
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);


//typedef struct {
//	int sensorId;
//	float value;
//} payload;
//
//payload data;

void printAddress(DeviceAddress deviceAddress)
{
	for (uint8_t i = 0; i < 8; i++)
	{
		Serial.print("0x");
		if (deviceAddress[i] < 0x10) Serial.print("0");
		Serial.print(deviceAddress[i], HEX);
		if (i < 7) Serial.print(", ");
	}
	Serial.println("");
}

void clearDisplay() {
	u8g.firstPage();
	do {
	} while (u8g.nextPage());
}

void setup()
{
	// serial line communication with 9600 baud
	Serial.begin(9600);

	Serial.println(F("Startuji"));

	// set OneWire port
	Serial.println(F("  IO porty"));
	pinMode(ONE_WIRE_BUS, INPUT);

	// find OneWire sensors
	Serial.println(F("  OneWire cidla"));
	senzoryDS.begin();
	DeviceAddress Thermometer;
	oneWireDeviceCount = senzoryDS.getDeviceCount();
	for (int i = 0; i < oneWireDeviceCount; i++)
	{
		Serial.print("    Sensor ");
		Serial.print(i + 1);
		Serial.print(": ");
		senzoryDS.getAddress(Thermometer, i);
		printAddress(Thermometer);
	}

	// begin nRF24 communication
	Serial.println(F("  RF24"));
	nRF.begin();
	initRF(nRF);

	nRF.openWritingPipe(RF_KOTELNA_ADDRESS);
	//nRF.openReadingPipe(1, RF_OUTSIDE_ADDRESS);

	//nRF.startListening();

	//Serial.println(F("Nastaveni nRF21: "));
	//fdevopen(&serial_putc, 0);
	//nRF.printDetails();
	//Serial.println(F(""));

	// settle time
	delay(2000);

	Serial.println(F("Nastartovano"));
}

unsigned long d1 = SEND_DELAY;
unsigned long d2 = SEND_DELAY*2UL;
unsigned long d3 = SEND_DELAY*3UL;
unsigned long d4 = SEND_DELAY*4UL;


void loop()
{
	unsigned long time = millis();


	// read sensor values
	// internal
	senzoryDS.requestTemperatures();
	float internalTemp = senzoryDS.getTempC(SENSOR_INTERNI);

	// output
	float outputTemp = senzoryDS.getTempC(SENSOR_VYSTUP);

	// return
	float returnTemp = senzoryDS.getTempC(SENSOR_ZPATECKA);

	// gas kettle
	float gasKettleTemp = senzoryDS.getTempC(SENSOR_PLYN);

	// smoke
	float smokeTemp = ktc.readCelsius();


	// print values on display

	u8g.firstPage();
	do {
		u8g.setFont(u8g_font_7x14);
		u8g.setPrintPos(0, 14);
		u8g.print("Vystup: " + String(outputTemp));
		u8g.setPrintPos(0, 28);
		u8g.print("Zpatecka: " + String(returnTemp));
		u8g.setPrintPos(0, 42);
		u8g.print("Kourovod: " + String(smokeTemp));
		u8g.setPrintPos(0, 56);
		u8g.print("Plyn: " + String(gasKettleTemp));
	} while (u8g.nextPage());


	// check notification intervals
	if ((time - lastInternalSensorValuesSent) > INTERNAL_SENSOR_VALUES_SEND_INTERVAL)
	{
		sendTemp(nRF, RF_SENSOR_KOTELNA_INTERNAL_TEMPERATURE_ID, internalTemp, true);
		lastInternalSensorValuesSent = time;
	}
	if ((time - lastOutputSensorValuesSent) > SENSOR_VALUES_SEND_INTERVAL + d1)
	{
		sendTemp(nRF, RF_SENSOR_KOTELNA_OUTPUT_TEMPERATURE_ID, outputTemp, true);
		lastOutputSensorValuesSent = time;
		d1 = 0;
	}

	if ((time - lastReturnSensorValuesSent) > SENSOR_VALUES_SEND_INTERVAL + d2)
	{
		sendTemp(nRF, RF_SENSOR_KOTELNA_RETURN_TEMPERATURE_ID, returnTemp, true);
		lastReturnSensorValuesSent = time;
		d2 = 0;
	}

	if ((time - lastSmokeSensorValuesSent) > SENSOR_VALUES_SEND_INTERVAL + d3)
	{
		sendTemp(nRF, RF_SENSOR_KOTELNA_SMOKE_TEMPERATURE_ID, smokeTemp, true);
		lastSmokeSensorValuesSent = time;
		d3 = 0;
	}

	if ((time - lastGasKettleSensorValuesSent) > SENSOR_VALUES_SEND_INTERVAL + d4)
	{
		sendTemp(nRF, RF_SENSOR_KOTELNA_SMOKE_TEMPERATURE_ID, gasKettleTemp, true);
		lastGasKettleSensorValuesSent = time;
		d4 = 0;
	}


	// proxy function

	//if (nRF.available())
	//{
	//	// wait for data
	//	while (nRF.available())
	//	{
	//		Serial.println(F("Preposilam nova data ze vzdaleneho senzoru"));


	//		long sensorId;
	//		float sensorValue;

	//		if (!readData(nRF, &sensorId, &sensorValue))
	//			break;

	//		sendTemp(nRF, sensorId, sensorValue);
	//	}
	//	Serial.println();
	//}

	delay(1000);
}