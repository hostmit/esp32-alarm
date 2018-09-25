#ifndef OTA_H
#define OTA_H

#include <Arduino.h>
#include <ArduinoOTA.h>
#include "config.h"
#include "logger.h"

class Ota
{
  public:
    static void setup()
    {
        ArduinoOTA.onStart([]() {
            Logger::syslog(F("\n --- OTA update started ---\n"));
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
                type = F("sketch");
            else // U_SPIFFS
                type = F("filesystem");
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            OtaProgress = progress / (total / 100);
            if (OtaProgressPrev != OtaProgress && OtaProgress % 10 == 0)
            {
                Logger::syslog("OTA Progress: " + String(OtaProgress));
                OtaProgressPrev = OtaProgress;
                builtInLed.invert();
            }
        });

        ArduinoOTA.onError([](ota_error_t error) {
            Logger::syslog("OTA Error: " + String(error));
            if (error == OTA_AUTH_ERROR)
                Logger::syslog("Auth Failed");
            else if (error == OTA_BEGIN_ERROR)
                Logger::syslog("Begin Failed");
            else if (error == OTA_CONNECT_ERROR)
                Logger::syslog("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR)
                Logger::syslog("Receive Failed");
            else if (error == OTA_END_ERROR)
                Logger::syslog("End Failed");
        });

        ArduinoOTA.begin();
    }

    static void handle() {
        ArduinoOTA.handle();
    }
};

#endif