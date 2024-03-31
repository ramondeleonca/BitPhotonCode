#include <Arduino.h>
#include <WiFi.h>

class utils {
    public:
        static void getLEDDigit(int digit, int ledStates[]) {
            if (digit >= 0 && digit <= 15) {
                switch (digit) {
                    case 0:
                        ledStates[0] = LOW;
                        ledStates[1] = LOW;
                        ledStates[2] = LOW;
                        ledStates[3] = LOW;
                        break;
                    case 1:
                        ledStates[0] = HIGH;
                        ledStates[1] = LOW;
                        ledStates[2] = LOW;
                        ledStates[3] = LOW;
                        break;
                    case 2:
                        ledStates[0] = LOW;
                        ledStates[1] = HIGH;
                        ledStates[2] = LOW;
                        ledStates[3] = LOW;
                        break;
                    case 3:
                        ledStates[0] = HIGH;
                        ledStates[1] = HIGH;
                        ledStates[2] = LOW;
                        ledStates[3] = LOW;
                        break;
                    case 4:
                        ledStates[0] = LOW;
                        ledStates[1] = LOW;
                        ledStates[2] = HIGH;
                        ledStates[3] = LOW;
                        break;
                    case 5:
                        ledStates[0] = HIGH;
                        ledStates[1] = LOW;
                        ledStates[2] = HIGH;
                        ledStates[3] = LOW;
                        break;
                    case 6:
                        ledStates[0] = LOW;
                        ledStates[1] = HIGH;
                        ledStates[2] = HIGH;
                        ledStates[3] = LOW;
                        break;
                    case 7:
                        ledStates[0] = HIGH;
                        ledStates[1] = HIGH;
                        ledStates[2] = HIGH;
                        ledStates[3] = LOW;
                        break;
                    case 8:
                        ledStates[0] = LOW;
                        ledStates[1] = LOW;
                        ledStates[2] = LOW;
                        ledStates[3] = HIGH;
                        break;
                    case 9:
                        ledStates[0] = HIGH;
                        ledStates[1] = LOW;
                        ledStates[2] = LOW;
                        ledStates[3] = HIGH;
                        break;
                    case 10:
                        ledStates[0] = LOW;
                        ledStates[1] = HIGH;
                        ledStates[2] = LOW;
                        ledStates[3] = HIGH;
                        break;
                    case 11:
                        ledStates[0] = HIGH;
                        ledStates[1] = HIGH;
                        ledStates[2] = LOW;
                        ledStates[3] = HIGH;
                        break;
                    case 12:
                        ledStates[0] = LOW;
                        ledStates[1] = LOW;
                        ledStates[2] = HIGH;
                        ledStates[3] = HIGH;
                        break;
                    case 13:
                        ledStates[0] = HIGH;
                        ledStates[1] = LOW;
                        ledStates[2] = HIGH;
                        ledStates[3] = HIGH;
                        break;
                    case 14:
                        ledStates[0] = LOW;
                        ledStates[1] = HIGH;
                        ledStates[2] = HIGH;
                        ledStates[3] = HIGH;
                        break;
                    case 15:
                        ledStates[0] = HIGH;
                        ledStates[1] = HIGH;
                        ledStates[2] = HIGH;
                        ledStates[3] = HIGH;
                        break;
                }
            } else {
                // Invalid digit, turn off all LEDs
                ledStates[0] = LOW;
                ledStates[1] = LOW;
                ledStates[2] = LOW;
                ledStates[3] = LOW;
            }
        }

        static void blinkLEDs(int ledStates[], const int ledPins[], int leds, int delayTime, int delayBetween = 0, int times = 1) {
            for (int j = 0; j < leds; j++) {
                pinMode(ledPins[j], OUTPUT);
                digitalWrite(ledPins[j], LOW);
            }
            for (int i = 0; i < times; i++) {
                for (int j = 0; j < leds; j++) {
                    digitalWrite(ledPins[j], HIGH);
                    delay(delayBetween);
                }
                delay(delayTime);
                for (int j = 0; j < 4; j++) {
                    digitalWrite(ledPins[j], LOW);
                    delay(delayBetween);
                }
                delay(delayTime);
            }
            for (int j = 0; j < leds; j++) digitalWrite(ledPins[j], ledStates[j]);
        }
};
