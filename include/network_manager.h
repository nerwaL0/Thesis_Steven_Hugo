#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>

void setupNetwork();

void checkResetButton();

void maintainWiFiConnection();

String getDeviceID();

#endif