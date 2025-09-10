//Copyright (c) Piet Wauters 2022 <piet.wauters@gmail.com>
#ifndef WS2812B_LEDMATRIX_H
#define WS2812B_LEDMATRIX_H
#include <Adafruit_NeoPixel.h>
//#include "SubjectObserverTemplate.h"


////////////////////////////////////////////////////////////////////////////////////
// Which pin on the Arduino is connected to the NeoPixels?

#define PIN 26
#define BUZZERPIN 22
#define RELATIVE_HIGH HIGH
#define RELATIVE_LOW LOW

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 25

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.

/////////////////////////////////////////////////////////////////////////////////////

#define MASK_RED 0x80
#define MASK_WHITE_L 0x40
#define MASK_ORANGE_L 0x20
#define MASK_ORANGE_R 0x10
#define MASK_WHITE_R 0x08
#define MASK_GREEN 0x04
#define MASK_BUZZ 0x02

#define BRIGHTNESS_LOW 15
#define BRIGHTNESS_NORMAL 150
#define BRIGHTNESS_HIGH 100
#define BRIGHTNESS_ULTRAHIGH 125



class WS2812B_LedMatrix
{
    public:
        /** Default constructor */
        WS2812B_LedMatrix();
        /** Default destructor */
        virtual ~WS2812B_LedMatrix();

        /** Access m_LedStatus
         * \return The current value of m_LedStatus
         */

        /** Set m_LedStatus
         * \param val New value to set
         */
        void ClearAll();
        void setBuzz(bool Value);
        void myShow(){m_pixels->show();};
        void SetBrightness(uint8_t val);
        void begin();
        void SetLine(int i, uint32_t theColor);
        void SetFullMatrix(uint32_t theColor){m_pixels->fill(theColor,0,NUMPIXELS);myShow();};
        void SetSwappedLines(int i, int j);
        void AnimateSwap(int i, int j);
        void AnimateShort(int i, int j);
        void AnimateGoodConnection(int k,int level = 0);
        void AnimateBrokenConnection(int k);
        void AnimateWrongConnection(int i, int j);
        void AnimateArBrConnection();
        void AnimateBrCrConnection();
        int MapCoordinates(int i, int j);
        uint32_t   m_Red;
        uint32_t   m_Purple;
        uint32_t   m_Green;
        uint32_t   m_White;
        uint32_t   m_Orange;
        uint32_t   m_Yellow;
        uint32_t   m_Blue;
        uint32_t   m_Off;

    protected:

    private:


        Adafruit_NeoPixel *m_pixels;
        uint8_t m_Brightness = BRIGHTNESS_NORMAL;
        bool m_Loudness = true;
        int animationspeed = 100;


        QueueHandle_t queue = NULL;

};

#endif // WS2812B_LEDMATRIX_H
