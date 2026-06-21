#include <Arduino.h>

#define TdsSensorPin 35     
#define TDS_RELAY_PIN 25    
#define VREF 3.3            
#define SCOUNT 30

const int TDS_THRESHOLD = 40; 

int analogBuffer[SCOUNT];
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;

float averageVoltage = 0;
float tdsValue = 0;
float temperature = 25.0;    
int getMedianNum(int bArray[], int iFilterLen) {
    int bTab[iFilterLen];
    for (int i = 0; i < iFilterLen; i++) {
        bTab[i] = bArray[i];
    }
    int bTemp;
    for (int j = 0; j < iFilterLen - 1; j++) {
        for (int i = 0; i < iFilterLen - j - 1; i++) {
            if (bTab[i] > bTab[i + 1]) {
                bTemp = bTab[i];
                bTab[i] = bTab[i + 1];
                bTab[i + 1] = bTemp;
            }
        }
    }
    if (iFilterLen & 1) {
        bTemp = bTab[(iFilterLen - 1) / 2];
    } else {
        bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
    }
    return bTemp;
}

void setup() {
    Serial.begin(115200);

    pinMode(TdsSensorPin, INPUT);
    pinMode(TDS_RELAY_PIN, OUTPUT);

    digitalWrite(TDS_RELAY_PIN, HIGH); 

    analogReadResolution(12);  
}

void loop() {
    static unsigned long analogSampleTimepoint = millis();

    if (millis() - analogSampleTimepoint > 40U) {
        analogSampleTimepoint = millis();
        analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);
        analogBufferIndex++;
        if (analogBufferIndex >= SCOUNT) {
            analogBufferIndex = 0;
        }
    }

    static unsigned long printTimepoint = millis();
    if (millis() - printTimepoint > 800U) {
        printTimepoint = millis();

        for (int i = 0; i < SCOUNT; i++) {
            analogBufferTemp[i] = analogBuffer[i];
        }

        averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * VREF / 4095.0;

        float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
        float compensationVoltage = averageVoltage / compensationCoefficient;

        tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage
                    - 255.86 * compensationVoltage * compensationVoltage
                    + 857.39 * compensationVoltage) * 0.5;

        Serial.print("Voltage: ");
        Serial.print(averageVoltage, 3);
        Serial.print(" V | TDS Level: ");
        Serial.print(tdsValue, 0);
        Serial.print(" ppm");

        if (tdsValue >= TDS_THRESHOLD) {
            digitalWrite(TDS_RELAY_PIN, LOW); 
            Serial.println(" -> [ALERT] HIGH DISSOLVED SOLIDS! Water Pump/Filter is NOW ON.");
        } else {
            digitalWrite(TDS_RELAY_PIN, HIGH);  
            Serial.println(" -> [STATUS] Water mineral levels are normal. Relay is OFF.");
        }
    }
}