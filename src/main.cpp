#include <Arduino.h>
#include <ArduinoTapTempo.h>
#include <SPI.h>


// ============================================================================
// CONFIGURATION
// ============================================================================
#define DEBUG_SERIAL                        // comment to disable serial debug
#define PIN_DIGIPOT_CS          (0)         // MCP41xxx: pin 1
#define PIN_DIGIPOT_SCLK        (1)         // MCP41xxx: pin 2
#define PIN_DIGIPOT_SI          (2)         // MCP41xxx: pin 3
#define PIN_LED                 (3)         // output: blinking tap led
#define PIN_TAP                 (4)         // input: tap tempo footswitch
#define TAP_LED_DURATION_MS     (20)
#define PIN_POT                 (PIN_A6)    // input: delay pot wiper
#define MIN_VALUE_MS            (20)
#define MAX_VALUE_MS            (1000)
#define DIGIPOT_ADDRESS         (0x11)
#define LONGPRESS_MS            (1000)
#define DIGIPOT_OHMS            (100000)


// ============================================================================
// DEBUG
// ============================================================================
#ifdef DEBUG_SERIAL
#define dinit(x)            Serial.begin(x)
#define dprint(x)           Serial.print(x)
#define dprintln(x)         Serial.println(x)
#else
#define dinit(x)
#define dprint(x)
#define dprintln(x)
#endif


// ============================================================================
// GLOBALS
// ============================================================================
ArduinoTapTempo gTap;
int gPrevPotVal, gPotVal;
int gBeatMS, gPrevBeatMS;
bool gShowTempo;
bool tap_last;
long gTimestamp = 0;


// Delay msecs = (11.46 * Resistance KΩ) + 29.70
inline void digipot_write(uint8_t aByte) {
    dprint(F("pot: "));
    dprintln(aByte);
    digitalWrite(PIN_DIGIPOT_CS, LOW);
    SPI.transfer(DIGIPOT_ADDRESS);
    SPI.transfer(aByte);
    digitalWrite(PIN_DIGIPOT_CS, HIGH);
}


// write to digipot from analog pot read value
inline void digipot_write_pot(int aValue) {
    digipot_write(map(aValue, 0, 1024, 0, 0xff));
}


// write to digipot from tapped beat
inline void digipot_write_tap(int aBeatMS) {
    // for PT2399:
    // beat in millisecs = (11.46 * Resistance in KΩ) + 29.70
    // RKΩ = (beat_ms - 29.7) / 11.46
    // RΩ = 1000 * ((beat_ms - 29.7) / 11.46)
    float ohms = (aBeatMS - 29.7) / 11.46;
    ohms *= 1000;
    dprint(F("ohms: "));
    dprintln(ohms);
    uint8_t b = map((long)ohms, 0, DIGIPOT_OHMS, 0, 0xff);
    digipot_write(b);
}


void setup() {                
    dinit(9600);

    // digipot (MCP41xxx)
    pinMode(PIN_DIGIPOT_CS, OUTPUT);
    pinMode(PIN_DIGIPOT_SCLK, OUTPUT);
    pinMode(PIN_DIGIPOT_SI, OUTPUT);
    SPI.begin();

    pinMode(PIN_LED, OUTPUT);   
    pinMode(PIN_TAP, INPUT_PULLUP);
    pinMode(PIN_POT, INPUT);

    digitalWrite(PIN_LED, 0);

    gPotVal = gPrevPotVal = analogRead(PIN_POT);
    gBeatMS = gPrevBeatMS = 0;

    gShowTempo = false;

    gTap.setMaxBeatLengthMS(MAX_VALUE_MS);
    gTap.setMinBeatLengthMS(MIN_VALUE_MS);

    // set default value
    digipot_write_pot(gPotVal);
}


void loop() {

    gTap.update(!digitalRead(PIN_TAP));

    // turn on tempo LED from tap button, for TAP_LED_DURATION_MS
    if(gTap.onBeat()) {
        gShowTempo = true;
        gTimestamp = millis(); 
    }
    if(gShowTempo) {
        if(millis() - gTimestamp < TAP_LED_DURATION_MS) {
            digitalWrite(PIN_LED, 1);
        }
        else {
            gShowTempo = false;
            digitalWrite(PIN_LED, 0);
        }
    }

  
    // read input pot value (w/ basic filtering)
    gPotVal = analogRead(PIN_POT);
    if(abs(gPrevPotVal - gPotVal) > 2) {
        gPrevPotVal = gPotVal;
        tap_last = false;
        digipot_write_pot(gPotVal);
    }


    gBeatMS = gTap.getBeatLength();
    if(gPrevBeatMS - gBeatMS != 0) {
        gPrevBeatMS = gBeatMS;
        digipot_write_tap(gBeatMS);
    }  
}
