// p�ipojen� knihoven
#include <SPI.h>
#include "RF24.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// nastaven� propojovac�ch pin�
#define CE 7
#define CS 8
// inicializace nRF s piny CE a CS
RF24 nRF(CE, CS);
// nastaven� adres pro p�ij�ma� a vys�la�,
// mus� b�t nastaveny stejn� v obou programech!
byte adresaPrijimac[] = "sensorServer";


// Interni senzor teploty
#define ONE_WIRE_BUS 2
OneWire ds(ONE_WIRE_BUS);
DallasTemperature senzoryDS(&ds);

unsigned long lastTemperatureSent = 0;  // cas od posledniho nahlaseni interni teploty
const unsigned long TEMPERATURE_INTERVAL = 60000; // interval nahlasovani interni teploty

void setup() 
{
	// komunikace p�es s�riovou linku rychlost� 9600 baud
	Serial.begin(9600);

	Serial.println(F("Startuji"));

	// nastavim port cidla
	Serial.println(F("  IO porty"));
	pinMode(ONE_WIRE_BUS, INPUT);

	// zapnut� komunikace nRF modulu
	Serial.println(F("  RF24"));
	nRF.begin();
	// nastaven� v�konu nRF modulu,
	// mo�nosti jsou RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH and RF24_PA_MAX,
	// pro HIGH a MAX je nutn� extern� 3,3V zdroj
	nRF.setPALevel(RF24_PA_LOW);
	// nastaven� zapisovac�ho a �tec�ho kan�lu 
	nRF.openWritingPipe(adresaPrijimac);

	Serial.println(F("Nastartovano"));
}

void sendTemp(int id, float value) 
{
	Serial.println("Odesilam teplotu cidla " + String(id) + ": " + String(value));

	if (!nRF.write(&id, sizeof(id)))
	{
		Serial.println(F("Chyba p�i odesl�n�!"));
	}	
	if (!nRF.write(&value, sizeof(value)))
	{
		Serial.println(F("Chyba p�i odesl�n�!"));
	}
	Serial.println(F("Odeslano"));
}

void loop()
{
	unsigned long time = millis();

	if (time - lastTemperatureSent > TEMPERATURE_INTERVAL)
	{
		// interni senzor
		senzoryDS.requestTemperatures();
		float temp = senzoryDS.getTempCByIndex(0);

		sendTemp(1, temp);
		lastTemperatureSent = time;
	}
}