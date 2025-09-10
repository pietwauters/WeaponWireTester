#include <Arduino.h>
#include <cstdio>
#include <cinttypes>
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_timer.h"
#include <iostream>
#include <Preferences.h>
#include "WS2812BLedMatrix.h"
#include <vector>
#include <algorithm>
using namespace std; 



// If you have different pins, change below defines

// Below table uses AD channels and not pin numbers
#define cl_analog ADC1_CHANNEL_0_GPIO_NUM
#define bl_analog ADC1_CHANNEL_3_GPIO_NUM
#define piste_analog ADC1_CHANNEL_6_GPIO_NUM
#define cr_analog ADC1_CHANNEL_7_GPIO_NUM
#define br_analog ADC1_CHANNEL_4_GPIO_NUM


#define al_driver 33
#define bl_driver 21
#define cl_driver 23
#define ar_driver 25
#define br_driver 05
#define cr_driver 18
#define piste_driver 19


// below defines are generated with the excel tool

#define IODirection_ar_br 231
#define IODirection_ar_cr 215
#define IODirection_ar_piste 183
#define IODirection_ar_bl 245
#define IODirection_ar_cl 243
#define IODirection_al_br 238
#define IODirection_al_cr 222
#define IODirection_al_piste 190
#define IODirection_al_bl 252
#define IODirection_al_cl 250
#define IODirection_br_cr 207
#define IODirection_br_bl 237
#define IODirection_br_cl 235
#define IODirection_br_piste 175
#define IODirection_bl_cl 249
#define IODirection_bl_piste 189
#define IODirection_bl_cr 221
#define IODirection_cr_piste 159
#define IODirection_cr_cl 219
#define IODirection_cl_piste 187
#define IODirection_cr_bl 221


#define IOValues_ar_br 8
#define IOValues_ar_cr 8
#define IOValues_ar_piste 8
#define IOValues_ar_bl 8
#define IOValues_ar_cl 8
#define IOValues_al_br 1
#define IOValues_al_cr 1
#define IOValues_al_piste 1
#define IOValues_al_bl 1
#define IOValues_al_cl 1
#define IOValues_br_cr 16
#define IOValues_br_bl 16
#define IOValues_br_cl 16 
#define IOValues_br_piste 16
#define IOValues_bl_cl 2
#define IOValues_bl_piste 2
#define IOValues_bl_cr 2
#define IOValues_cr_piste 32
#define IOValues_cr_cl 32
#define IOValues_cl_piste 4

#define IOValues_cr_bl 32

typedef   enum State_t {Waiting, WireTesting_1,WireTesting_2, FoilTesting, EpeeTesting};
const uint8_t driverpins[] = {al_driver, bl_driver, cl_driver, ar_driver, br_driver, cr_driver, piste_driver};

#define FOIL_TEST_TIMEOUT 12
#define WIRE_TEST_1_TIMEOUT 2
#define NO_WIRES_PLUGGED_IN_TIMEOUT 3

int measurements[3][3];
#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  4        /* Time ESP32 will go to sleep (in seconds) */


long TimeToDeepSleep=-1;

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}


void Set_IODirectionAndValue(uint8_t setting, uint8_t values)
{
  uint8_t mask = 1;
  for (int i = 0; i < 7; i++)
  {
    if (setting & mask)
    {
      pinMode(driverpins[i], INPUT);
    }
    else
    {
      pinMode(driverpins[i], OUTPUT);
      if (values & mask)
      {
        digitalWrite(driverpins[i], HIGH);
      }
      else
      {
        digitalWrite(driverpins[i], LOW);
      }

    }
    mask <<= 1;
  }
}

int getSample(int pin){
  int max = 0;
  int sample;
  for(int i = 0; i< 7; i++){
      sample = analogReadMilliVolts(pin);
      if(sample > max)
        max =sample;
  }
  return max;
}

