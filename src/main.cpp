
#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include <map>
//#include <ESPAsyncWebServer.h>
//#include <WebServer.h>
#include "esp_wifi.h" //need this to control wifi power saving mode

#include <AsyncMqttClient.h>
#include "gpioswitch.h" // must be included before config

#include "config.h"
#include "logger.h"
#include "ota.h"
#include "conf.h"
#include "fstools.h"
#include "web.h"
#include "sysinfo.h"
#include "mqtt.h"
#include "alarm.h"
#include "task.h"
#include "gsmserial.h"
#include "voltmeter.h"






void WiFiEvent(WiFiEvent_t event)
{
    switch (event)
    {
    case SYSTEM_EVENT_STA_GOT_IP:
        Logger::syslog(String(__func__) + ": event: " + String(event) + ", WiFi.status(): " + String(WiFi.status()));
        wifiConnectTime = millis();
        wifiConnectCount++;
        Logger::syslog(String(__func__) + ": Station connected, IP: " + WiFi.localIP().toString() +
                       ", WiFi.subnetMask(): " + WiFi.subnetMask().toString() + ", WiFi.gatewayIP(): " + WiFi.gatewayIP().toString() +
                       ", WiFi.dnsIP(): " + WiFi.dnsIP().toString() + ", wifiConnectCount: " + String(wifiConnectCount));
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Logger::syslog(String(__func__) + ": event: " + String(event) + ", WiFi.status(): " + String(WiFi.status()));
        Logger::syslog("Station disconnected. wifiConnectTime: " + Timetools::formattedInterval(millis() - wifiConnectTime));
        WiFi.reconnect();
        break;
    case SYSTEM_EVENT_WIFI_READY:
        Logger::syslog(String(__func__) + ": event: " + String(event) + ", WiFi.status(): " + String(WiFi.status()));
        WiFi.reconnect();
        break;
    case SYSTEM_EVENT_STA_START:
        Logger::syslog(String(__func__) + ": event: " + String(event) + ", WiFi.status(): " + String(WiFi.status()));
        WiFi.reconnect();
        break;
    default:
        Logger::syslog(String(__func__) + ": event: " + String(event) + ", WiFi.status(): " + String(WiFi.status()));
    }
}

void setup()
{
    Serial.begin(115200);
    Serial2.begin(9600);
    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
        Serial.println("Failed SPIFFS.begin()");

    Conf::load();
    Logger::syslog("Config data: \n" + Conf::getAsSting() + "End of config data \n");
    Conf::updateStartNumber();
    WiFi.onEvent(WiFiEvent);
    WiFi.mode(WIFI_STA);
    WiFi.begin(config["wifiSsid"].c_str(), config["wifiPassword"].c_str());
    esp_wifi_set_ps(WIFI_PS_NONE);

    Timetools::update();
    Web::setup();
    Ota::setup();
    Mqtt::setup();
    Alarm::load();
    Alarm::zoneLoadState();
    Alarm::zoneUpdateRemoteLed();
    Task::init();
    //Voltmeter(config["batteryGpio"].toInt(),config["batteryAdcOffV"].toFloat(),config["batteryLowV"].toFloat());
    Voltmeter::readConf();
    builtInLed.start(-1, 50);
    
       
}

void loop()
{
    if (espRestartRequest)
    {
        Logger::syslog("espRestartRequest request received by loop(), will reset after delay:" + String(ESPRESTARTDELAY));
        delay(ESPRESTARTDELAY);
        esp_restart();
    }

    Ota::handle();
    Gpioswitch::handleGlobal();
    Sysinfo::loopStatHandle();
    Alarm::alarmHandle();
    Mqtt::handleQueue();
    Gsmserial::handle();
    Voltmeter::handle();

 


}