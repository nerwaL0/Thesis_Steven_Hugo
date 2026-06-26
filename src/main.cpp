#include <Arduino.h>
#include "ac_control.h"
#include "network_manager.h"
#include "firebase_manager.h"
#include "sensor_manager.h"

void setup() {
    Serial.begin(115200);
    setupSensor();    // Inisialisasi Sensor DHT22
    setupAC();
    setupNetwork();   // Inisialisasi WiFi Manager
    setupFirebase();  // Inisialisasi Firebase & Path

    
}

void loop() {
    checkResetButton();      
    handleFirebaseUpdates(); 
    readAndUploadSensor();   
    maintainWiFiConnection(); 
}