#define NR_SAMPLES_DIVIDER 7
#define NR_SAMPLES  1<<NR_SAMPLES_DIVIDER
void Calibrate()
{

long value = 0;
//long values[NR_SAMPLES];
long sample;
vector<long> SortedSamples[3];

long max[] ={0,0,0};
long min[] = {9999,9999,9999};
long current_time;
long duration;

while(1){
value = 0;


SortedSamples[0].clear();
SortedSamples[1].clear();
SortedSamples[2].clear();
current_time = millis();
for(int i = 0; i< NR_SAMPLES; i++){
  testStraightOnly();
  for(int j=0;j<3;j++){
    auto it = std::lower_bound(SortedSamples[j].begin(), SortedSamples[j].end(), measurements[j][j]);
    SortedSamples[j].insert(it, measurements[j][j]);
  }

      
  }
   duration = (millis()-current_time);
      
 
      Serial.print("Duration = ");
      Serial.println(duration);
      
    for(int i = 0; i<3;i++){

      if(min[i] > SortedSamples[i][0])
        min[i] = (SortedSamples[i])[0];
      Serial.print("   Min");Serial.print(i);Serial.print(" = ");
      Serial.print(min[i]);
      if(max[i] < SortedSamples[i][NR_SAMPLES-1] )
        max[i] = SortedSamples[i][NR_SAMPLES-1] ;
      
      Serial.print("   Max");Serial.print(i);Serial.print(" = ");
      Serial.print(max[i]);


      Serial.print("   Median");Serial.print(i);Serial.print(" = ");
      Serial.println((SortedSamples[i][NR_SAMPLES/2] + SortedSamples[i][NR_SAMPLES/2-1])>>1);
    }
}
      //av = 0;

    /*while(1){
      for (int i = 0; i < 512; i++){
        Set_IODirectionAndValue(IODirection_cr_cl,IOValues_cr_cl);
        delay(1);
        value = getSample(cr_analog);
        if(min > value)
          min = value;
        if(max < value)
          max = value;
        av +=value;
        
      }
      av>>=9;
      Serial.print("Av = ");
      Serial.print(av);
      Serial.print("   Min = ");
      Serial.print(min);
      Serial.print("   Max = ");
      Serial.println(max);
      av = 0;
    }*/


}

const int Reference_3_Ohm[]={1721,1698,1727};
const int Reference_5_Ohm[]={1700,1676,1706};
const int Reference_10_Ohm[]={1647,1624,1654};


WS2812B_LedMatrix *LedPanel;
#define MY_ATTENUATION  ADC_ATTEN_DB_11
//#define MY_ATTENUATION  ADC_ATTEN_DB_6

void setup() {
  // put your setup code here, to run once:
  LedPanel = new WS2812B_LedMatrix();
  LedPanel->begin();
  LedPanel->ClearAll();
  Serial.begin(115200);
  adc_power_on();
    if(ESP_OK != adc_set_clk_div(2))
      cout << "I did not expect this!" << endl;
    if(ESP_OK != adc1_config_width(ADC_WIDTH_BIT_12))
      cout << "I did not expect this!" << endl;
    if(ESP_OK != adc1_config_channel_atten(ADC1_CHANNEL_0,MY_ATTENUATION))
      cout << "I did not expect this!" << endl;
    adc1_config_channel_atten(ADC1_CHANNEL_3,MY_ATTENUATION);
    adc1_config_channel_atten(ADC1_CHANNEL_4,MY_ATTENUATION);
    adc1_config_channel_atten(ADC1_CHANNEL_6,MY_ATTENUATION);
    adc1_config_channel_atten(ADC1_CHANNEL_7,MY_ATTENUATION);
    
    Set_IODirectionAndValue(IODirection_ar_bl,IOValues_ar_bl);
    testWiresOnByOne();
    
   // Calibrate();
  
  
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  
  if(ESP_SLEEP_WAKEUP_TIMER == esp_sleep_get_wakeup_cause()){
    testWiresOnByOne();
    if(!WirePluggedIn()){
      esp_deep_sleep_start();
    }
  }
  long TimeToDeepSleep = millis()+15000;

}


