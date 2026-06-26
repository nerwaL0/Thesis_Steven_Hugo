#include "ac_control.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include "firebase_manager.h"
#include <time.h>

#include <ir_Daikin.h> //14
#include <ir_Panasonic.h> //3
#include <ir_Sharp.h> //27
#include <ir_LG.h> //6
#include <ir_Samsung.h> //11
#include <ir_Gree.h> //32

const uint16_t kIrLed = 14;

IRDaikinESP acDaikin(kIrLed);
IRPanasonicAc acPanasonic(kIrLed);
IRSharpAc acSharp(kIrLed);
IRLgAc acLG(kIrLed);
IRSamsungAc acSamsung(kIrLed);
IRGreeAC acGree(kIrLed);

void setupAC() {
    acDaikin.begin();
    acPanasonic.begin();
    acSharp.begin();
    acLG.begin();
    acSamsung.begin();
    acGree.begin();
}

void tembakSinyalAC(
    int id, bool powerStatus, int temp, int fan, bool swing, int mode, unsigned long startTime, uint64_t commandTimestamp, unsigned long networkLatencyMs) {
    if (id <= 0) return;

    Serial.printf("\n--- TRANSMIT: ID:%d, Pwr:%d, Temp:%d, Fan:%d, Swing:%d, Mode:%d ---\n", id, powerStatus, temp, fan, swing, mode);

    unsigned long t_config_start = millis();
    unsigned long t_before_send, t_after_send;

    switch (id) {
        case 14: { // DAIKIN
            if (powerStatus) {
                acDaikin.on();
                acDaikin.setTemp(temp);
                if (mode == 0) acDaikin.setMode(kDaikinAuto);
                else if (mode == 2) acDaikin.setMode(kDaikinDry);
                else if (mode == 3) acDaikin.setMode(kDaikinFan);
                else acDaikin.setMode(kDaikinCool); // Default ke Cool
                if (fan == 0) acDaikin.setFan(kDaikinFanAuto);
                else if (fan == 1) acDaikin.setFan(1); // Low
                else if (fan == 2) acDaikin.setFan(3); // Med
                else acDaikin.setFan(5);              // High
                
                acDaikin.setSwingVertical(swing);
            } else {
                acDaikin.off();
            }
            unsigned long t_before_send = millis();
            acDaikin.send();
            unsigned long t_after_send = millis();
            Serial.printf("  Config time: %lu ms\n", t_before_send - t_config_start);
            Serial.printf("  IR Send time: %lu ms\n", t_after_send - t_before_send);
            Serial.printf("  Total AC function: %lu ms\n", t_after_send - t_config_start);
            Serial.printf("  Total Firebase→IR: %lu ms\n", t_after_send - startTime);
            break;
        }
        case 3: { // PANASONIC
            if (powerStatus) {
                acPanasonic.on();
                acPanasonic.setTemp(temp);
                if (mode == 0) acPanasonic.setMode(kPanasonicAcAuto);
                else if (mode == 2) acPanasonic.setMode(kPanasonicAcDry);
                else if (mode == 3) acPanasonic.setMode(kPanasonicAcFan);
                else acPanasonic.setMode(kPanasonicAcCool); // Default ke Cool
                if (fan == 0) acPanasonic.setFan(kPanasonicAcFanAuto);
                else if (fan == 1) acPanasonic.setFan(kPanasonicAcFanMin);
                else if (fan == 2) acPanasonic.setFan(kPanasonicAcFanMed);
                else acPanasonic.setFan(kPanasonicAcFanMax);

                acPanasonic.setSwingVertical(swing ? kPanasonicAcSwingVAuto : kPanasonicAcSwingVHighest);
            } else {
                acPanasonic.off();
            }
            t_before_send = millis();
            acPanasonic.send();
            t_after_send = millis();
            Serial.printf("  Config time: %lu ms\n", t_before_send - t_config_start);
            Serial.printf("  IR Send time: %lu ms\n", t_after_send - t_before_send);
            Serial.printf("  Total AC function: %lu ms\n", t_after_send - t_config_start);
            Serial.printf("  Total Firebase→IR: %lu ms\n", t_after_send - startTime);
            break;
        }
        case 27: { // SHARP
            if (powerStatus) {
                acSharp.on();
                acSharp.setMode(kSharpAcCool); 
                acSharp.setTemp(temp);
                if (mode == 0) acSharp.setMode(kSharpAcAuto);
                else if (mode == 2) acSharp.setMode(kSharpAcDry);
                else if (mode == 3) acSharp.setMode(kSharpAcFan);
                else acSharp.setMode(kSharpAcCool); // Default ke Cool
                // Mapping Fan Speed Sharp
                if (fan == 0) acSharp.setFan(kSharpAcFanAuto);
                else if (fan == 1) acSharp.setFan(kSharpAcFanMin);
                else if (fan == 2) acSharp.setFan(kSharpAcFanMed);
                else acSharp.setFan(kSharpAcFanMax);

                acSharp.setSwingV(swing);
            } else {
                acSharp.off();
            }
            t_before_send = millis();
            acSharp.send();
            t_after_send = millis();
            Serial.printf("  Config time: %lu ms\n", t_before_send - t_config_start);
            Serial.printf("  IR Send time: %lu ms\n", t_after_send - t_before_send);
            Serial.printf("  Total AC function: %lu ms\n", t_after_send - t_config_start);
            Serial.printf("  Total Firebase→IR: %lu ms\n", t_after_send - startTime);
            break;
        }
        case 6: { // LG
            if (powerStatus) {
                acLG.on();
                acLG.setMode(kLgAcCool); 
                acLG.setTemp(temp);
                if (mode == 0) acLG.setMode(kLgAcAuto);
                else if (mode == 2) acLG.setMode(kLgAcDry);
                else if (mode == 3) acLG.setMode(kLgAcFan);
                else acLG.setMode(kLgAcCool); 
                if (fan == 0) acLG.setFan(kLgAcFanAuto);
                else if (fan == 1) acLG.setFan(kLgAcFanLowest);
                else if (fan == 2) acLG.setFan(kLgAcFanMedium);
                else acLG.setFan(kLgAcFanHigh);

                acLG.setSwingV(swing); 
            } else {
                acLG.off();
            }
            t_before_send = millis();
            acLG.send();
            t_after_send = millis();
            Serial.printf("  Config time: %lu ms\n", t_before_send - t_config_start);
            Serial.printf("  IR Send time: %lu ms\n", t_after_send - t_before_send);
            Serial.printf("  Total AC function: %lu ms\n", t_after_send - t_config_start);
            Serial.printf("  Total Firebase→IR: %lu ms\n", t_after_send - startTime);
            break;
        }
        case 11: { // SAMSUNG
            if (powerStatus) {
                acSamsung.on();
                acSamsung.setMode(kSamsungAcCool);
                acSamsung.setTemp(temp);
                if (mode == 0) acSamsung.setMode(kSamsungAcAuto);
                else if (mode == 2) acSamsung.setMode(kSamsungAcDry);
                else if (mode == 3) acSamsung.setMode(kSamsungAcFan);
                else acSamsung.setMode(kSamsungAcCool);
                if (fan == 0) acSamsung.setFan(kSamsungAcFanAuto);
                else if (fan == 1) acSamsung.setFan(kSamsungAcFanLow);
                else if (fan == 2) acSamsung.setFan(kSamsungAcFanMed);
                else acSamsung.setFan(kSamsungAcFanHigh);

                acSamsung.setSwing(swing);
            } else {
                acSamsung.off();
            }
            t_before_send = millis();
            acSamsung.send();
            t_after_send = millis();
            Serial.printf("  Config time: %lu ms\n", t_before_send - t_config_start);
            Serial.printf("  IR Send time: %lu ms\n", t_after_send - t_before_send);
            Serial.printf("  Total AC function: %lu ms\n", t_after_send - t_config_start);
            Serial.printf("  Total Firebase→IR: %lu ms\n", t_after_send - startTime);
            break;
        }
        case 32: { // GREE
            if (powerStatus) {
                acGree.on();
                acGree.setMode(kGreeCool);
                acGree.setTemp(temp);
                if (mode == 0) acGree.setMode(kGreeAuto);
                else if (mode == 2) acGree.setMode(kGreeDry);
                else if (mode == 3) acGree.setMode(kGreeFan);
                else acGree.setMode(kGreeCool);
                switch(fan) {
                    case 0: acGree.setFan(kGreeFanAuto); break;
                    case 1: acGree.setFan(kGreeFanMin); break;
                    case 2: acGree.setFan(kGreeFanMed); break;
                    case 3: acGree.setFan(kGreeFanMax); break;
                    default: acGree.setFan(kGreeFanAuto); break;
                }
                acGree.setSwingVertical(swing, kGreeSwingAuto);
            } else {
                acGree.off();
            }
            t_before_send = millis();
            acGree.send();
            t_after_send = millis();
            Serial.printf("  Config time: %lu ms\n", t_before_send - t_config_start);
            Serial.printf("  IR Send time: %lu ms\n", t_after_send - t_before_send);
            Serial.printf("  Total AC function: %lu ms\n", t_after_send - t_config_start);
            Serial.printf("  Total Firebase→IR: %lu ms\n", t_after_send - startTime);
            break;
        }
        default:
            Serial.println("Protocol ID tidak terdaftar!");
            return;
    }
    Serial.println("Sinyal Berhasil Terkirim!");
    delay(200);
    unsigned long totalProcessingTime = millis() - startTime;
    
    uint64_t estimatedFullLatency = 0;
    uint64_t estimatedPollingWait = 0;
    if (commandTimestamp > 0) {
        uint64_t executedTime = (uint64_t)time(nullptr) * 1000;
        estimatedFullLatency = executedTime - commandTimestamp;
        if (estimatedFullLatency > (totalProcessingTime + networkLatencyMs)) {
            estimatedPollingWait = estimatedFullLatency - totalProcessingTime - networkLatencyMs;
        }
    }
    
    Serial.printf("\n=== LATENCY SUMMARY ===\n");
    Serial.printf("Polling Wait (check interval): %llu ms\n", estimatedPollingWait);
    Serial.printf("Network Latency: %lu ms\n", networkLatencyMs);
    Serial.printf("ESP32 Local Processing: %lu ms\n", totalProcessingTime);
    Serial.printf("TOTAL (Firebase → IR): %llu ms\n", estimatedPollingWait + networkLatencyMs + totalProcessingTime);
    Serial.printf("Sending full latency confirmation to Firebase...\n");
    logLatencyConfirmation(totalProcessingTime, commandTimestamp, networkLatencyMs);
    Serial.printf("=======================\n\n");
}