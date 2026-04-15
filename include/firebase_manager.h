#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

#include <Arduino.h>

// Fungsi utama untuk inisialisasi dan pengecekan data
void setupFirebase();
void handleFirebaseUpdates();
void uploadSensorData(float temp, float hum);
void logLatencyConfirmation(unsigned long processingTimeMs, uint64_t commandTimestamp, unsigned long networkLatencyMs);
#endif