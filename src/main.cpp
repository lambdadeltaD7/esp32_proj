#include "HardwareSerial.h"
#include "esp32-hal-gpio.h"
#include <Arduino.h>
#include <cmath>
#include "esp32-hal.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <IRremote.hpp>

#define IR_PIN 4

#define R_PIN 16
#define G_PIN 17
#define B_PIN 18
// int PREV_ACTIVE = -1;


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCK 22
#define SDA 21
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


char R_CLR_STR[8] = "Red";
char G_CLR_STR[8] = "Green";
char B_CLR_STR[8] = "Blue";
char BAD_CLR__[8]   = "ERROR";


inline void sep(char c, int n){
    for(int i=0; i<n; ++i){
        Serial.print(c);
    }
    Serial.print('\n');
}

char* clr2cstr(int clr){
    switch(clr){
        case R_PIN:
            return R_CLR_STR;
        case G_PIN:
            return G_CLR_STR;
        case B_PIN:
            return B_CLR_STR;
        default:
            return BAD_CLR__;
    }
}


struct event{
    int color;
    int dur;
};

const int CYCLE_LEN = 6;
event EVENTS[CYCLE_LEN] = {
    {R_PIN, 1000},
    {G_PIN, 1001},
    {B_PIN, 1002},
    {R_PIN, 2503},
    {G_PIN, 2504},
    {B_PIN, 2505}
};
int EVENT_IX;
unsigned long EVENT_STARTED_AT;

void setup()
{   
    IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);


    Wire.begin(SDA, SCK);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        while (true);
    }
    display.clearDisplay();
    display.display();

    Serial.begin(115200);
    pinMode(R_PIN, OUTPUT);
    pinMode(G_PIN, OUTPUT);
    pinMode(B_PIN, OUTPUT);

    EVENT_IX = 0;
    digitalWrite(EVENTS[EVENT_IX].color, HIGH);
    EVENT_STARTED_AT = millis();
}


void refresh_display(char* curr_clr, int curr_dur){
    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.printf("uptime: %dms", millis());

    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 16);
    display.printf("clr %s", curr_clr);

    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 40);
    display.printf("dur %dms", curr_dur);

    display.display();
}


// void led_cycle(){
//     Serial.println("Entered led_cycle\n");
//     for(int i=0; i<CYCLE_LEN; ++i){
//         Serial.printf("    cycle iteration %d/%d\n", i+1, CYCLE_LEN);
//         if(PREV_ACTIVE != -1){
//             digitalWrite(PREV_ACTIVE, LOW);
//         }
//         Serial.printf("    KILLED %s\n", clr2cstr(PREV_ACTIVE));
//         digitalWrite(EVENTS[i].color, HIGH);
//         Serial.printf("    STARTED %s\n", clr2cstr(EVENTS[i].color));
//         Serial.printf("    SLEEP FOR %d\n\n", EVENTS[i].dur);
//         PREV_ACTIVE = EVENTS[i].color;
//         refresh_display(clr2cstr(EVENTS[i].color), EVENTS[i].dur);
//         delay(EVENTS[i].dur);
//     }
//     Serial.println("Exited led_cycle\n\n");
// }


void led_cycle(){
    if(millis() - EVENT_STARTED_AT < EVENTS[EVENT_IX].dur){
        return;
    }

    digitalWrite(EVENTS[EVENT_IX].color, LOW);
    Serial.printf("    KILLED %s\n", clr2cstr(EVENTS[EVENT_IX].color));
    EVENT_IX = (EVENT_IX + 1) % CYCLE_LEN;
    digitalWrite(EVENTS[EVENT_IX].color, HIGH);
    EVENT_STARTED_AT = millis();
    Serial.printf("    STARTED %s\n", clr2cstr(EVENTS[EVENT_IX].color));
    Serial.printf("    SLEEP FOR %d\n\n", EVENTS[EVENT_IX].dur);    
}

void loop()
{      
    // sep('#',32);
    // Serial.println("Another loop iteration");
    // Serial.printf("Uptime: %dms\n\n", millis());
    led_cycle();
    if (IrReceiver.decode()) {

        Serial.print("Protocol: ");
        Serial.println(IrReceiver.decodedIRData.protocol);

        Serial.print("Address: 0x");
        Serial.println(IrReceiver.decodedIRData.address, HEX);

        Serial.print("Command: 0x");
        Serial.println(IrReceiver.decodedIRData.command, HEX);

        Serial.println();

        IrReceiver.resume();
    }
    refresh_display(clr2cstr(EVENTS[EVENT_IX].color), EVENTS[EVENT_IX].dur);
    delay(50);
}