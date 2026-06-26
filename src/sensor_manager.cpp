#include "sensor_manager.h"
#include <DHTesp.h>
#include "firebase_manager.h"
#include <WiFi.h>

#define DHTPIN 4

DHTesp dht;
unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 5000; 

void setupSensor() {
    delay(2000); 
    dht.setup(DHTPIN, DHTesp::DHT22);
    Serial.println(F("DHTesp (DHT22) Berhasil diinisialisasi pada Pin 4."));
}

void readAndUploadSensor() {
    if (millis() - lastSensorRead >= SENSOR_INTERVAL) {
        lastSensorRead = millis();

        if (WiFi.status() != WL_CONNECTED) {
            return; 
        }

        float hum, temp;
        int retries = 3;
        while (retries--) {
            hum = dht.getHumidity();
            temp = dht.getTemperature();
            if (dht.getStatus() == DHTesp::ERROR_NONE) break;
            Serial.println("Retrying DHT read...");
            delay(2000);
        }

        if (dht.getStatus() != DHTesp::ERROR_NONE) {
            Serial.print(F("Gagal membaca dari sensor DHT! Error: "));
            Serial.println(dht.getStatusString());
            delay(2000);
            return;
        }

        Serial.print("Humidity: ");
        Serial.print(hum);
        Serial.print(" %\t");
        Serial.print("Temperature: ");
        Serial.print(temp);
        Serial.println(" *C");

        uploadSensorData(temp, hum);
    }
}