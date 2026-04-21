#include "firebase_manager.h"
#include <FirebaseESP32.h>
#include "config.h"
#include "ac_control.h"
#include "network_manager.h"
#include <time.h>

FirebaseData fbdo;
FirebaseData streamData; // Objek khusus agar jalur Stream tidak bertabrakan dengan upload sensor
FirebaseAuth auth;
FirebaseConfig config;

String userPath;

// Variabel status terakhir
int lastProtocol = -1, lastTemp = -1, lastFan = -1, lastMode = -1;
bool lastPower = false, lastSwing = false, isFirstRun = true;

// Struct untuk menampung perintah secara instan dari Stream
struct ACCommand {
    bool ready = false;
    uint64_t timestamp = 0;
    int protocol = 0;
    bool power = false;
    int temp = 0;
    int fan = 0;
    bool swing = false;
    int mode = 0;
    unsigned long received_time = 0;
} currentCmd;

String createPath(String base, String child) {
  String result = base;
  result += child;
  return result;
}

void streamCallback(StreamData data) {
    currentCmd.received_time = millis(); // Catat waktu kedatangan paket
    
    // Pastikan data yang masuk adalah objek JSON utuh dari aplikasi
    if (data.dataType() == "json") {
        FirebaseJson json;
        json.setJsonData(data.jsonString());
        FirebaseJsonData result;

        json.get(result, "command_timestamp"); currentCmd.timestamp = (uint64_t)result.doubleValue;
        json.get(result, "protocol_id"); currentCmd.protocol = result.intValue;
        json.get(result, "power"); currentCmd.power = result.boolValue;
        json.get(result, "temp"); currentCmd.temp = result.intValue;
        json.get(result, "fan_speed"); currentCmd.fan = result.intValue;
        json.get(result, "swing"); currentCmd.swing = result.boolValue;
        json.get(result, "mode"); currentCmd.mode = result.intValue;

        currentCmd.ready = true; // Beri sinyal ke main loop untuk mengeksekusi IR
    } else {
        // Mekanisme aman: Jika struktur JSON parsial, beri sinyal untuk di-fetch ulang
        currentCmd.timestamp = 0; 
        currentCmd.ready = true;
    }
}

void streamTimeoutCallback(bool timeout) {
    if (timeout) {
        Serial.println("Koneksi Stream Timeout, menyambungkan kembali...");
    }
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
    
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("Syncing with NTP server...");
    time_t now = time(nullptr);
    while (now < 24 * 3600 * 2) {
        delay(500);
        now = time(nullptr);
    }
    Serial.println("Clock synced with NTP");

    if (Firebase.beginStream(streamData, userPath.c_str())) {
        Serial.println("Firebase Stream Aktif! Sistem siap merespons instan.");
        Firebase.setStreamCallback(streamData, streamCallback, streamTimeoutCallback);
    } else {
        Serial.print("Gagal memulai Stream: ");
        Serial.println(streamData.errorReason());
    }
}

