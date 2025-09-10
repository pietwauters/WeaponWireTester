//Copyright (c) Piet Wauters 2022 <piet.wauters@gmail.com>
#include "WS2812BLedMatrix.h"
// Attention the order of the leds is "snake", starting top left = 0
// 0,1,2,3,4
// 9,8,7,6,5
// 10,11,12,13,14
// 19,18,17,16,15
// 20,21,22,23,24
int WS2812B_LedMatrix::MapCoordinates(int i, int j)
{
  if(i%2){
    //i is odd
    return (i+1)*5-j-1;
  }
  else{
    return(i*5+j);
  }

}


WS2812B_LedMatrix::WS2812B_LedMatrix()
{
    //ctor
    pinMode(PIN, OUTPUT);
    digitalWrite(PIN, LOW);
    pinMode(BUZZERPIN, OUTPUT);
    digitalWrite(BUZZERPIN, RELATIVE_LOW);
    m_pixels = new Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
    //m_pixels->begin();
    //m_pixels->fill(m_pixels->Color(0, 0, 0),0,NUMPIXELS);
    SetBrightness(BRIGHTNESS_NORMAL);
    //queue = xQueueCreate( 60, sizeof( int ) );
}
void WS2812B_LedMatrix::begin()
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN, LOW);
  m_pixels->begin();
  m_pixels->fill(m_pixels->Color(0, 0, 0),0,NUMPIXELS);
  m_pixels->clear();
  m_pixels->show();
}

void WS2812B_LedMatrix::SetBrightness(uint8_t val)
{
  m_Brightness = val;
  m_pixels->setBrightness(m_Brightness);
  m_Red = Adafruit_NeoPixel::Color(255, 0, 0, m_Brightness);
  m_Green = Adafruit_NeoPixel::Color(0, 255, 0, m_Brightness);
  m_White = Adafruit_NeoPixel::Color(200, 200, 200, m_Brightness);
  m_Orange = Adafruit_NeoPixel::Color(160, 60, 0, m_Brightness);
  m_Yellow = Adafruit_NeoPixel::Color(204, 148, 0, m_Brightness);
  m_Blue = Adafruit_NeoPixel::Color(0, 0, 255, m_Brightness);
  m_Purple = Adafruit_NeoPixel::Color(105, 0, 200, m_Brightness);
  m_Off = Adafruit_NeoPixel::Color(0, 0, 0, m_Brightness);
}


WS2812B_LedMatrix::~WS2812B_LedMatrix()
{
    //dtor
    delete m_pixels;
}


void WS2812B_LedMatrix::ClearAll()
{
   setBuzz(false);
   m_pixels->fill(m_pixels->Color(0, 0, 0),0,NUMPIXELS);
   myShow();
}



void WS2812B_LedMatrix::setBuzz(bool Value)
{
    if(m_Loudness)
    {
        if(Value)
        {
            digitalWrite(BUZZERPIN, RELATIVE_HIGH);
        }
        else
        {
            digitalWrite(BUZZERPIN, RELATIVE_LOW);
        }
    }

}

void WS2812B_LedMatrix::SetLine(int i, uint32_t theColor){
m_pixels->fill(theColor,i*5,5);
//m_pixels->show();
}

// 0,1,2,3,4
// 9,8,7,6,4
// 10,11,12,13,14
// 19,18,17,16,15
// 20,21,22,23,24
uint8_t animation_sequence[][2][7] ={
  {{0,1,2,7,12,13,14},{10,11,12,7,2,3,4}},
  {{10,11,12,17,22,23,24},{20,21,22,17,12,13,14}},
  {{0,1,8,12,16,23,24},{20,21,18,12,6,3,4}},
};

void WS2812B_LedMatrix::AnimateSwap(int i, int j){
  int k,l,m;
  if(i>j){k=j;l=i;}
  else{k=i;l=j;}

  uint32_t currentcolor = m_Blue;
  if(l-k == 1){
    m = k;
  }
  else{
      m=2;
    }
  delay(100);
  for(int j = 0; j<2; j++){
    for(int i = 0; i<7; i++){
      m_pixels->setPixelColor(animation_sequence[m][j][i],currentcolor);
      m_pixels->show();
      delay(70*animationspeed);
      //m_pixels->setPixelColor(animation_sequence[j][i],m_Off);
    }
    currentcolor = m_Red;
    delay(300);

  }
  m_pixels->show();

}

