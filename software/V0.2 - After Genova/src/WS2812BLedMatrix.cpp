// Copyright (c) Piet Wauters 2022 <piet.wauters@gmail.com>
#include "WS2812BLedMatrix.h"
// Attention the order of the leds is "snake", starting top left = 0
// 0,1,2,3,4
// 9,8,7,6,5
// 10,11,12,13,14
// 19,18,17,16,15
// 20,21,22,23,24
int WS2812B_LedMatrix::MapCoordinates(int i, int j) {
    if (i % 2) {
        // i is odd
        return (i + 1) * 5 - j - 1;
    } else {
        return (i * 5 + j);
    }
}

// Static transform function implementations
int WS2812B_LedMatrix::transformStandard(int n) { return n; }

int WS2812B_LedMatrix::transformMirrored(int n) { return n + 10 * (2 - n / 5); }

void WS2812B_LedMatrix::setMirrorMode(bool mirrored) {
    m_transformFunc = mirrored ? transformMirrored : transformStandard;
}

WS2812B_LedMatrix::WS2812B_LedMatrix() {
    // ctor
    pinMode(PIN, OUTPUT);
    digitalWrite(PIN, LOW);
    pinMode(BUZZERPIN, OUTPUT);
    digitalWrite(BUZZERPIN, RELATIVE_LOW);
    m_pixels = new Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
    SetBrightness(BRIGHTNESS_NORMAL);
    // Default to standard transform
    m_transformFunc = transformStandard;

    // queue = xQueueCreate( 60, sizeof( int ) );
}
void WS2812B_LedMatrix::begin() {
    pinMode(PIN, OUTPUT);
    digitalWrite(PIN, LOW);
    m_pixels->begin();
    m_pixels->fill(m_pixels->Color(0, 0, 0), 0, NUMPIXELS);
    m_pixels->clear();
    m_pixels->show();
}

void WS2812B_LedMatrix::SetBrightness(uint8_t val) {
    m_Brightness = val;
    m_pixels->setBrightness(m_Brightness);
    m_Red = Adafruit_NeoPixel::Color(255, 0, 0, m_Brightness);
    m_Green = Adafruit_NeoPixel::Color(0, 255, 0, m_Brightness);
    m_White = Adafruit_NeoPixel::Color(200, 200, 200, m_Brightness);
    m_Orange = Adafruit_NeoPixel::Color(255, 70, 0, m_Brightness);
    m_Yellow = Adafruit_NeoPixel::Color(255, 251, 0, m_Brightness);
    m_Blue = Adafruit_NeoPixel::Color(0, 0, 255, m_Brightness);
    m_Purple = Adafruit_NeoPixel::Color(105, 0, 200, m_Brightness);
    m_Off = Adafruit_NeoPixel::Color(0, 0, 0, m_Brightness);
}

WS2812B_LedMatrix::~WS2812B_LedMatrix() {
    // dtor
    delete m_pixels;
}

void WS2812B_LedMatrix::ClearAll() {
    setBuzz(false);
    m_pixels->fill(m_pixels->Color(0, 0, 0), 0, NUMPIXELS);
    myShow();
}

void WS2812B_LedMatrix::SequenceTest() {
    ClearAll();
    m_pixels->show();
    for (int i = 0; i < NUMPIXELS; i++) {
        if (i % 2)
            m_pixels->setPixelColor(m_transformFunc(i), m_Yellow);
        else
            m_pixels->setPixelColor(m_transformFunc(i), m_Orange);
        m_pixels->show();
        delay(animationspeed);
    }
    ClearAll();
    m_pixels->show();
}

void WS2812B_LedMatrix::setBuzz(bool Value) {
    if (m_Loudness) {
        if (Value) {
            digitalWrite(BUZZERPIN, RELATIVE_HIGH);
        } else {
            digitalWrite(BUZZERPIN, RELATIVE_LOW);
        }
    }
}

