#ifndef SYSINFO_H
#define SYSINFO_H

#include <rom/rtc.h>
#include <Arduino.h>
#include <SPIFFS.h>

class Sysinfo
{
public:
  static long loopCount;
  static long loopStat[LOOPSTATSIZE];
  static long loopLastMillis;

  static String getResetReason()
  {
    String response = "<strong>--- CPU RESET INFO: </strong>\n";
    String r = "";
    RESET_REASON reason;
    for (int i = 0; i < 2; i++)
    {

      reason = rtc_get_reset_reason(i);
      switch (reason)
      {
      case 1:
        r = (F("Vbat power on reset"));
        break;
      case 3:
        r = (F("Software reset digital core"));
        break;
      case 4:
        r = (F("Legacy watch dog reset digital core"));
        break;
      case 5:
        r = (F("Deep Sleep reset digital core"));
        break;
      case 6:
        r = (F("Reset by SLC module, reset digital core"));
        break;
      case 7:
        r = (F("Timer Group0 Watch dog reset digital core"));
        break;
      case 8:
        r = (F("Timer Group1 Watch dog reset digital core"));
        break;
      case 9:
        r = (F("RTC Watch dog Reset digital core"));
        break;
      case 10:
        r = (F("Instrusion tested to reset CPU"));
        break;
      case 11:
        r = (F("Time Group reset CPU"));
        break;
      case 12:
        r = (F("Software reset CPU"));
        break;
      case 13:
        r = (F("RTC Watch dog Reset CPU"));
        break;
      case 14:
        r = (F("for APP CPU, reseted by PRO CPU"));
        break;
      case 15:
        r = (F("Reset when the vdd voltage is not stable"));
        break;
      case 16:
        r = (F("RTC Watch dog reset digital core and rtc module"));
        break;
      default:
        r = (F("NO_MEAN"));
      }
      response += "CPU" + String(i) + " reason: " + r + "\n";
    }
    return response;
  }

  static String getFsInfo()
  {
    String t;
    t += F("<strong>--- FS INFO:</strong>");
    t += F("totalBytes: ");
    t += String(SPIFFS.totalBytes());
    t += F(", usedBytes: ");
    t += String(SPIFFS.usedBytes()) + "\n";
    return t;
  }

  static String getRamInfo()
  {
    String t;
    t += F("<strong>--- RAM: getFreeHeap(): </strong>");
    t += String(ESP.getFreeHeap()) + "\n";
    return t;
  }

  static String getFlashInfo()
  {
    String t;
    t += F("<strong>--- ESP FLASH: </strong>\n");
    t += "getChipRevision: " + String(ESP.getChipRevision()) + "\n";
    t += "getCpuFreqMHz: " + String(ESP.getCpuFreqMHz()) + "\n";
    t += "getCycleCount: " + String(ESP.getCycleCount()) + "\n";
    t += "getEfuseMac: " + String((int)ESP.getEfuseMac(), HEX) + "\n";
    t += "getFlashChipMode: " + String(ESP.getFlashChipMode()) + "\n";
    t += "getFlashChipSize: " + String(ESP.getFlashChipSize()) + "\n";
    t += "getFlashChipSpeed: " + String(ESP.getFlashChipSpeed()) + "\n";
    t += "getSdkVersion: " + String(ESP.getSdkVersion()) + "\n";
    return t;
  }

  static String getAll()
  {
    return getResetReason() + getFsInfo() + getRamInfo() + getFlashInfo();
  }
  static void loopStatHandle()
  {
    loopCount++;
    if (millis() - loopLastMillis > LOOPSTATDURATION)
    {
      std::copy(loopStat, loopStat + LOOPSTATSIZE - 1, loopStat + 1);
      loopStat[0] = loopCount;
      loopLastMillis = millis();
      loopCount = 0;
    }
  }
  static String getLoopStat()
  {
    String t = "";
    t += "<strong>--- LOOP() STATS</strong>. Duration: " + String(LOOPSTATDURATION / 1000) + " sec each---\n";
    int loopAverage = 0;
    for (int i = 0; i < LOOPSTATSIZE; i++)
    {
      t += String(loopStat[i]);
      if (i != LOOPSTATSIZE - 1)
        t += ",";
      loopAverage += loopStat[i];
    }
    t += "\n";
    t += "Average: " + String(round(loopAverage / LOOPSTATSIZE)) + "\n";
    return t;
  }

  static String getWifi(){
    String t = "";
    t += "wifiConnectTime: " + Timetools::formattedInterval(millis() - wifiConnectTime) + ", ";
            t += "wifiConnectCount: " + String(wifiConnectCount) + "\n";
    t += "SSID: " + WiFi.SSID() + ": Station connected, IP: " + WiFi.localIP().toString() + 
        ", WiFi.subnetMask(): " + WiFi.subnetMask().toString() + ", WiFi.gatewayIP(): " + WiFi.gatewayIP().toString() +
        ", WiFi.dnsIP(): " + WiFi.dnsIP().toString() + ", WiFi.getHostname(): "+ WiFi.getHostname() + "\n";
    return t;
  }


};

long Sysinfo::loopCount = 0;
long Sysinfo::loopLastMillis = 0;
long Sysinfo::loopStat[LOOPSTATSIZE];

#endif
