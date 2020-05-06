#include <Arduino.h>

/*
    Name:       HomeAutomation_WaterTank.ino
    Created:    05-05-2020 20:07:34
    Author:     DESKTOP-C6474B6\Abhijit Chavan
*/


#include <NewPing.h>
#include <ESP8266WiFi.h>

#define TRIGGER_PIN  13  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     12  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

#define RELAY_PIN 14
#define WIFI_LED_PIN 13

int mainLoopDelay = 1500;

// Wifi parameters
const char* ssid = "TP-LINK_8FD0";
const char* password = "largeumbrella7";

//Tank level parameter
const int tankEmpty = 0;
const int tankUnused = 10;
const int tankMinThreshold = tankUnused + 10;
const int tankFull = 100;
const int tankOverflow = 102;
const int tankMax = 115;
const int tankRangeMax = 90;
const int tankRangeMin = 30;
const int tankReadingAvg = 10;

//Tank Sonar variables
int sonarValLast = 0;
int sonarValCurr = 0;
int changeLimit = 10;
int tankLevelPerc = 0;


//Motor Relay parameter
const int motorCoolingPeriodMins = 2;
const int motorMinRunTimeSecs = 30;


//Motor Relay variables
int relayState = 0;
long motorStartMillis = 0;
long motorStopMillis = 0;

const int indexLimit = 10;
int readings[indexLimit];
int indexNum = 0;
//int prevIndex = indexLimit - 1;
int prevIndexReading = 0;
int totalVal = 0;
int avgVal = 0;

long loopStart = 0;


NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

void setMotorAction(int overrideParameters);
int getWaterLevel();
void getTankValuesMapping();
void connectWifi();

void setup()
    {
        Serial.begin(115200); // Open serial monitor at 115200 baud to see ping results.
        Serial.println();

        connectWifi();

        pinMode(RELAY_PIN, OUTPUT);
        pinMode(WIFI_LED_PIN, OUTPUT);

    }

void loop()
    {
        loopStart = millis();

        //Get Tank levels
        avgVal = getWaterLevel();
        tankLevelPerc = map(avgVal, tankRangeMax, tankRangeMin, tankEmpty, tankMax);

        setMotorAction(0);

        //Serial.print("millis = ");
        //Serial.print(millis());
        //Serial.print("; motorStopMillis = ");
        //Serial.print(motorStopMillis);
        //Serial.print("; EvalStart = ");
        //Serial.print(motorStopMillis + (motorCoolingPeriodMins * 60000));
        //Serial.print("; motorStartMillis = ");
        //Serial.print(motorStartMillis);
        //Serial.print("; EvalStop = ");
        //Serial.print(motorStartMillis + (motorMinRunTimeSecs * 1000));
        //Serial.print("; sonarValCurr = ");
        //Serial.print(sonarValCurr);
        //Serial.print("; tankLevelPerc = ");
        //Serial.print(tankLevelPerc);
        //Serial.print("; relayState = ");
        //Serial.println(relayState);

        indexNum = indexNum + 1;
        if (indexNum == indexLimit)
            {
                indexNum = 0;
            }

        delay(mainLoopDelay);
    }

void setMotorAction(int overrideParameters)
    {
        if (    tankLevelPerc <= tankMinThreshold & relayState == 0
                &   (   motorStopMillis + (motorCoolingPeriodMins * 60000) < millis()
                        || overrideParameters == 1
                    )
           )
            {
                Serial.println("ON");
                digitalWrite(14, HIGH);
                relayState = 1;
                motorStartMillis = millis();
            }
        else if (   tankLevelPerc >= tankFull & relayState == 1
                    &   (   motorStartMillis + (motorMinRunTimeSecs * 1000) < millis()
                            || overrideParameters == 1
                        )
                )
            {
                Serial.println("OFF");
                digitalWrite(14, LOW);
                relayState = 0;
                motorStopMillis = millis();
            }

        if (motorStartMillis > millis())
            {
                motorStartMillis = motorStartMillis - 4294967295;
            }

        if (motorStopMillis > millis())
            {
                motorStopMillis = motorStopMillis - 4294967295;
            }
    }

//Get the range from JSN-SR04T v2.0 in CMs
int getWaterLevel()
    {
        sonarValLast = sonarValCurr;
        sonarValCurr = sonar.ping_cm(); //Get range in cm

        if (sonarValCurr == 0)  //
            {
                sonarValCurr = sonarValLast;
            }
        else if (abs(sonarValCurr - sonarValLast) > changeLimit)
            {
                if (sonarValCurr < sonarValLast)
                    {
                        sonarValCurr = sonarValLast - changeLimit;
                    }
                else
                    {
                        sonarValCurr = sonarValLast + changeLimit;
                    }
            }

        prevIndexReading = readings[indexNum];
        readings[indexNum] = sonarValCurr;

        totalVal = totalVal + (readings[indexNum] - prevIndexReading);
        return totalVal / tankReadingAvg;
        //return sonarValCurr;
    }

void getTankValuesMapping()
    {
        Serial.println("Tank values mapping (Range, Perc):");

        for (int i = tankRangeMax; i >= tankRangeMin; i--)
            {
                Serial.print(i);
                Serial.print(",");
                Serial.println(map(i, tankRangeMax, tankRangeMin, tankEmpty, tankMax));
            }
    }

void connectWifi()
    {
        Serial.print("Connecting wifi - ");
        Serial.println(ssid);

        WiFi.begin(ssid, password);

        while (WiFi.status() != WL_CONNECTED)
            {
                digitalWrite(WIFI_LED_PIN, HIGH);
                delay(200);
                digitalWrite(WIFI_LED_PIN, LOW);
                delay(300);
                Serial.print(".");
            }

        Serial.print("Connected to WiFi with LocalIP - ");
        Serial.println(WiFi.localIP());
    }
