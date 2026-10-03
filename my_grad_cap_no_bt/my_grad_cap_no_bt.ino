#include <SmartMatrix3.h>
#include <SPI.h>
#include <SD.h>
#include "GIFDecoder.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define DISPLAY_TIME_SECONDS 10
#define ENABLE_SCROLLING  1

// range 0-255
int defaultBrightness = 255;
long futureTime;
const rgb24 COLOR_BLACK = { 0, 0, 0 };
#define COLOR_DEPTH 24                  // known working: 24, 48 - If the sketch uses type `rgb24` directly, COLOR_DEPTH must be 24
const uint8_t kMatrixWidth = 32;        // known working: 32, 64, 96, 128
const uint8_t kMatrixHeight = 32;       // known working: 16, 32, 48, 64
const uint8_t kRefreshDepth = 36;       // known working: 24, 36, 48
const uint8_t kDmaBufferRows = 2;       // known working: 2-4
const uint8_t kPanelType = SMARTMATRIX_HUB75_32ROW_MOD16SCAN; // use SMARTMATRIX_HUB75_16ROW_MOD8SCAN for common 16x32 panels
const uint8_t kMatrixOptions = (SMARTMATRIX_OPTIONS_NONE);    // see http://docs.pixelmatix.com/SmartMatrix for options
const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);
const uint8_t kScrollingLayerOptions = (SM_SCROLLING_OPTIONS_NONE);

SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);
SMARTMATRIX_ALLOCATE_SCROLLING_LAYER(scrollingLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kScrollingLayerOptions);

// Chip select for SD card on the SmartMatrix Shield
#define SD_CS 15
#define GIF_DIRECTORY "/gifs/"

int num_files;
String message;
bool power;
void screenClearCallback(void){
    backgroundLayer.fillScreen({0,0,0});
}
void updateScreenCallback(void){
    backgroundLayer.swapBuffers();
}
void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
    backgroundLayer.drawPixel(x, y, {red, green, blue});
}

void setup(){
    setScreenClearCallback(screenClearCallback);
    setUpdateScreenCallback(updateScreenCallback);
    setDrawPixelCallback(drawPixelCallback);
    Serial.begin(115200);
    //matrix.setRotation(rotation180);
    matrix.setBrightness(255);
    matrix.addLayer(&backgroundLayer);
    matrix.addLayer(&scrollingLayer);
    matrix.begin();
    backgroundLayer.fillScreen(COLOR_BLACK);
    backgroundLayer.swapBuffers();
    // SD Card
    pinMode(SD_CS, OUTPUT);
    if (!SD.begin(SD_CS))
        Serial.println("No SD card");
    num_files = enumerateGIFFiles(GIF_DIRECTORY, true);
    if(num_files < 0)
        Serial.println("No gifs directory");
    if(!num_files)
        Serial.println("Empty gifs directory");
    for (size_t i = 0; i < num_files; i++) {

    }
}

void loop() {
    char pathname[30] = {' '};
    int index = 0;

    int arrPosition = 0;
    int arrLength = num_files;


    while(1) {
        index++;
        index %= arrLength;
        backgroundLayer.fillScreen(COLOR_BLACK);
        backgroundLayer.swapBuffers();
        getGIFFilenameByIndex(GIF_DIRECTORY, index, pathname);

        // Display the gif
        if(*pathname != ' '){
            futureTime = millis() + 5000;
            while(millis() < futureTime)
                processGIFFile(pathname);
        }
    }
}
