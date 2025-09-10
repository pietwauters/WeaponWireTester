// Copyright (c) Piet Wauters 2022 <piet.wauters@gmail.com>
#pragma once

#include <Arduino.h>

#include "Hardware.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

// Global variables (extern declarations)
extern int measurements[3][3];
extern const uint8_t driverpins[7];
extern const int Reference_3_Ohm[3];
extern const int Reference_5_Ohm[3];
extern const int Reference_10_Ohm[3];

// Function prototypes
void Set_IODirectionAndValue(uint8_t setting, uint8_t values);
int getSample(adc1_channel_t pin);
void testWiresOnByOne();
bool WirePluggedIn(int threashold = 160);
bool WirePluggedInFoil(int threashold = 160);
bool WirePluggedInEpee(int threashold = 160);
bool testStraightOnly(int threashold = 160);
int testArBr();
int testArCr();
int testArCl();
int testBrCr();
int testBrCl();
bool IsBroken(int Nr, int threashold = 160);
bool IsSwappedWith(int i, int j, int threashold = 160);
void init_AD();
