#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

#include <Arduino.h>

void streamTimeoutCallback(bool timeout);
void setupFirebase();
void handleFirebaseUpdates();
void uploadSensorData(float temp, float hum);
void logLatencyConfirmation(unsigned long processingTimeMs, uint64_t commandTimestamp, unsigned long networkLatencyMs);
#endif