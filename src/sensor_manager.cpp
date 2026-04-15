#include "sensor_manager.h"
#include <DHTesp.h>
#include "firebase_manager.h"

#define DHTPIN 4

DHTesp dht;
unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 5000; // tiap 2 menit (testing)

void setupSensor() {
    delay(2000); // Delay untuk memastikan sensor siap setelah boot
    // Setup DHT22 pada Pin 4
    dht.setup(DHTPIN, DHTesp::DHT22);
    Serial.println(F("DHTesp (DHT22) Berhasil diinisialisasi pada Pin 4."));
}

void readAndUploadSensor() {
    if (millis() - lastSensorRead >= SENSOR_INTERVAL) {
        lastSensorRead = millis();
    float hum, temp;
    int retries = 3;
    while (retries--) {
        hum = dht.getHumidity();
        temp = dht.getTemperature();
        if (dht.getStatus() == DHTesp::ERROR_NONE) break;
        Serial.println("Retrying DHT read...");
        delay(2000); // must wait full 2s between reads
    }

        if (dht.getStatus() != DHTesp::ERROR_NONE) {
            Serial.print(F("Gagal membaca dari sensor DHT! Error: "));
            Serial.println(dht.getStatusString());
            return;
        }

            Serial.print("Humidity: ");
            Serial.print(hum);
            Serial.print(" %\t");
            Serial.print("Temperature: ");
            Serial.print(temp);
            Serial.println(" *C");

        // Upload ke Firebase parameter read_temp dan read_hum
        uploadSensorData(temp, hum);
    }
}