/*uint8_t testsettings[][3][2]={
  {{IODirection_ar_bl,IOValues_ar_bl},{IODirection_ar_cl,IOValues_ar_cl},{IODirection_ar_piste,IOValues_ar_piste}},
  {{IODirection_cr_cl,IOValues_cr_cl},{IODirection_cr_bl,IOValues_cr_bl},{IODirection_cr_piste,IOValues_cr_piste}},
  {{IODirection_br_piste,IOValues_br_piste},{IODirection_br_cl,IOValues_br_cl},{IODirection_br_bl,IOValues_br_bl}}
};*/
uint8_t testsettings[][3][2]={
  {{IODirection_cr_cl,IOValues_cr_cl},{IODirection_cr_piste,IOValues_cr_piste},{IODirection_cr_bl,IOValues_cr_bl}},
  {{IODirection_ar_cl,IOValues_ar_cl},{IODirection_ar_piste,IOValues_ar_piste},{IODirection_ar_bl,IOValues_ar_bl}},
  {{IODirection_br_cl,IOValues_br_cl},{IODirection_br_piste,IOValues_br_piste},{IODirection_br_bl,IOValues_br_bl}},
  
};
int analogtestsettings[3]={cl_analog,piste_analog,bl_analog};



void testWiresOnByOne()
{
  for(int Nr=0; Nr<3;Nr++)
  {
    for(int j=0;j<3;j++){
      Set_IODirectionAndValue(testsettings[Nr][j][0],testsettings[Nr][j][1]);
      measurements[Nr][j] = getSample(analogtestsettings[j]);
    }
  }
  return;
}

bool WirePluggedIn(){
for(int Nr=0; Nr<3;Nr++)
  {
    for(int j=0;j<3;j++){
      
      if(measurements[Nr][j] > 1600)
        return true;
    }
  }
  return false;
}


bool testStraightOnly() {
bool bOK = true;
for(int Nr=0; Nr<3;Nr++)
  {
    {
        Set_IODirectionAndValue(testsettings[Nr][Nr][0],testsettings[Nr][Nr][1]);
        
        measurements[Nr][Nr] = getSample(analogtestsettings[Nr]);
        if(measurements[Nr][Nr] < Reference_10_Ohm[Nr])
          bOK = false;
    }
  }
  
  return bOK;

}

bool testArBr() {

  Set_IODirectionAndValue(IODirection_ar_br,IOValues_ar_br);
  if(getSample(br_analog) > 1500)
    return true;
  else 
    return false;

}
bool testArCr() {

  Set_IODirectionAndValue(IODirection_ar_cr,IOValues_ar_cr);
  if(getSample(cr_analog) > 1500)
    return true;
  else 
    return false;

}
bool testArCl() {

  Set_IODirectionAndValue(IODirection_ar_cl,IOValues_ar_cl);
  if(getSample(cl_analog) > 1500)
    return true;
  else 
    return false;

}

bool testBrCr() {

  Set_IODirectionAndValue(IODirection_br_cr,IOValues_br_cr);
  if(getSample(cr_analog) > 1500)
    return true;
  else 
    return false;

}

void DoFoilTest() {
  int timeout = FOIL_TEST_TIMEOUT;
  testWiresOnByOne();
  while(!WirePluggedIn()){
  while((testArBr()));
  // Check if valid or non-valid test
  if(testArCl())
    LedPanel->SetFullMatrix(LedPanel->m_Green);
  else
   LedPanel->SetFullMatrix(LedPanel->m_White);;
  delay(1000);
  if(testArBr()){
    LedPanel->ClearAll();
    LedPanel->myShow();
    timeout = FOIL_TEST_TIMEOUT;}
    else {
      timeout--;
      if(!timeout){
        LedPanel->ClearAll();
        LedPanel->myShow();
        return;
      }
    }
  testWiresOnByOne();
  measurements[1][0] = 0;
  measurements[2][0] = 0;
  }
}

void DoEpeeTest() {
  bool bArCr = false;
  bool bArBr = false;
  bool bBrCr = false;
  testWiresOnByOne();
  while(!WirePluggedIn()){
    bArCr = testArCr();
    bArBr = testArBr();
    bBrCr = testBrCr();
    if(bArCr && !bArBr && !bBrCr){
      LedPanel->SetFullMatrix(LedPanel->m_Green);
      delay(1000);
      if(!testArCr()){
        LedPanel->ClearAll();
        LedPanel->myShow();}
    }
    if(bArBr){
      LedPanel->AnimateArBrConnection();
    }
    if(bBrCr){
      LedPanel->AnimateBrCrConnection();
    }

    testWiresOnByOne();
  }
}

// Simply broken: no contact between i-i' and no contact with other wires
bool IsBroken(int Nr)
{
  if((measurements[Nr][Nr]>2000))
    return false;
  for(int i= 0; i< 3; i++){
    if(i != Nr){
      if(measurements[Nr][i]>1600)
        return false;
    }
  }
  return true;
}

