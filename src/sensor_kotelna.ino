// pøipojení knihoven
#include <SPI.h>
#include "RF24.h"
// nastavení propojovacích pinù
#define CE 7
#define CS 8
// inicializace nRF s piny CE a CS
RF24 nRF(CE, CS);
// nastavení adres pro pøijímaè a vysílaè,
// musí být nastaveny stejnì v obou programech!
byte adresaPrijimac[] = "sensorServer";

void setup() {
	// komunikace pøes sériovou linku rychlostí 9600 baud
	Serial.begin(9600);
	// zapnutí komunikace nRF modulu
	nRF.begin();
	// nastavení výkonu nRF modulu,
	// možnosti jsou RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH and RF24_PA_MAX,
	// pro HIGH a MAX je nutný externí 3,3V zdroj
	nRF.setPALevel(RF24_PA_LOW);
	// nastavení zapisovacího a ètecího kanálu
	nRF.openWritingPipe(adresaPrijimac);
}

int data = 0;

void loop()
{
	if (!nRF.write(&data, sizeof(data))) {
		Serial.println("Chyba pøi odeslání!");
	}

	data++;

	delay(1000);
}