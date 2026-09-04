#include "HardwareSerial.h"
#include "esp32-hal-gpio.h"
#include <Arduino.h>
#include <cmath>
#include "esp32-hal.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <IRremote.hpp>
#include "esp32-hal-cpu.h"



#define LOOP_SLEEP_TIME_MS 50 

#define BUTTON_PIN 23


#define IR_PIN 4

#define R_PIN 16
#define G_PIN 17
#define B_PIN 18


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCK 22
#define SDA 21
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


struct event{
    int color;
    int dur;
};

char R_CLR_STR[8] = "Red";
char G_CLR_STR[8] = "Green";
char B_CLR_STR[8] = "Blue";
char BAD_CLR__[8]   = "ERROR";


const int CNT_DISPLAY_STATES = 3;
enum class DisplayState{
    LED_INFO,
    IR_INFO,
    STATIC
};

char DISPLAY_LED_INFO[16] = "LED_INFO_MODE";
char DISPLAY_IR_INFO[16] = "IR_INFO_MODE";
char DISPLAY_STATIC[16] = "STATIC_MODE";


char* DisplayState2cstr(DisplayState ds){
    switch(ds){
        case DisplayState::LED_INFO: return DISPLAY_LED_INFO;
        case DisplayState::IR_INFO: return DISPLAY_IR_INFO;
        case DisplayState::STATIC: return DISPLAY_STATIC;
    }
}

enum class IRKeys{
    k0,
    k1,
    k2,
    k3,
    k4,
    k5,
    k6,
    k7,
    k8,
    k9,
    kstar,
    khash,
    kok,
    kup,
    kdown,
    kleft,
    kright,
    kERROR,
    kNONE
};


char k0_str[8] = "0";
char k1_str[8] = "1";
char k2_str[8] = "2";
char k3_str[8] = "3";
char k4_str[8] = "4";
char k5_str[8] = "5";
char k6_str[8] = "6";
char k7_str[8] = "7";
char k8_str[8] = "8";
char k9_str[8] = "9";
char kstar_str[8] = "*";
char khash_str[8] = "#";
char kok_str[8] = "OK";
char kup_str[8] = "UP";
char kdown_str[8] = "DOWN";
char kleft_str[8] = "LEFT";
char kright_str[8] = "RIGHT";
char kNONE_str[8] = "NONE";
char kERROR_str[8] = "ERROR";

IRKeys LAST_PRESSED_KEY = IRKeys::kERROR;
unsigned long LAST_PRESSED_KEY_TS = 0;

IRKeys cmd2key(uint16_t command){
    // Serial.println("cmd2key()");
    switch(command){
        case 25: return IRKeys::k0;
        case 69: return IRKeys::k1;
        case 70: return IRKeys::k2;
        case 71: return IRKeys::k3;
        case 68: return IRKeys::k4;
        case 64: return IRKeys::k5;
        case 67: return IRKeys::k6;
        case 7: return IRKeys::k7;
        case 21: return IRKeys::k8;
        case 9: return IRKeys::k9;
        case 22: return IRKeys::kstar;
        case 13: return IRKeys::khash;
        case 28: return IRKeys::kok;
        case 24: return IRKeys::kup;
        case 82: return IRKeys::kdown;
        case 8: return IRKeys::kleft;
        case 90: return IRKeys::kright;
        case 0: return IRKeys::kNONE;
        default: return IRKeys::kERROR;
    }
} 



char* key2cstr(IRKeys key){
    // Serial.println("key2cstr()");
    switch(key){
        case IRKeys::k0: return k0_str;
        case IRKeys::k1: return k1_str;
        case IRKeys::k2: return k2_str;
        case IRKeys::k3: return k3_str;
        case IRKeys::k4: return k4_str;
        case IRKeys::k5: return k5_str;
        case IRKeys::k6: return k6_str;
        case IRKeys::k7: return k7_str;
        case IRKeys::k8: return k8_str;
        case IRKeys::k9: return k9_str;
        case IRKeys::kstar: return kstar_str;
        case IRKeys::khash: return khash_str;
        case IRKeys::kok: return kok_str;
        case IRKeys::kup: return kup_str;
        case IRKeys::kdown: return kdown_str;
        case IRKeys::kleft: return kleft_str;
        case IRKeys::kright: return kright_str;
        case IRKeys::kNONE: return kNONE_str;
        case IRKeys::kERROR: return kERROR_str;
    }
}


DisplayState CURR_DISPLAY_STATE = DisplayState::LED_INFO;