void WS2812B_LedMatrix::SetLine(int i, uint32_t theColor) {
    // m_pixels->fill(theColor,i*5,5);
    for (int j = i * 5; j < i * 5 + 5; j++) {
        m_pixels->setPixelColor(m_transformFunc(j), theColor);
    }
}

// 0,1,2,3,4
// 9,8,7,6,4
// 10,11,12,13,14
// 19,18,17,16,15
// 20,21,22,23,24
uint8_t animation_sequence[][2][7] = {
    {{0, 1, 2, 7, 12, 13, 14}, {10, 11, 12, 7, 2, 3, 4}},
    {{10, 11, 12, 17, 22, 23, 24}, {20, 21, 22, 17, 12, 13, 14}},
    {{0, 1, 8, 12, 16, 23, 24}, {20, 21, 18, 12, 6, 3, 4}},
};

void WS2812B_LedMatrix::AnimateSwap(int i, int j) {
    int k, l, m;
    if (i > j) {
        k = j;
        l = i;
    } else {
        k = i;
        l = j;
    }

    uint32_t currentcolor = m_Blue;
    if (l - k == 1) {
        m = k;
    } else {
        m = 2;
    }
    delay(100);
    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < 7; i++) {
            m_pixels->setPixelColor(m_transformFunc(animation_sequence[m][j][i]), currentcolor);
            m_pixels->show();
            delay(70 * animationspeed);
            // m_pixels->setPixelColor(animation_sequence[j][i],m_Off);
        }
        currentcolor = m_Red;
        delay(300);
    }
    m_pixels->show();
}

// We arrange the sequences such that m = 2*(i+1)-j;
#ifdef CONFIG_15_20

uint8_t animation_sequence_wrong[][7] = {
    {24, 24, 16, 12, 8, 0, 0},     // 0-2
    {14, 13, 12, 7, 2, 1, 0},      // 1-0
    {24, 23, 22, 17, 12, 11, 10},  // 2-1
    {4, 3, 2, 1, 0, 0, 0},         // unused
    {4, 3, 2, 7, 12, 11, 10},      // 0-1
    {14, 13, 12, 17, 22, 21, 20},  // 1-2
    {4, 4, 6, 12, 18, 20, 20}      // 2-0

};
#else
uint8_t animation_sequence_wrong[][7] = {
    {4, 4, 6, 12, 18, 20, 20},     // 0-2
    {4, 3, 2, 7, 12, 11, 10},      // 0-1
    {14, 13, 12, 17, 22, 21, 20},  // 1-2
    {4, 3, 2, 1, 0, 0, 0},         // unused
    {14, 13, 12, 7, 2, 1, 0},      // 1-0
    {24, 23, 22, 17, 12, 11, 10},  // 2-1
    {24, 24, 16, 12, 8, 0, 0},     // 2-0

};
#endif

void WS2812B_LedMatrix::AnimateWrongConnection(int i, int j) {
    uint32_t currentcolor = m_Blue;
    int m = (i + 1) * 2 - j;

    for (int i = 0; i < 7; i++) {
        m_pixels->setPixelColor(m_transformFunc(animation_sequence_wrong[m][i]), currentcolor);
        m_pixels->show();
        delay(70 * animationspeed / 100);
    }
}

void WS2812B_LedMatrix::AnimateShort(int i, int j) {
    int k, l, m;
    if (i > j) {
        k = j;
        l = i;
    } else {
        k = i;
        l = j;
    }

    uint32_t currentcolor = m_Yellow;
    if (l - k == 1) {
        m = k;
    } else {
        m = 2;
    }
    delay(100);
    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < 7; i++) {
            m_pixels->setPixelColor(m_transformFunc(animation_sequence[m][j][i]), currentcolor);
            m_pixels->show();
            delay(70 * animationspeed / 100);
            // m_pixels->setPixelColor(animation_sequence[j][i],m_Off);
        }

        delay(200 * animationspeed / 100);
    }
    m_pixels->show();
}