bool IsSwappedWith(int i, int j){
  if((measurements[i][j]>2000) && (measurements[j][i]>2000))
    return true;
  return false;
}

bool AnimateSingleWire(int i)
{
  bool bOK= false;
    if(measurements[i][i] > Reference_10_Ohm[i])
    {
        if((measurements[i][(i+1)%3]<1800) && (measurements[i][(i+2)%3]<1800))
        {
          // OK
          int level = 2;
          if(measurements[i][i] >= Reference_5_Ohm[i])
          level = 1;
          if(measurements[i][i] >= Reference_3_Ohm[i])
          level  = 0;
         

          LedPanel->AnimateGoodConnection(i, level);
          bOK= true;
        }
        else 
        {
            // short
            if(measurements[i][(i+1)%3]>1800)
              LedPanel->AnimateShort(i, (i+1)%3);
            else 
            if(measurements[i][(i+2)%3]>1800)
              LedPanel->AnimateShort(i, (i+2)%3);
        }
    }
    else 
    {
        if((measurements[i][(i+1)%3]<1600) && (measurements[i][(i+2)%3]<1600))
        {
            // Simply broken
            LedPanel->AnimateBrokenConnection(i);
        }
        else 
        {
          if(measurements[i][(i+1)%3]>1600)
            LedPanel->AnimateWrongConnection(i,(i+1)%3);
          if(measurements[i][(i+2)%3]>1600)
            LedPanel->AnimateWrongConnection(i,(i+2)%3);
        }
    }
  return bOK;
}

bool DoQuickCheck(){
bool bAllGood = true;
  testWiresOnByOne();
  for(int i = 0; i < 3; i++)
  {
    bAllGood &= AnimateSingleWire(i);
  }
  delay(500);
  LedPanel->ClearAll();
  delay(300);
  return bAllGood;
}



int timetoswitch = WIRE_TEST_1_TIMEOUT;
int NoWireTimeout = NO_WIRES_PLUGGED_IN_TIMEOUT;
bool bAllGood = true;
State_t State = Waiting;


void loop() {



  if(Waiting == State)
  {
      if(testArCr()){
        //State = EpeeTesting;
        DoEpeeTest();
        TimeToDeepSleep = millis()+10000;
      }
      else{
        if(testArBr()){
          //State = FoilTesting;
          DoFoilTest();
          TimeToDeepSleep = millis()+10000;
        }
      }
      testWiresOnByOne();
      if(WirePluggedIn()){
        State = WireTesting_1;
        NoWireTimeout = NO_WIRES_PLUGGED_IN_TIMEOUT;
        timetoswitch = WIRE_TEST_1_TIMEOUT;
        
      }
      if(TimeToDeepSleep < millis()){
      Serial.println("Going to sleep now");
      Serial.flush(); 
      esp_deep_sleep_start();

      }
  }

  
  if(WireTesting_1 == State)
  {
    TimeToDeepSleep = millis()+10000;
    testWiresOnByOne();
    bAllGood = DoQuickCheck();
    if(bAllGood){
      timetoswitch--;
    }
    else{
      timetoswitch = WIRE_TEST_1_TIMEOUT;
      if(!WirePluggedIn())
        NoWireTimeout--;
      else {
        NoWireTimeout = NO_WIRES_PLUGGED_IN_TIMEOUT;
      }
      if(!NoWireTimeout)
        State = Waiting;

    }
  if(!timetoswitch){
    for(int i = 0; i< 5; i+=2){
      LedPanel->SetLine(i, LedPanel->m_Green);
    }
      LedPanel->myShow();
      delay(1000);
      State = WireTesting_2;
    }
  }
  if(WireTesting_2 == State)
  {
    TimeToDeepSleep = millis()+10000;
    for(int i= 100000;i>0;i--){
      if(!testStraightOnly())
        i = 0;
    }
    
    LedPanel->ClearAll();
    LedPanel->myShow();
    for(int i = 0; i < 3; i++)
    {
      bAllGood &= AnimateSingleWire(i);
    }
    delay(5000);
    timetoswitch = 3;
    LedPanel->ClearAll();
    LedPanel->myShow();
    State = Waiting;
  }
  
}

