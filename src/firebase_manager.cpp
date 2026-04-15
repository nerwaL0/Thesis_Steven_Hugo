#include "firebase_manager.h"
#include <FirebaseESP32.h>
#include "config.h"
#include "ac_control.h"
#include "network_manager.h"
#include <time.h>

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String userPath;
unsigned long lastFirebaseCheck = 0;

// Variabel status terakhir (Encapsulated)
int lastProtocol = -1, lastTemp = -1, lastFan = -1, lastMode = -1;
bool lastPower = false, lastSwing = false, isFirstRun = true;

String createPath(String base, String child) {
  String result = base;
  result += child;
  return result;
}

void setupFirebase() {
    userPath = "/devices/";
    userPath += getDeviceID();
    userPath += "/settings";
    
    Serial.print("Menghubungkan ke Firebase Path: ");
    Serial.println(userPath);

    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    config.signer.test_mode = true;
    
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    
    // Sync ESP32 clock with network time with firebase token generation requires a valid timestamp
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("Syncing with NTP server...");
    time_t now = time(nullptr);
    while (now < 24 * 3600 * 2) {
        delay(500);
        now = time(nullptr);
    }
    Serial.println("Clock synced with NTP");
}

void handleFirebaseUpdates() {
    // cek tiap 500ms dari firebase dan 5000ms sejak boot untuk menghindari pembacaan data yang tidak stabil saat baru mulai
    if (millis() < 5000) return; 
    if (millis() - lastFirebaseCheck < 500) return;
    lastFirebaseCheck = millis();   

    if (Firebase.ready()) {
        // ambil seluruh objek settings sekaligus
        unsigned long t_before_getjson = millis();  // Measure network latency
        if (Firebase.getJSON(fbdo, userPath.c_str())) {
            unsigned long t_after_getjson = millis();  // After Firebase response
            unsigned long network_latency = t_after_getjson - t_before_getjson;
            
            unsigned long t_start = millis();  // Step 1: Start timing
            
            FirebaseJson &json = fbdo.jsonObject();
            FirebaseJsonData result;

            // Get Firebase server timestamp (use uint64_t for full precision)
            json.get(result, "command_timestamp");
            uint64_t firebaseTimestamp = (uint64_t)result.doubleValue;
            
            // DEBUG: Show what we're reading
            Serial.printf("DEBUG: command_timestamp value from Firebase: %llu\n", firebaseTimestamp);
            
            unsigned long t_after_timestamp = millis();  // Step 2: After getting timestamp

            // Parsing data
            json.get(result, "protocol_id"); int cProtocol = result.intValue;
            json.get(result, "power"); bool cPower = result.boolValue;
            json.get(result, "temp"); int cTemp = result.intValue;
            json.get(result, "fan_speed"); int cFan = result.intValue;
            json.get(result, "swing"); bool cSwing = result.boolValue;
            json.get(result, "mode"); int cMode = result.intValue;

            unsigned long t_after_parsing = millis();  // Step 3: After parsing all values
            
            // logika pengecekan data
            if (isFirstRun || cProtocol != lastProtocol || cPower != lastPower || 
                cTemp != lastTemp || cFan != lastFan || cSwing != lastSwing || cMode != lastMode) {
                
                // Pass BOTH local time and Firebase command timestamp, plus network latency
                tembakSinyalAC(cProtocol, cPower, cTemp, cFan, cSwing, cMode, t_start, firebaseTimestamp, network_latency);
                
                unsigned long t_after_transmit = millis();  // Step 4: After IR transmission
                
                // Print timing breakdown
                Serial.printf("\n=== TIMING BREAKDOWN ===\n");
                Serial.printf("Network Latency (Firebase GET): %lu ms\n", network_latency);
                Serial.printf("Get Timestamp: %lu ms\n", t_after_timestamp - t_start);
                Serial.printf("Parse All Data: %lu ms\n", t_after_parsing - t_after_timestamp);
                Serial.printf("Firebase->Parse Total: %lu ms\n", t_after_parsing - t_start);
                Serial.printf("Config+Transmit in AC func: %lu ms\n", t_after_transmit - t_after_parsing);
                Serial.printf("ESP32 Total (after network): %lu ms\n", t_after_transmit - t_start);
                Serial.printf("Command Timestamp from Firebase: %lu\n", firebaseTimestamp);
                Serial.printf("======================\n\n");
                
                lastProtocol = cProtocol; 
                lastPower = cPower; 
                lastTemp = cTemp;
                lastFan = cFan; 
                lastSwing = cSwing;
                lastMode = cMode;
                isFirstRun = false;
            }
        }
    }
}
void uploadSensorData(float temp, float hum) {
    if (Firebase.ready()) {
        if (Firebase.setFloat(fbdo, createPath(userPath, "/read_temp").c_str(), temp)) {
            Serial.println(F("Data Suhu berhasil diunggah."));
     }
        if (Firebase.setFloat(fbdo, createPath(userPath, "/read_hum").c_str(), hum)) {
            Serial.println(F("Data Kelembapan berhasil diunggah."));
        }
    }
}