void WS2812B_LedMatrix::AnimateGoodConnection(int k, int level) {
    uint32_t currentcolor = m_Green;
    switch (level) {
        case 1:
            currentcolor = m_Yellow;
            break;

        case 2:
            currentcolor = m_Orange;
            break;
    }
    for (int i = 10 * k + 4; i >= 10 * k; i--) {
        m_pixels->setPixelColor(m_transformFunc(i), currentcolor);
        m_pixels->show();
        delay(60 * animationspeed / 100);
    }
}

void WS2812B_LedMatrix::AnimateBrokenConnection(int k) {
    int i = k * 2;
    for (int j = 4; j >= 0; j -= 2) {
        m_pixels->setPixelColor(m_transformFunc(MapCoordinates(i, j)), m_Red);
        m_pixels->show();
        delay(140 * animationspeed / 100);
    }
}

void WS2812B_LedMatrix::SetSwappedLines(int i, int j) {
    int k, l;
    if (i > j) {
        k = j;
        l = i;
    } else {
        k = i;
        l = j;
    }
    if ((l - k == 1)) {
        m_pixels->setPixelColor(m_transformFunc(5 * k + 0), m_Blue);
        m_pixels->setPixelColor(m_transformFunc(5 * k + 1), m_Blue);
        m_pixels->setPixelColor(m_transformFunc(5 * k + 2), m_Blue);
        m_pixels->setPixelColor(m_transformFunc(5 * k + 5), m_Blue);
        m_pixels->setPixelColor(m_transformFunc(5 * k + 6), m_Blue);

        m_pixels->setPixelColor(m_transformFunc(5 * k + 4), m_Purple);
        m_pixels->setPixelColor(m_transformFunc(5 * k + 9), m_Purple);
        m_pixels->setPixelColor(m_transformFunc(5 * k + 7), m_Purple);
        m_pixels->setPixelColor(m_transformFunc(5 * k + 3), m_Purple);
        m_pixels->setPixelColor(m_transformFunc(5 * k + 8), m_Purple);

    } else {
        m_pixels->setPixelColor(m_transformFunc(0), m_Blue);
        m_pixels->setPixelColor(m_transformFunc(1), m_Blue);
        // m_pixels->setPixelColor(2,m_Blue);
        m_pixels->setPixelColor(m_transformFunc(13), m_Blue);
        m_pixels->setPixelColor(m_transformFunc(14), m_Blue);

        m_pixels->setPixelColor(m_transformFunc(4), m_Purple);
        // m_pixels->setPixelColor(3,m_Purple);

        m_pixels->setPixelColor(m_transformFunc(7), m_Blue);

        m_pixels->setPixelColor(m_transformFunc(6), m_Purple);

        m_pixels->setPixelColor(m_transformFunc(10), m_Purple);
        m_pixels->setPixelColor(m_transformFunc(11), m_Purple);
        m_pixels->setPixelColor(m_transformFunc(12), m_Purple);
    }

    // m_pixels->show();
}
#ifdef CONFIG_15_20
uint8_t animation_sequence_ArBr[] = {10, 11, 18, 21, 20};
#else
uint8_t animation_sequence_ArBr[] = {14, 13, 16, 23, 24};
#endif

void WS2812B_LedMatrix::AnimateArBrConnection() {
    uint32_t currentcolor = m_Blue;

    for (int i = 0; i < 5; i++) {
        m_pixels->setPixelColor(m_transformFunc(animation_sequence_ArBr[i]), currentcolor);
        m_pixels->show();
        delay(170 * animationspeed / 100);
    }
    delay(300 * animationspeed / 100);
    ClearAll();
}

#ifdef CONFIG_15_20
uint8_t animation_sequence_BrCr[] = {20, 21, 18, 11, 8, 1, 0};
#else
uint8_t animation_sequence_BrCr[] = {4, 3, 6, 13, 16, 23, 24};
#endif

void WS2812B_LedMatrix::AnimateBrCrConnection() {
    uint32_t currentcolor = m_Blue;

    for (int i = 0; i < 7; i++) {
        m_pixels->setPixelColor(m_transformFunc(animation_sequence_BrCr[i]), currentcolor);
        m_pixels->show();
        delay(130 * animationspeed / 100);
    }
    delay(300 * animationspeed / 100);
    ClearAll();
}

