// p�ipojen� knihoven
#include <SPI.h>
#include "RF24.h"
// nastaven� propojovac�ch pin�
#define CE 7
#define CS 8
// inicializace nRF s piny CE a CS
RF24 nRF(CE, CS);
// nastaven� adres pro p�ij�ma� a vys�la�,
// mus� b�t nastaveny stejn� v obou programech!
byte adresaPrijimac[] = "sensorServer";

void setup() {
	// komunikace p�es s�riovou linku rychlost� 9600 baud
	Serial.begin(9600);
	// zapnut� komunikace nRF modulu
	nRF.begin();
	// nastaven� v�konu nRF modulu,
	// mo�nosti jsou RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH and RF24_PA_MAX,
	// pro HIGH a MAX je nutn� extern� 3,3V zdroj
	nRF.setPALevel(RF24_PA_LOW);
	// nastaven� zapisovac�ho a �tec�ho kan�lu
	nRF.openWritingPipe(adresaPrijimac);
}

int data = 0;

void loop()
{
	if (!nRF.write(&data, sizeof(data))) {
		Serial.println("Chyba p�i odesl�n�!");
	}

	data++;

	delay(1000);
}