// We arrange the sequences such that m = 2*(i+1)-j;
uint8_t animation_sequence_wrong[][7] ={
  {4,4,6,12,18,20,20}, // 0-2
   {4,3,2,7,12,11,10}, //0-1
  {14,13,12,17,22,21,20},//1-2
  {4,3,2,1,0,0,0}, // unused
  {14,13,12,7,2,1,0}, //1-0
  {24,23,22,17,12,11,10},// 2-1
  {24,24,16,12,8,0,0}, //2-0
  
};
void WS2812B_LedMatrix::AnimateWrongConnection(int i, int j){
  
  uint32_t currentcolor = m_Blue;
  int m = (i+1)*2-j;
  
  
    for(int i = 0; i<7; i++){
      m_pixels->setPixelColor(animation_sequence_wrong[m][i],currentcolor);
      m_pixels->show();
      delay(70*animationspeed/100);
      
    }
    


}

void WS2812B_LedMatrix::AnimateShort(int i, int j){
  int k,l,m;
  if(i>j){k=j;l=i;}
  else{k=i;l=j;}

  uint32_t currentcolor = m_Yellow;
  if(l-k == 1){
    m = k;
  }
  else{
      m=2;
    }
  delay(100);
  for(int j = 0; j<2; j++){
    for(int i = 0; i<7; i++){
      m_pixels->setPixelColor(animation_sequence[m][j][i],currentcolor);
      m_pixels->show();
      delay(70*animationspeed/100);
      //m_pixels->setPixelColor(animation_sequence[j][i],m_Off);
    }

    delay(200*animationspeed/100);

  }
  m_pixels->show();

}

void WS2812B_LedMatrix::AnimateGoodConnection(int k,int level){
uint32_t currentcolor = m_Green;
  switch(level){
    case 1:
    currentcolor = m_Yellow;
    break;

    case 2:
    currentcolor = m_Red;
    break;

  }
  for(int i = 10*k+4; i>=10*k; i--){
    m_pixels->setPixelColor(i,currentcolor);
    m_pixels->show();
    delay(60*animationspeed/100);
  }
}

void WS2812B_LedMatrix::AnimateBrokenConnection(int k){
  int i = k*2;
    for(int j = 4; j>=0; j-=2){
    m_pixels->setPixelColor(MapCoordinates(i,j),m_Red);
    m_pixels->show();
    delay(140*animationspeed/100);
    }

}

void WS2812B_LedMatrix::SetSwappedLines(int i, int j){
  int k,l;
  if(i>j){k=j;l=i;}
  else{k=i;l=j;}
  if((l-k == 1) ){
    m_pixels->setPixelColor(5*k+0,m_Blue);
    m_pixels->setPixelColor(5*k+1,m_Blue);
    m_pixels->setPixelColor(5*k+2,m_Blue);
    m_pixels->setPixelColor(5*k+5,m_Blue);
    m_pixels->setPixelColor(5*k+6,m_Blue);

    m_pixels->setPixelColor(5*k+4,m_Purple);
    m_pixels->setPixelColor(5*k+9,m_Purple);
    m_pixels->setPixelColor(5*k+7,m_Purple);
    m_pixels->setPixelColor(5*k+3,m_Purple);
    m_pixels->setPixelColor(5*k+8,m_Purple);

  }
  else{
    m_pixels->setPixelColor(0,m_Blue);
    m_pixels->setPixelColor(1,m_Blue);
    //m_pixels->setPixelColor(2,m_Blue);
    m_pixels->setPixelColor(13,m_Blue);
    m_pixels->setPixelColor(14,m_Blue);

    m_pixels->setPixelColor(4,m_Purple);
    //m_pixels->setPixelColor(3,m_Purple);

    m_pixels->setPixelColor(7,m_Blue);

    m_pixels->setPixelColor(6,m_Purple);

    m_pixels->setPixelColor(10,m_Purple);
    m_pixels->setPixelColor(11,m_Purple);
    m_pixels->setPixelColor(12,m_Purple);

  }


  //m_pixels->show();
}

uint8_t animation_sequence_ArBr[] = {14,13,16,23,24}; 

void WS2812B_LedMatrix::AnimateArBrConnection(){
  
  uint32_t currentcolor = m_Blue;
  
    for(int i = 0; i<5; i++){
      m_pixels->setPixelColor(animation_sequence_ArBr[i],currentcolor);
      m_pixels->show();
      delay(170*animationspeed/100);
      
    }
    delay(300*animationspeed/100);
    ClearAll();
}

uint8_t animation_sequence_BrCr[] = {4,3,6,13,16,23,24}; 

void WS2812B_LedMatrix::AnimateBrCrConnection(){
  
  uint32_t currentcolor = m_Blue;
  
    for(int i = 0; i<7; i++){
      m_pixels->setPixelColor(animation_sequence_BrCr[i],currentcolor);
      m_pixels->show();
      delay(130*animationspeed/100);
    }
    delay(300*animationspeed/100);
    ClearAll();
}