void logLatencyConfirmation(unsigned long processingTimeMs, uint64_t commandTimestamp, unsigned long networkLatencyMs) {
    if (Firebase.ready()) {
        // SAFETY CHECK: Only log if we have a valid command timestamp
        // If commandTimestamp is 0/missing, skip writing to avoid corrupting data
        if (commandTimestamp == 0) {
            Serial.println("⚠️  WARNING: command_timestamp is 0 - skipping latency log to prevent data corruption");
            Serial.println("   Make sure you include command_timestamp in your Firebase settings update");
            return;
        }
        
        // Get current server time (epoch in milliseconds)
        uint64_t executedTimestamp = (uint64_t)time(nullptr) * 1000;
        
        // Calculate FULL latency: from command sent to IR transmitted
        uint64_t fullLatencyMs = 0;
        uint64_t pollingWaitMs = 0;
        if (commandTimestamp > 0) {
            fullLatencyMs = executedTimestamp - commandTimestamp;
            // Polling wait = total latency - network - processing
            // (everything else is waiting for next check interval)
            if (fullLatencyMs > (networkLatencyMs + processingTimeMs)) {
                pollingWaitMs = fullLatencyMs - networkLatencyMs - processingTimeMs;
            }
        }
        
        // Create confirmation data
        FirebaseJson confirmData;
        confirmData.set("command_timestamp", (double)commandTimestamp);
        confirmData.set("executed_timestamp", (double)executedTimestamp);
        confirmData.set("polling_wait_ms", (double)pollingWaitMs);
        confirmData.set("network_latency_ms", networkLatencyMs);
        confirmData.set("processing_time_ms", processingTimeMs);
        confirmData.set("full_latency_ms", (double)fullLatencyMs);
        confirmData.set("confirmed_at", (unsigned long)time(nullptr));
        
        // Write to Firebase
        String confirmPath = userPath;
        confirmPath += "/last_execution";
        
        if (Firebase.setJSON(fbdo, confirmPath.c_str(), confirmData)) {
            Serial.printf("✓ Latency confirmation sent to Firebase\n");
            Serial.printf("  Command Timestamp: %llu ms (from Firebase)\n", commandTimestamp);
            Serial.printf("  Executed Timestamp: %llu ms\n", executedTimestamp);
            Serial.printf("  Polling Wait: %llu ms (ESP32 check interval delay)\n", pollingWaitMs);
            Serial.printf("  Network Latency: %lu ms\n", networkLatencyMs);
            Serial.printf("  ESP32 Processing: %lu ms\n", processingTimeMs);
            Serial.printf("  FULL SYSTEM LATENCY: %llu ms (Command → IR)\n", fullLatencyMs);
        } else {
            Serial.printf("✗ Failed to log confirmation: %s\n", fbdo.errorReason().c_str());
        }
    }
}