void display_render_uptime(){
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.printf("uptime: %dms", millis());
}


inline void sep(char c, int n){
    for(int i=0; i<n; ++i){
        Serial.print(c);
    }
    Serial.print('\n');
}

char* clr2cstr(int clr){
    // Serial.println("clr2cstr()");
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


const int HardwareInfoLen = 4;
enum class HardwareInfo{
    cpu_mhz,
    cores,
    ram,
    flash
};


void render_hardware_detail(HardwareInfo hi){
    
    display.setTextColor(SSD1306_WHITE);

    float used = 100.0f * (1.0f - (float)ESP.getFreeHeap() / ESP.getHeapSize());

    switch(hi){
        case HardwareInfo::cpu_mhz:
            display.setTextSize(2);
            display.setCursor(0, 17);
            display.printf("CPU:\n%d MHz\n", getCpuFrequencyMhz());
            break;

        case HardwareInfo::cores:
            display.setTextSize(2);
            display.setCursor(0, 17);
            display.printf("Cores: %d\n", ESP.getChipCores());
            break;

        case HardwareInfo::ram:
            display.setTextSize(1);
            display.setCursor(0, 17);
            display.printf(
                " RAM (bytes)\n Free: %u\n Total: %u\n Used: %.1f%%",
                ESP.getFreeHeap(),
                ESP.getHeapSize(),
                used
            );
            break;
        
        case HardwareInfo::flash:
            display.setTextSize(2);
            display.setCursor(0, 17);
            display.printf(
                "FlashTotal\n%ub\n\n",
                ESP.getFlashChipSize()
            );
            break;
    }

}

void print_system_info(){
    Serial.printf("CPU: %d MHz\n", getCpuFrequencyMhz());
    Serial.printf("Cores: %d\n", ESP.getChipCores());

    Serial.printf(
        "[RAM/bytes] Free: %u, Total: %u\n",
        ESP.getFreeHeap(),
        ESP.getHeapSize()
    );
    float used = 100.0f *
    (1.0f - (float)ESP.getFreeHeap() / ESP.getHeapSize());
    Serial.printf("RAM used: %.1f%%\n", used);  

    Serial.printf(
        "FlashTotal: %u bytes\n\n",
        ESP.getFlashChipSize()
    );
}


void setup()
{   
    pinMode(BUTTON_PIN, INPUT_PULLUP);

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






void display_show_led_info(char* curr_clr, int curr_dur){
    // Serial.println("display_show_led_info()");
    display_render_uptime();

    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 16);
    display.printf("clr %s", curr_clr);

    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 40);
    display.printf("dur %dms", curr_dur);
}


void display_show_ir_info(){
    // Serial.println("display_show_ir_info()");
    display_render_uptime();

    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 16);
    display.printf("key %s", key2cstr(LAST_PRESSED_KEY));

    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 40);
    display.printf("TS %ds", LAST_PRESSED_KEY_TS);
}


void display_show_button_info(){
    display.setTextSize(3);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 20);
    display.println("BTN :3");
}


void display_show_static(){
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(4, 24);
    display.println("Lambda :3");
}


void refresh_display(char* curr_clr, int curr_dur){
    // Serial.println("refresh_display()");
    display.clearDisplay();

    if (digitalRead(BUTTON_PIN) == LOW) {
        Serial.println("Button is pressed");
        display_show_button_info();
        display.display();
        return;
    }

    switch (CURR_DISPLAY_STATE) {
        case DisplayState::LED_INFO:
            display_show_led_info(curr_clr, curr_dur);
            break;

        case DisplayState::IR_INFO:
            display_show_ir_info();
            break;

        case DisplayState::STATIC:
            display_show_static();
            break;

    }
    
    display.display();
}


void led_cycle(){
    if(millis() - EVENT_STARTED_AT < EVENTS[EVENT_IX].dur){
        return;
    }

    // Serial.println("    led_cycle()");
    digitalWrite(EVENTS[EVENT_IX].color, LOW);
    Serial.printf("    KILLED %s\n", clr2cstr(EVENTS[EVENT_IX].color));
    EVENT_IX = (EVENT_IX + 1) % CYCLE_LEN;
    digitalWrite(EVENTS[EVENT_IX].color, HIGH);
    EVENT_STARTED_AT = millis();
    Serial.printf("    STARTED %s\n", clr2cstr(EVENTS[EVENT_IX].color));
    Serial.printf("    SLEEP FOR %d\n\n", EVENTS[EVENT_IX].dur);    
}


