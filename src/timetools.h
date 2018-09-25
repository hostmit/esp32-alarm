#ifndef TIMETOOLS_H
#define TIMETOOLS_H

#include <time.h>
#include <Arduino.h>
#include "config.h"

class Timetools
{
  public:
    static long lastUpdate;
    static bool updated;
    static long stopWatchStartMark;
    static long stopWatchStopMark;

    //returns unixTime 
    static long getUnixTime()
    {
        time_t now;
        return time(&now);
    }

    //attempts to udpate using ntp
    static bool update()
    {
        configTime(NTPGMTOFFSETSEC, NTPDAYLIGHTOFFSETSEC, NTPSERVER);
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo))
        {
            return false;
        }
        lastUpdate = millis();
        updated = true;
        return true;
    }
    static void stopWatchStart()
    {
        stopWatchStartMark = millis();
    }
    static void stopWatchStop()
    {
        stopWatchStopMark = millis();
    }
    static long stopWatchResult()
    {
        stopWatchStopMark = millis();
        return stopWatchStartMark - stopWatchStopMark;
    }

    static String formattedTime() {
        if (!updated) return "[]";
        time_t now;
        time(&now);
        struct tm *tm = localtime(&now);
        char date[20];
        strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", tm);
        return ("["+String(date)+"] ");
    }
    static int getHour() {
        if (!updated) return -1;
        time_t now;
        time(&now);
        struct tm *tm = localtime(&now);
        char buff[3];
        strftime(buff, sizeof(buff), "%H", tm);
        return (String(buff).toInt());
    }
    static String formattedInterval(long timeArg = 0)
    {
        String text = "";
        long days = 0;
        long hours = 0;
        long mins = 0;
        long secs = 0;
        timeArg == 0 ? secs = millis() / 1000 : secs = timeArg / 1000;
        //secs = millis() / 1000;      //convect milliseconds to seconds
        mins = secs / 60;            //convert seconds to minutes
        hours = mins / 60;           //convert minutes to hours
        days = hours / 24;           //convert hours to days
        secs = secs - (mins * 60);   //subtract the coverted seconds to minutes in order to display 59 secs max
        mins = mins - (hours * 60);  //subtract the coverted minutes to hours in order to display 59 minutes max
        hours = hours - (days * 24); //subtract the coverted hours to days in order to display 23 hours max
        if (days > 0)
            text += String(days) + " days and ";
        if (hours < 10)
            text += "0";
        text += String(hours) + ":";
        if (mins < 10)
            text += "0";
        text += String(mins) + ":";
        if (secs < 10)
            text += "0";
        text += String(secs);
        return text;
    }
};
bool Timetools::updated = false;

long Timetools::lastUpdate;
long stopWatchStartMark;
long Timetools::stopWatchStopMark;
#endif