uint8_t Letter_P[] = {2, 7, 12, 17, 22, 14, 13, 11, 10};
// uint8_t Letter_P[] = {7, 12, 17, 13, 11};

void WS2812B_LedMatrix::Draw_P(uint32_t theColor) {
    for (int i = 0; i < 9; i++) {
        m_pixels->setPixelColor(m_transformFunc(Letter_P[i]), theColor);
    }
    m_pixels->show();
}

uint8_t DiamondShape[] = {2, 8, 7, 6, 10, 11, 12, 13, 14, 18, 17, 16, 22};
void WS2812B_LedMatrix::DrawDiamond(uint32_t theColor) {
    for (int i = 0; i < 13; i++) {
        m_pixels->setPixelColor(m_transformFunc(DiamondShape[i]), theColor);
    }
    m_pixels->show();
}

#ifdef CONFIG_15_20
uint8_t Letter_E[] = {0, 1, 2, 3, 4, 5, 7, 9, 10, 12, 14, 15, 19};
#else
uint8_t Letter_E[] = {20, 21, 22, 23, 24, 19, 10, 9, 17, 12, 15, 14, 5};
#endif

void WS2812B_LedMatrix::Draw_E(uint32_t theColor) {
    for (int i = 0; i < 13; i++) {
        m_pixels->setPixelColor(m_transformFunc(Letter_E[i]), theColor);
    }
    m_pixels->show();
}

#ifdef CONFIG_15_20
uint8_t Letter_F[] = {5, 6, 7, 8, 9, 12, 14, 15, 17};
#else
uint8_t Letter_F[] = {15, 16, 17, 18, 19, 10, 9, 17, 12};
#endif
void WS2812B_LedMatrix::Draw_F(uint32_t theColor) {
    for (int i = 0; i < 9; i++) {
        m_pixels->setPixelColor(m_transformFunc(Letter_F[i]), theColor);
    }
    m_pixels->show();
}

void WS2812B_LedMatrix::ConfigureBlinking(int PixelNr, uint32_t theColor, int OnTime, int OffTime, int Repeat) {
    m_BlinkingPixel = PixelNr;  // -1 means no blinking
    m_BlinkingColor = theColor;
    m_BlinkingOnTime = OnTime;
    m_BlinkingOffTime = OffTime;
    m_BlinkingRepeat = Repeat;  // 0 means infinite blinking
    m_BlinkingState = false;
}

void WS2812B_LedMatrix::Blink() {
    if (m_BlinkingPixel < 0)
        return;  // No blinking configured
    long currentTime = millis();
    if (m_BlinkingNextTimeToChange > currentTime)
        return;

    if (m_BlinkingState) {
        // Turn off the blinking pixel
        m_pixels->setPixelColor(m_BlinkingPixel, m_Off);
        m_BlinkingState = false;
        m_BlinkingNextTimeToChange = currentTime + m_BlinkingOffTime;
    } else {
        // Turn on the blinking pixel
        m_pixels->setPixelColor(m_BlinkingPixel, m_BlinkingColor);
        m_BlinkingNextTimeToChange = currentTime + m_BlinkingOnTime;
        m_BlinkingState = true;
    }
    m_pixels->show();
}

void WS2812B_LedMatrix::RestartBlink() {
    m_BlinkingState = true;
    m_BlinkingNextTimeToChange = millis() + m_BlinkingOnTime;  // Start with off state
    if (m_BlinkingPixel >= 0) {
        m_pixels->setPixelColor(m_BlinkingPixel, m_BlinkingColor);  // Ensure pixel is off initially
        m_pixels->show();
    }
}

void WS2812B_LedMatrix::SetInner9(uint32_t theColor) {
    ClearAll();
    m_pixels->fill(theColor, 6, 3);
    m_pixels->fill(theColor, 16, 3);
    m_pixels->fill(theColor, 11, 3);
    myShow();
}