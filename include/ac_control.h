#ifndef AC_CONTROL_H
#define AC_CONTROL_H
#include <Arduino.h>

void setupAC();
void tembakSinyalAC(int id, bool powerStatus, int temp, int fan, bool swing, int mode, unsigned long startTime, uint64_t commandTimestamp, unsigned long networkLatencyMs);

#endif