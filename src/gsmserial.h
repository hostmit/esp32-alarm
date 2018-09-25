#ifndef GSMSERIAL_H
#define GSMSERIAL_H

#include <Arduino.h>
#include "config.h"
#include "logger.h"

class Gsmserial
{
  public:
    static String serial2InData;
    static char serial2char;
    static std::vector<serialLog_struct> serialLogV;
    static balance_struct balance;

    static void handle()
    {
        while (Serial2.available() > 0)
        {
            serial2char = Serial2.read();
            serial2InData += serial2char;
            if (serial2char == '\n')
            {
                serial2InData.trim();
                if (serial2InData.length() == 0)
                    continue;
                while (serialLogV.size() > SERIALLOGMAXSIZE)
                    serialLogV.erase(serialLogV.begin());

                serialLogV.push_back({millis(), serial2InData});
                Logger::syslog("Serial2:" + serial2InData);
                serial2InData = "";
            }
        }
    }
    static String serialLogToString()
    {
        String r;
        for (auto &value : serialLogV)
        {
            r += "millis: " + String(value.time) + " data: " + value.msg + "\n";
        }
        return r;
    }
};

String Gsmserial::serial2InData;
char Gsmserial::serial2char;
std::vector<serialLog_struct> Gsmserial::serialLogV;
balance_struct Gsmserial::balance;

#endif