IRKeys get_key_from_ir(){
    IRKeys pressed_key = IRKeys::kNONE;

    if (IrReceiver.decode()) {
        Serial.print("IRCommand: ");
        Serial.println(IrReceiver.decodedIRData.command);
        Serial.print("IRCommand_str: ");
        pressed_key = cmd2key(IrReceiver.decodedIRData.command);
        Serial.println(key2cstr(pressed_key));
        Serial.println();

        IrReceiver.resume();
    }

    return pressed_key;
}


bool list_hardware_handle_key(IRKeys pressed_key, HardwareInfo &choosen_state){
    /*
    exit if list_hardware_handle_key() == true;
    */

    switch(pressed_key){
        case IRKeys::kleft:
            if(static_cast<int>(choosen_state) == 0){
                choosen_state = static_cast<HardwareInfo>(
                    HardwareInfoLen - 1
                );
            }
            else{
                choosen_state = static_cast<HardwareInfo>(
                    static_cast<int>(choosen_state) - 1
                );
            }
            break;

        case IRKeys::kright:
            choosen_state = static_cast<HardwareInfo>(
                (static_cast<int>(choosen_state) + 1) % HardwareInfoLen
            );
            break;

        case IRKeys::kok:
            return true;
            break;

        default:
            break;
    }

    return false;
}


void render_hardware_nav_hint(){
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 55);
    display.println("use L/R/OK 4 nav");
}

void start_list_hardware_info(){
    HardwareInfo choosen_state = HardwareInfo::ram;

    Serial.println("start_list_hardware_info()");

    while(1){
        display.clearDisplay();

        display_render_uptime();

        render_hardware_detail(choosen_state);

        render_hardware_nav_hint();

        display.display();

        IRKeys pressed_key = get_key_from_ir();
        
        bool pressed_ok = list_hardware_handle_key(pressed_key, choosen_state);

        if(pressed_ok){
            return;
        }

        delay(LOOP_SLEEP_TIME_MS);
    }
}


bool choose_display_state_handle_key(IRKeys pressed_key, DisplayState& choosen_state){
    /*
    exit if choose_display_state_handle_key() == true;
    */

    switch(pressed_key){
        case IRKeys::kup:
            if(static_cast<int>(choosen_state) == 0){
                choosen_state = static_cast<DisplayState>(
                    CNT_DISPLAY_STATES - 1
                );
            }
            else{
                choosen_state = static_cast<DisplayState>(
                    static_cast<int>(choosen_state) - 1
                );
            }
            break;

        case IRKeys::kdown:
            choosen_state = static_cast<DisplayState>(
                (static_cast<int>(choosen_state) + 1) % CNT_DISPLAY_STATES
            );
            break;

        case IRKeys::kok:
            CURR_DISPLAY_STATE = choosen_state;
            return true;
            break;

        default:
            break;
    }

    return false;
}


void start_choose_display_state_dialog(){
    DisplayState choosen_state = CURR_DISPLAY_STATE;
    int menu_offset = 20;
    int menu_line_size = 10;

    Serial.println("start_choose_display_state_dialog()");

    while(1){
        display.clearDisplay();

        display_render_uptime();

        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);

        for(int i=0; i<CNT_DISPLAY_STATES; ++i){
            display.setCursor(0, menu_offset + i * menu_line_size);
            if(i == static_cast<int>(choosen_state)){
                display.print(" ->");
            }
            display.printf(" %s", DisplayState2cstr(static_cast<DisplayState>(i)));
        }

        display.display();

        IRKeys pressed_key = get_key_from_ir();
        
        bool pressed_ok = choose_display_state_handle_key(pressed_key, choosen_state);

        if(pressed_ok){
            return;
        }

        delay(LOOP_SLEEP_TIME_MS);
    }
}


void handle_key_press(IRKeys key){
    // Serial.println("handle_key_press()");
    switch (key) {
        case IRKeys::kNONE:
            return;
        case IRKeys::khash:
            print_system_info();
            start_list_hardware_info();
            break;
        case IRKeys::kstar:
            start_choose_display_state_dialog();
            break;
    }

    LAST_PRESSED_KEY = key;
    LAST_PRESSED_KEY_TS = millis() / 1000; 
}


void loop()
{    
    led_cycle();

    IRKeys pressed_key = get_key_from_ir();
    handle_key_press(pressed_key);

    refresh_display(
        clr2cstr(EVENTS[EVENT_IX].color),
        EVENTS[EVENT_IX].dur
    );

    delay(LOOP_SLEEP_TIME_MS);
}