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
void turnOffScreen(){
    power = false;
    backgroundLayer.fillScreen(COLOR_BLACK);
    backgroundLayer.swapBuffers();
    Serial1.println("Screen Off");
}
void turnOnScreen() {
    power = true;
    backgroundLayer.swapBuffers();
    Serial1.println("Screen Off");
}

void setup(){
    setScreenClearCallback(screenClearCallback);
    setUpdateScreenCallback(updateScreenCallback);
    setDrawPixelCallback(drawPixelCallback);
    Serial.begin(115200);
    Serial1.begin(9600);
    String str = "";
    while(str == "")
        while(Serial1.available())
            str += char(Serial1.read()); //store string from serial command
    
    Serial1.println("Connected");
    power = true;
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
        Serial1.println("No SD card");
    num_files = enumerateGIFFiles(GIF_DIRECTORY, true);
    if(num_files < 0)
        Serial1.println("No gifs directory");
    if(!num_files)
        Serial1.println("Empty gifs directory");
    for (size_t i = 0; i < num_files; i++) {

    }
}

void loop() {
    char pathname[30] = {' '};
    int index = 0;

    int arrPosition = 0;
    int arrLength = 0;
    int gifList[500];
    bool arrImg = false;

    String dispText;
    char charBuf[1000];


    while(1) {
        while(Serial1.available()){
            message += char(Serial1.read());
            delay(100);
        }

        // Deal with the bluetooth cmds
        if(!Serial1.available()){
            if(message != ""){
                Serial1.println(message); //show the data
                String temp = message.charAt(0);
                if (temp.toInt() || temp == "0"){
                    // Can clear screen for new animation here, but this might cause flicker with short animations
                    arrImg = false;
                    index = 0;
                    for (int i = 0; i < message.length(); i++) {
                        index *= 10;
                        temp = message.charAt(i);
                        index += temp.toInt();
                    }
                    backgroundLayer.fillScreen(COLOR_BLACK);
                    backgroundLayer.swapBuffers();
                    getGIFFilenameByIndex(GIF_DIRECTORY, index, pathname);
                    message = "";
                    Serial1.println("Running Single gif");
                }
                else if (message.charAt(0) == '('){
                    // Reset the array
                    message.remove(0,1);
                    arrLength = 0;
                    arrPosition = 0;
                    arrImg = true;
                    String str = "";
                    for (int i = 0; i <= message.length()+1; i++) {
                        if(message.charAt(i) == ' ' || message.charAt(i) == ')'){
                            int val = str.toInt();
                            gifList[arrLength] = val;
                            arrLength++;
                            str = "";
                        }
                        else{
                            str += message.charAt(i);
                        }
                    }

                    Serial1.println("Sequence set");
                    message = "";
                }
                else if (message.charAt(0) == '*'){ // Print scrolling text
                    pathname[0] = ' ';
                    pathname[1] = '\0';
                    backgroundLayer.fillScreen(COLOR_BLACK);
                    backgroundLayer.swapBuffers();
                    delay (200);
                    message.remove(0,1);
                    message.toCharArray(charBuf, 1000);
                    scrollingLayer.setColor({0xff, 0xcc, 0x21});
                    scrollingLayer.setMode(wrapForward);
                    scrollingLayer.setSpeed(20);
                    scrollingLayer.setFont(font8x13);
                    scrollingLayer.start(charBuf, 1);
                    message = "";
                }
                else if (message.charAt(0) == '$'){ // Device setting cmds
                    if(message.substring(0,4).equals("$off"))
                        turnOffScreen();
                    else if(message.substring(0,3).equals("$on"))
                        turnOnScreen();
                    else if(message.substring(0,14).equals("$setBrightness")){
                        message.remove(0,15);
                        message = message.substring(0, message.length() - 1);
                        int val = message.toInt();
                        if (val < 256 && val > 0)
                            matrix.setBrightness(val);
                        else
                            Serial1.println("Not a valid brightness");
                    }
                    else
                        Serial1.println("Command not recognized");
                    message = "";
                }
                else{ // Catch unsupported input
                    Serial1.println("Input not recognized");
                    message = "";
                }
            }
            if(arrImg){
                arrPosition++;
                arrPosition %= arrLength;
                index = gifList[arrPosition];
                backgroundLayer.fillScreen(COLOR_BLACK);
                backgroundLayer.swapBuffers();
                getGIFFilenameByIndex(GIF_DIRECTORY, index, pathname);
            }
        }

        // Display the gif
        if(*pathname != ' ' && power){
            futureTime = millis() + 5000;
            while(millis() < futureTime)
                processGIFFile(pathname);
        }
    }
}
