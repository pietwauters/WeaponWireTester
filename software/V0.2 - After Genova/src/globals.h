#pragma once

#include <Arduino.h>

// External variables that need to be shared
extern bool DoCalibration;
extern WS2812B_LedMatrix* LedPanel;
extern int myRefs_Ohm[];  // Correct type: int array
extern int StoredRefs_ohm[];
extern int measurements[3][3];

// Function declarations
extern void Calibrate();
extern void testWiresOnByOne();
extern int testArCr();
extern int testArBr();
extern int testBrCr();
extern bool testStraightOnly(int threshold);
