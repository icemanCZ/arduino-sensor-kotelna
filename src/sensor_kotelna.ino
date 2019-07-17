// libraries
#include "shared.h"
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
unsigned long lastSmokeSensorValuesSent = 0;									// time from last smoke pipe sensor notification
const unsigned long INTERNAL_SENSOR_VALUES_SEND_INTERVAL = 6UL * 60UL * 1000UL; // internal sensor notification interval
const unsigned long SENSOR_VALUES_SEND_INTERVAL = 2UL * 60UL * 1000UL;			// other sensor notifications interval
int oneWireDeviceCount = 0;
const uint8_t SENSOR_ZPATECKA[8] = { 0x28, 0xAA, 0xBE, 0xB9, 0x4E, 0x14, 0x01, 0x2E };
const uint8_t SENSOR_VYSTUP[8] = { 0x28, 0xAA, 0x51, 0xE5, 0x49, 0x14, 0x01, 0x2B };
const uint8_t SENSOR_INTERNI[8] = { 0x28, 0xAA, 0xB3, 0x0D, 0x4B, 0x14, 0x01, 0x68 };


// display
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);



//typedef struct {
//	int sensorId;
//	float value;
//} payload;
//
//payload data;

int serial_putc(char c, FILE*)
{
	Serial.write(c);
	return c;
}

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

void sendTemp(int id, float value)
{
	clearDisplay();

	Serial.println("Odesilam teplotu cidla " + String(id) + ": " + String(value));

	if (!nRF.write(&id, sizeof(id)))
	{
		Serial.println(F("Chyba pri odesilani 1!"));
	}
	delay(10);
	if (!nRF.write(&value, sizeof(value)))
	{
		Serial.println(F("Chyba pri odesilani 2!"));
	}
	//data.sensorId = id;
	//data.value = value;
	//if (!nRF.write(&data, sizeof(data)))
	//{
	//	Serial.println(F("Chyba pri odesilani!"));
	//}
	Serial.println(F("Odeslano"));
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
	nRF.stopListening();
	// možnosti jsou RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH and RF24_PA_MAX,
	// pro HIGH a MAX je nutný externí 3,3V zdroj
	nRF.setPALevel(RF24_PA_HIGH);
	//nRF.disableDynamicPayloads();
	//nRF.setDataRate(RF24_250KBPS);
	//nRF.setAutoAck(false);
	//nRF.setChannel(50);
	nRF.openWritingPipe(RF_KOTELNA_ADDRESS);


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


void loop()
{
	unsigned long time = millis();


	// read sensor values
	// internal
	senzoryDS.requestTemperatures();
	float internalTemp = senzoryDS.getTempC(SENSOR_INTERNI);

	// smoke
	float smokeTemp = ktc.readCelsius();

	// output
	senzoryDS.requestTemperatures();
	float outputTemp = senzoryDS.getTempC(SENSOR_VYSTUP);

	// return
	senzoryDS.requestTemperatures();
	float returnTemp = senzoryDS.getTempC(SENSOR_ZPATECKA);


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
		u8g.print("Vnitrni: " + String(internalTemp));
	} while (u8g.nextPage());


	// check notification intervals
	if ((time - lastInternalSensorValuesSent) > INTERNAL_SENSOR_VALUES_SEND_INTERVAL)
	{
		sendTemp(RF_SENSOR_KOTELNA_INTERNAL_TEMPERATURE_ID, internalTemp);
		lastInternalSensorValuesSent = time;
	}
	if ((time - lastOutputSensorValuesSent) > SENSOR_VALUES_SEND_INTERVAL + d1)
	{
		sendTemp(RF_SENSOR_KOTELNA_OUTPUT_TEMPERATURE_ID, outputTemp);
		lastOutputSensorValuesSent = time;
		d1 = 0;
	}

	if ((time - lastReturnSensorValuesSent) > SENSOR_VALUES_SEND_INTERVAL + d2)
	{
		sendTemp(RF_SENSOR_KOTELNA_RETURN_TEMPERATURE_ID, returnTemp);
		lastReturnSensorValuesSent = time;
		d2 = 0;
	}

	if ((time - lastSmokeSensorValuesSent) > SENSOR_VALUES_SEND_INTERVAL + d3)
	{
		sendTemp(RF_SENSOR_KOTELNA_SMOKE_TEMPERATURE_ID, smokeTemp);
		lastSmokeSensorValuesSent = time;
		d3 = 0;
	}


	delay(1000);
}