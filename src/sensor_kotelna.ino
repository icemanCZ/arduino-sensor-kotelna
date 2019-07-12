// pøipojení knihoven
#include "shared.h"
#include <SPI.h>
#include "RF24.h"
#include "max6675.h"
#include <OneWire.h>
#include <DallasTemperature.h>




// RF24
// inicializace nRF s piny CE a CS
RF24 nRF(9, 10);
// nastavení adres pro pøijímaè a vysílaè,
// musí být nastaveny stejnì v obou programech!
const byte RF_ADDRESS[] = "sensorKotelna";
const int SEND_DELAY = 10000;



// MAX6675
MAX6675 ktc(8, 7, 12); // Create a Module (CLK, CS, SO)




// OneWIre senzory
#define ONE_WIRE_BUS 2
OneWire ds(ONE_WIRE_BUS);
DallasTemperature senzoryDS(&ds);
unsigned long lastTemperatureSent = 0;  // cas od posledniho nahlaseni interni teploty
const unsigned long TEMPERATURE_INTERVAL = 60000; // interval nahlasovani interni teploty
int oneWireDeviceCount = 0;
const uint8_t SENSOR_ZPATECKA[8] = { 0x28, 0xAA, 0xBE, 0xB9, 0x4E, 0x14, 0x01, 0x2E };
const uint8_t SENSOR_VYSTUP[8] = { 0x28, 0xAA, 0x51, 0xE5, 0x49, 0x14, 0x01, 0x2B };
const uint8_t SENSOR_INTERNI[8] = { 0x28, 0xAA, 0xB3, 0x0D, 0x4B, 0x14, 0x01, 0x68 };



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

void setup()
{
	// komunikace pøes sériovou linku rychlostí 9600 baud
	Serial.begin(9600);

	Serial.println(F("Startuji"));

	// nastavim OneWire port
	Serial.println(F("  IO porty"));
	pinMode(ONE_WIRE_BUS, INPUT);

	//vyhledam OneWire cidla
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

	// zapnutí komunikace nRF modulu
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
	nRF.openWritingPipe(RF_ADDRESS);


	//Serial.println(F("Nastaveni nRF21: "));
	//fdevopen(&serial_putc, 0);
	//nRF.printDetails();
	//Serial.println(F(""));

	Serial.println(F("Nastartovano"));
}

void sendTemp(int id, float value)
{
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

void loop()
{
	unsigned long time = millis();

	if (time - lastTemperatureSent > TEMPERATURE_INTERVAL)
	{
		// interni senzor
		senzoryDS.requestTemperatures();
		float temp = senzoryDS.getTempC(SENSOR_INTERNI);
		sendTemp(SENSOR_KOTELNA_INTERNI_ID, temp);
		delay(SEND_DELAY);

		// kourovod
		temp = ktc.readCelsius();
		sendTemp(SENSOR_KOTELNA_KOUROVOD_ID, temp);
		delay(SEND_DELAY);

		// vystup
		senzoryDS.requestTemperatures();
		temp = senzoryDS.getTempC(SENSOR_VYSTUP);
		sendTemp(SENSOR_KOTELNA_VYSTUP_ID, temp);
		delay(SEND_DELAY);

		// zpatecka
		senzoryDS.requestTemperatures();
		temp = senzoryDS.getTempC(SENSOR_ZPATECKA);
		sendTemp(SENSOR_KOTELNA_ZPATECKA_ID, temp);
		delay(SEND_DELAY);

		lastTemperatureSent = time;
	}
}