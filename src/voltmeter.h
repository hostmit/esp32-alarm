#ifndef VOLTMETER_H
#define VOLTMETER_H

#include <driver/adc.h>
#include <map>
#include <vector>
#include <Arduino.h>
#include "config.h"
#include "logger.h"


class Voltmeter
{
  public:
    static std::vector<float> readings;
    static long readLastTime;
    static const double coeff;
    static bool notifiedAdcOff;
    static bool notifiedAdcOn;
    static bool notifiedBatteryLow;
    static float adcOffV;
    static float batteryLowV;
    static long lastCheckTime;
    static adc1_channel_t channel;

    static void readConf()
    {
        int gpio = config["batteryGpio"].toInt();
        adcOffV = config["batteryAdcOffV"].toFloat();
        batteryLowV = config["batteryLowV"].toFloat();
        
        switch (gpio)
        {
        case 36:
        {
            channel = ADC1_CHANNEL_0;
            break;
        }
        case 39:
        {
            channel = ADC1_CHANNEL_3;
            break;
        }
        case 34:
        {
            channel = ADC1_CHANNEL_6;
            break;
        }
        case 35:
        {
            channel = ADC1_CHANNEL_7;
            break;
        }
        case 32:
        {
            channel = ADC1_CHANNEL_4;
            break;
        }
        case 33:
        {
            channel = ADC1_CHANNEL_5;
            break;
        }
        default:
            return;
        }

        adc1_config_width(ADC_WIDTH_12Bit);
        adc1_config_channel_atten(channel, ADC_ATTEN_DB_0);
        notifiedAdcOff = false;
        notifiedAdcOn = true;
        notifiedBatteryLow = false;
        
    }

    static void handle()
    {
        if (millis() - readLastTime > VOLTMETERREADINTERVAL)
        {
            while (readings.size() != 0 && readings.size() >= VOLTMETERPOOLSIZE)
                readings.erase(readings.begin());
            readings.push_back(adc1_get_raw(channel) * coeff * config["batteryVCoff"].toFloat());
            readLastTime = millis();
        }

        if ((adcOffV != 0 || batteryLowV != 0) && readings.size() == VOLTMETERPOOLSIZE && millis() - lastCheckTime > VOLTMETERCHECKINTERVAL)
        {
            lastCheckTime=millis();
            if (adcOffV != 0 && notifiedAdcOff != true && get() < adcOffV)
            {
                for (auto &user : Alarm::userM)
                {
                    if (user.second.telegram != "")
                    {
                        mqttQueueV.push_back({"External Power went OFF", user.second.telegram, Timetools::getUnixTime(), -1, false});
                    }
                }
                notifiedAdcOn = false;
                notifiedAdcOff = true;
                Logger::syslog("External Power went OFF");
                
            }
            if (adcOffV != 0 && notifiedAdcOn != true && get() > adcOffV)
            {
                for (auto &user : Alarm::userM)
                {
                    if (user.second.telegram != "")
                    {
                        mqttQueueV.push_back({"External Power went ON", user.second.telegram, Timetools::getUnixTime(), -1, false});
                    }
                }
                notifiedAdcOn = true;
                notifiedAdcOff = false;
                Logger::syslog("External Power went ON");
            }
            if (batteryLowV != 0 && notifiedBatteryLow != true && get() < batteryLowV)
            {
                for (auto &user : Alarm::userM)
                {
                    if (user.second.telegram != "")
                    {
                        mqttQueueV.push_back({"Battery LOW", user.second.telegram, Timetools::getUnixTime(), -1, false});
                    }
                }
                Logger::syslog("Battery LOW");
                notifiedBatteryLow = true;
            }

            if (batteryLowV != 0 && notifiedBatteryLow == true && get() > batteryLowV)
            {
                Logger::syslog("Battery Ok");
                notifiedBatteryLow = false;
            }

        }
    }
    static float get()
    {
        if (readings.size()==0) return -1;
        float result = 0;
        for (auto &value : readings)
        {
            result += value;
        }

        return result / readings.size();
    }

};

std::vector<float> Voltmeter::readings;
long Voltmeter::readLastTime;
bool Voltmeter::notifiedAdcOff;
bool Voltmeter::notifiedAdcOn;
bool Voltmeter::notifiedBatteryLow;
float Voltmeter::adcOffV;
float Voltmeter::batteryLowV;
long Voltmeter::lastCheckTime;
adc1_channel_t Voltmeter::channel;
const double Voltmeter::coeff = 1.1 / 4095 * 20;
#endif