void handleFirebaseUpdates() {
    // Fungsi ini dipanggil ribuan kali per detik oleh loop(), tetapi hanya berjalan jika ada flag 'ready'
    if (currentCmd.ready) {
        currentCmd.ready = false; 
        
        if (currentCmd.timestamp == 0) {
            // Fallback: Jika data parsial, gunakan metode tarik data (GET) yang akurat
            unsigned long t_before = millis();
            
            if (Firebase.getJSON(fbdo, userPath.c_str())) {
                unsigned long t_after = millis();
                unsigned long network_latency = t_after - t_before;
                
                FirebaseJson &json = fbdo.jsonObject();
                FirebaseJsonData result;

                json.get(result, "command_timestamp"); uint64_t firebaseTimestamp = (uint64_t)result.doubleValue;
                json.get(result, "protocol_id"); int cProtocol = result.intValue;
                json.get(result, "power"); bool cPower = result.boolValue;
                json.get(result, "temp"); int cTemp = result.intValue;
                json.get(result, "fan_speed"); int cFan = result.intValue;
                json.get(result, "swing"); bool cSwing = result.boolValue;
                json.get(result, "mode"); int cMode = result.intValue;

                if (isFirstRun || cProtocol != lastProtocol || cPower != lastPower || 
                    cTemp != lastTemp || cFan != lastFan || cSwing != lastSwing || cMode != lastMode) {
                    
                    tembakSinyalAC(cProtocol, cPower, cTemp, cFan, cSwing, cMode, t_before, firebaseTimestamp, network_latency);
                    
                    lastProtocol = cProtocol; lastPower = cPower; lastTemp = cTemp;
                    lastFan = cFan; lastSwing = cSwing; lastMode = cMode;
                    isFirstRun = false;
                }
            }
        } else {
            // Jalur Utama (Kecepatan Maksimal): Data sudah didapat dari Callback
            if (isFirstRun || currentCmd.protocol != lastProtocol || currentCmd.power != lastPower || 
                currentCmd.temp != lastTemp || currentCmd.fan != lastFan || currentCmd.swing != lastSwing || currentCmd.mode != lastMode) {
                
                unsigned long processing_latency = millis() - currentCmd.received_time;
                
                tembakSinyalAC(currentCmd.protocol, currentCmd.power, currentCmd.temp, currentCmd.fan, currentCmd.swing, currentCmd.mode, currentCmd.received_time, currentCmd.timestamp, processing_latency);
                
                lastProtocol = currentCmd.protocol; lastPower = currentCmd.power; lastTemp = currentCmd.temp;
                lastFan = currentCmd.fan; lastSwing = currentCmd.swing; lastMode = currentCmd.mode;
                isFirstRun = false;
            }
        }
    }
}

void uploadSensorData(float temp, float hum) {
    // BUAT PATH BARU KHUSUS SENSOR (Pisahkan dari userPath/settings)
    String sensorPath = "/devices/" + getDeviceID() + "/sensors"; 

    if (Firebase.ready()) {
        if (Firebase.setFloat(fbdo, sensorPath + "/read_temp", temp)) {
            Serial.println(F("Data Suhu berhasil diunggah."));
        }
        if (Firebase.setFloat(fbdo, sensorPath + "/read_hum", hum)) {
            Serial.println(F("Data Kelembapan berhasil diunggah."));
        }
    }
}

void logLatencyConfirmation(unsigned long processingTimeMs, uint64_t commandTimestamp, unsigned long networkLatencyMs) {
    if (Firebase.ready()) {
        if (commandTimestamp == 0) {
            Serial.println("⚠️ WARNING: command_timestamp is 0 - skipping latency log to prevent data corruption");
            return;
        }
        
        uint64_t executedTimestamp = (uint64_t)time(nullptr) * 1000;
        
        uint64_t fullLatencyMs = 0;
        uint64_t pollingWaitMs = 0; 
        
        if (commandTimestamp > 0) {
            fullLatencyMs = executedTimestamp - commandTimestamp;
            if (fullLatencyMs > (networkLatencyMs + processingTimeMs)) {
                pollingWaitMs = fullLatencyMs - networkLatencyMs - processingTimeMs; 
            }
        }
        
        FirebaseJson confirmData;
        confirmData.set("command_timestamp", (double)commandTimestamp);
        confirmData.set("executed_timestamp", (double)executedTimestamp);
        confirmData.set("polling_wait_ms", (double)pollingWaitMs);
        confirmData.set("network_latency_ms", networkLatencyMs);
        confirmData.set("processing_time_ms", processingTimeMs);
        confirmData.set("full_latency_ms", (double)fullLatencyMs);
        confirmData.set("confirmed_at", (unsigned long)time(nullptr));
        
        String confirmPath = userPath;
        confirmPath += "/last_execution";
        
        if (Firebase.setJSON(fbdo, confirmPath.c_str(), confirmData)) {
            Serial.printf("✓ Log Latency Stream berhasil diunggah\n");
            Serial.printf("  Propagasi Jaringan (Aplikasi -> ESP32): %llu ms\n", pollingWaitMs);
            Serial.printf("  Waktu Eksekusi Lokal ESP32: %lu ms\n", processingTimeMs);
            Serial.printf("  TOTAL LATENSI END-TO-END: %llu ms\n", fullLatencyMs);
        } else {
            Serial.printf("✗ Gagal upload latency: %s\n", fbdo.errorReason().c_str());
        }
    }
}