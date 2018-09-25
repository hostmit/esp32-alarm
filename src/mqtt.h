#ifndef MQTT_H
#define MQTT_H

#include <AsyncMqttClient.h>
#include <WiFi.h>
#include "config.h"
#include "logger.h"
#include "timetools.h"

class Mqtt
{
  public:
    static AsyncMqttClient mqttClient;
    static TimerHandle_t mqttReconnectTimer;
    static TimerHandle_t wifiReconnectTimer;
    static bool started;
    static bool connected;

    static void setup()
    {
        if (config["mqttHost"]=="" || config["mqttPort"]=="" || config["mqttInTopic"]=="" || config["mqttOutTopic"]=="") {
            Logger::syslog("mqtt config is not complete. Will not start mqtt");
            return;
        }
        started = true;
        
        mqttClient.onConnect(onMqttConnect);
        mqttClient.onDisconnect(onMqttDisconnect);
        mqttClient.onSubscribe(onMqttSubscribe);
        mqttClient.onUnsubscribe(onMqttUnsubscribe);
        mqttClient.onMessage(onMqttMessage);
        mqttClient.onPublish(onMqttPublish);
        mqttClient.setCredentials(config["mqttUser"].c_str(), config["mqttPassword"].c_str());
        mqttClient.setServer(config["mqttHost"].c_str(), config["mqttPort"].toInt());
    }

    static void onMqttConnect(bool sessionPresent)
    {
        Logger::syslog("Connected to MQTT.");
        Logger::syslog("Session present: " + String(sessionPresent));
        uint16_t packetIdSub = mqttClient.subscribe(config["mqttInTopic"].c_str(), MQTTINTOPICQOS);
        Logger::syslog("Subscribing to " + config["mqttInTopic"] + " QoS " + String(MQTTINTOPICQOS) + ", packetId: " + String(packetIdSub));
        connected = true;
    }

    static void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
    {
        connected=false;
        Logger::syslog("Disconnected from MQTT.");
    }

    static void onMqttSubscribe(uint16_t packetId, uint8_t qos)
    {
        Logger::syslog(__func__);
        Logger::syslog("Subscribe acknowledged.");
    }

    static void onMqttUnsubscribe(uint16_t packetId)
    {
        Logger::syslog("Unsubscribe acknowledged. packetId: " + String(packetId));
    }

    static void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
    {
        Logger::syslog("Publish received. topic: " + String(topic) + " qos: " + String(properties.qos) + " payload: " + String(payload));
        tgParseMsg(String(payload));
    }

    static void onMqttPublish(uint16_t packetId)
    {
        
        Logger::syslog("Publish acknowledged. packetId: " + String(packetId));
        for (auto &mqttQ : mqttQueueV) {
            if (mqttQ.packetId==packetId) {
                mqttQ.delivered=true;
                Logger::syslog("Setting delivered=true, packetId: " + String (packetId));
                return;
            }
        }
    }

    static uint16_t tgPublish(String text, String chatid, int qos = MQTTOUTTOPICQOS)
    {
        String msg;
        Logger::syslog(__func__);
        DynamicJsonBuffer jsonBuffer;
        JsonObject &root = jsonBuffer.createObject();
        root["text"] = text;
        root["chatid"] = chatid;
        root["date"] = Timetools::getUnixTime();
        root.printTo(msg);
        return mqttClient.publish(config["mqttOutTopic"].c_str(), qos, false, msg.c_str());
    }

    static void tgParseMsg(String msg)
    {

        Logger::syslog(__func__);
        Logger::syslog("Parsing msg: " + msg);
        DynamicJsonBuffer jsonBuffer;
        JsonObject &root = jsonBuffer.parseObject(msg);
        if (!root.success() || !root.containsKey("chatid") || !root.containsKey("date") || !root.containsKey("text"))
        {
            Logger::syslog("Unable to parse this message");
            return;
        }
        String chatid = root["chatid"];
        int date = root["date"];
        String text = root["text"];
        if (Alarm::userIdByTelegram(chatid) == -1)
        {
            tgPublish("You are not authorized", chatid);
            return;
        }
        if (!Timetools::updated)
        {
            tgPublish("I dont have RTC set, it's unsafe to act on possibly outdated commands.", chatid);
            return;
        }

        if ((Timetools::getUnixTime() - date) > TGMAXMSGDELAYSEC)
        {
            tgPublish("I got your message too late, max delay = " + String(TGMAXMSGDELAYSEC), chatid);
            return;
        }

        String response = "";
        if (text == "/status")
        {
            response += Alarm::zoneStatus();
            response += Alarm::pinOutStatus();
            tgPublish(response, chatid);
            return;
        }
        else if (text.substring(0, 4) == "/arm" || text.substring(0, 7) == "/disarm")
        {
            String part1;
            String part2;
            if (text.substring(0, 4) == "/arm")
            {
                part1 = "/arm";
                part2 = text.substring(4, text.length());
            }
            else
            {
                part1 = "/disarm";
                part2 = text.substring(7, text.length());
            }
            part2.replace(" ", "");
            if (part2.length() == 0)
            {
                part1 == "/arm" ? Alarm::zoneChangeState(true) : Alarm::zoneChangeState(false);
            }
            else
            {
                part1 == "/arm" ? Alarm::zoneChangeState(true, part2.toInt()) : Alarm::zoneChangeState(false, part2.toInt());
            }
            tgPublish(Alarm::zoneChangeStateMsg, chatid);
        }
        else if (text=="/uptime") {
            String t;
            t += "Uptime: " + Timetools::formattedInterval(millis()) + "\n";
            t += "wifiConnectTime: " + Timetools::formattedInterval(millis() - wifiConnectTime) + ", ";
            t += "wifiConnectCount: " + String(wifiConnectCount) + "\n";
            tgPublish(t, chatid);

        }
        else
        {
            tgPublish("Uknown command", chatid);
        }
    }

    static void handleQueue() {
        if (!Mqtt::started || !Mqtt::connected) return;
        if (mqttQueueV.size()!=0) {
            if (mqttQueueV.size() > MQTTQUEUESIZE) {
                Logger::syslog("mqttQueueV.size() over limit " + String(MQTTQUEUESIZE) + ", this should not happen");
                while(mqttQueueV.size()>MQTTQUEUESIZE) mqttQueueV.erase(mqttQueueV.begin());
            }
            if (millis()-mqttHandleLastAttempt>MQTTHANDLEDELAY) {
                Logger::syslog(__func__);
                while(mqttQueueV.size()!=0 && mqttQueueV.begin()->delivered==true) mqttQueueV.erase(mqttQueueV.begin());
                if (mqttQueueV.size()==0) return;
                String msg;
                DynamicJsonBuffer jsonBuffer;
                JsonObject &root = jsonBuffer.createObject();
                root["text"] = mqttQueueV.begin()->text;
                root["chatid"] = mqttQueueV.begin()->chatid;
                root["date"] = mqttQueueV.begin()->date;
                root.printTo(msg);
                mqttQueueV.begin()->packetId == -1 || mqttQueueV.begin()->packetId == 0
                    ? mqttQueueV.begin()->packetId = mqttClient.publish(config["mqttOutTopic"].c_str(), 2, false, msg.c_str())
                    : mqttClient.publish(config["mqttOutTopic"].c_str(), 2, false, msg.c_str(),msg.length(),false, mqttQueueV.begin()->packetId);
                
                mqttHandleLastAttempt=millis();
            }
        }
    }


};

AsyncMqttClient Mqtt::mqttClient;
bool Mqtt::started = false;
bool Mqtt::connected = false;
#endif