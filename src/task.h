#ifndef TASK_H
#define TASK_H

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "config.h"
#include "logger.h"
#include "mqtt.h"
#include "alarm.h"
#include "web.h"
#include "timetools.h"

class Task
{
  public:
    static TaskHandle_t timeUpdateHandle;
    static TaskHandle_t mqttConnectHandle;
    static TaskHandle_t alarmUpdateRemoteLedHandle;
    static TaskHandle_t restartNotifyHandle;
    static TaskHandle_t pokeRemoteRfidHandle;
    static TaskHandle_t webServerHandle;
    static TaskHandle_t gsmUpdateBalanceHandle;
    static void init()
    {
        xTaskCreate(
            timeUpdateTask,     /* Task function. */
            "updateTimeTask",   /* name of task. */
            10000,              /* Stack size of task */
            NULL,               /* parameter of the task */
            1,                  /* priority of the task */
            &timeUpdateHandle); /* Task handle to keep track of created task */

        xTaskCreate(
            mqttConnectTask,     /* Task function. */
            "mqttConnectTask",   /* name of task. */
            10000,               /* Stack size of task */
            NULL,                /* parameter of the task */
            1,                   /* priority of the task */
            &mqttConnectHandle); /* Task handle to keep track of created task */
        xTaskCreate(
            alarmUpdateRemoteLedTask,     /* Task function. */
            "alarmUpdateRemoteLedTask",   /* name of task. */
            10000,                        /* Stack size of task */
            NULL,                         /* parameter of the task */
            1,                            /* priority of the task */
            &alarmUpdateRemoteLedHandle); /* Task handle to keep track of created task */
        xTaskCreate(
            restartNotifyTask,     /* Task function. */
            "restartNotifyTask",   /* name of task. */
            10000,                 /* Stack size of task */
            NULL,                  /* parameter of the task */
            1,                     /* priority of the task */
            &restartNotifyHandle); /* Task handle to keep track of created task */

        // xTaskCreate(
        //     RemoteRfidPokeTask,     /* Task function. */
        //     "RemoteRfidPokeTask",   /* name of task. */
        //     10000,                  /* Stack size of task */
        //     NULL,                   /* parameter of the task */
        //     1,                      /* priority of the task */
        //     &pokeRemoteRfidHandle); /* Task handle to keep track of created task */
        xTaskCreate(
            webServerTask,     /* Task function. */
            "webServerTask",   /* name of task. */
            10000,             /* Stack size of task */
            NULL,              /* parameter of the task */
            1,                 /* priority of the task */
            &webServerHandle); /* Task handle to keep track of created task */

        xTaskCreate(
            gsmUpdateBalanceTask,     /* Task function. */
            "gsmUpdateBalanceTask",   /* name of task. */
            10000,                    /* Stack size of task */
            NULL,                     /* parameter of the task */
            1,                        /* priority of the task */
            &gsmUpdateBalanceHandle); /* Task handle to keep track of created task */
    }
    static void stopAll()
    {
        // Logger::syslog(__func__);
        // if (eTaskGetState(timeUpdateHandle) == 2)
        //     vTaskDelete(timeUpdateHandle);
        // if (eTaskGetState(mqttConnectHandle) == 2)
        //     vTaskDelete(mqttConnectHandle);
    }
    static String getStateAll()
    {
        String r = "";
        r += "timeUpdateHandle: " + String(eTaskGetState(timeUpdateHandle)) + "\n";
        return r;
    }
    static void timeUpdateTask(void *pvParameters)
    {
        Logger::syslog(__func__);
        while (Timetools::updated != true)
        {
            while (!WiFi.isConnected())
                vTaskDelay(5000 / portTICK_RATE_MS); // 5000 - 10sec, 50000 55 sec

            if (Timetools::update())
            {
                Logger::syslog("timeUpdateTask: Time set via ntp! vTaskDelete");
                vTaskDelete(NULL);
            }
            vTaskDelay(5000 / portTICK_RATE_MS); // 5000 - 10sec, 50000 55 sec
        }
        Logger::syslog("timeUpdateTask: Time already updated, no work for me. vTaskDelete");
        vTaskDelete(NULL);
    }

    static void mqttConnectTask(void *pvParameters)
    {
        Logger::syslog(__func__);

        if (!Mqtt::started)
        {
            Logger::syslog("Mqtt is not started, vTaskDelete on mqttConnectTask");
            vTaskDelete(NULL);
        }
        while (true)
        {
            if (!WiFi.isConnected())
            {
                vTaskDelay(5000 / portTICK_RATE_MS);
                continue;
            }

            if (!Mqtt::mqttClient.connected())
            {
                Mqtt::mqttClient.connect();
                vTaskDelay(30000 / portTICK_RATE_MS);
            }
            else
            {
                vTaskDelay(30000 / portTICK_RATE_MS);
            }
        }
        vTaskDelete(NULL);
    }
    static void alarmUpdateRemoteLedTask(void *pvParameters)
    {
        Logger::syslog(__func__);
        HTTPClient httpUpdater;
        httpUpdater.setTimeout(HTTPUPDATERTIMEOUT);
        httpUpdater.setReuse(HTTPUPDATERREUSE);
        int httpCode;
        int zoneId;
        String url;

        String zoneStrState;
        while (true)
        {
            while (!WiFi.isConnected())
                vTaskDelay(5000 / portTICK_RATE_MS);
            for (auto &ipM : Alarm::alarmUpdateRemoteLedM)
            {
                if (ipM.second == false)
                {
                    zoneId = Alarm::zoneIdByIp(ipM.first);
                    Alarm::zoneM[zoneId].armed ? zoneStrState = "on" : zoneStrState = "off";
                    url = "http://" + ipM.first + "/led/" + zoneStrState;
                    httpUpdater.begin(url);
                    httpCode = httpUpdater.GET();
                    if (httpCode == HTTP_CODE_OK && httpUpdater.getString() == zoneStrState)
                    {
                        ipM.second = true;
                        Logger::syslog("Zone id:" + String(zoneId) + ", name: " + Alarm::zoneM[zoneId].name + ", ip: " + ipM.first + " has been updated with value: " + zoneStrState);
                    }
                    httpUpdater.end();
                    vTaskDelay(200);
                }
            }
            vTaskDelay(500);
        }
        vTaskDelete(NULL);
    }
    static void restartNotifyTask(void *pvParameters)
    {
        Logger::syslog(__func__);
        while (!WiFi.isConnected() || !Timetools::updated)
            vTaskDelay(5000 / portTICK_RATE_MS);
        if (Alarm::userM.size() == 0)
            vTaskDelay(10000);

        for (auto &user : Alarm::userM)
        {
            if (user.second.telegram != "")
                mqttQueueV.push_back({"Device restarted", user.second.telegram, Timetools::getUnixTime(), -1, false});
        }
        Logger::syslog("Restart notification queued, task deleted");
        vTaskDelete(NULL);
    }
    // static void RemoteRfidPokeTask(void *pvParameters)
    // {
    //     Logger::syslog(__func__);
    //     vTaskDelay(60000);
    //     HTTPClient http;
    //     http.setReuse(true);
    //     http.setTimeout(500);
    //     while (true)
    //     {
    //         while (!WiFi.isConnected())
    //             vTaskDelay(5000 / portTICK_RATE_MS);
    //         for (auto &value : Alarm::remoteRfidIpV)
    //         {
    //             http.begin("http://" + value.ip + "/ping");
    //             value.httpCode = http.GET();
    //             http.end();
    //             value.updated = millis();
    //             vTaskDelay(1000);
    //         }

    //         vTaskDelay(120000);
    //     }

    //     vTaskDelete(NULL);
    // }
    static void webServerTask(void *pvParameters)
    {
        Logger::syslog(__func__);
        vTaskDelay(5000);
        while (true)
        {
            Web::handle();
            vTaskDelay(50);
        }
    }

    static void gsmUpdateBalanceTask(void *pvParameters)
    {
        Logger::syslog(__func__);
        vTaskDelay(GSMBALANCEONSTARTDELAY); //1 minute to prepear, no rush
        int cHour;
        int hDelay = 0;
        while (true)
        {
            cHour = Timetools::getHour();
            if (cHour != -1 && (cHour < GSMBALANCENOTIFYHOURSTART || cHour > GSMBALANCENOTIFYHOUREND))
            {
                if (cHour < GSMBALANCENOTIFYHOURSTART)
                {
                    hDelay = GSMBALANCENOTIFYHOURSTART - cHour;
                }
                else if (cHour > GSMBALANCENOTIFYHOUREND)
                {
                    hDelay = GSMBALANCENOTIFYHOURSTART + 24 - GSMBALANCENOTIFYHOUREND;
                }

                Logger::syslog("Will delay balance check till working hours: " + String(GSMBALANCENOTIFYHOURSTART) + "-" + String(GSMBALANCENOTIFYHOUREND) + ". Delay is for: " + String(hDelay) + " hours.");
                vTaskDelay(hDelay * 3600 * 1000);
            }

            if (gsmOperatorM.find(config["gsmOperator"]) == gsmOperatorM.end())
            {
                Logger::syslog("GSM operator:" + config["gsmOperator"] + " has no corresponding instuctions in gsmOperatorM");
                vTaskDelay(GSMBALANCECHECKPERIOD);
                continue;
            }
            gsm_operator_struct operatorS = gsmOperatorM[config["gsmOperator"]];

            Logger::syslog("Starting GSM balance check");
            {
                if (operatorS.cmdPrep != "")
                {
                    Serial2.println(operatorS.cmdPrep);
                    vTaskDelay(GSMBALANCECMDDELAY);
                }
                Gsmserial::balance.lastAttempt = millis();
                Serial2.println(operatorS.cmdBalance);
                vTaskDelay(GSMBALANCECMDDELAY);
                for (auto &value : Gsmserial::serialLogV)
                {
                    if (value.time > Gsmserial::balance.lastAttempt && value.msg.indexOf(operatorS.markStart) != -1 && value.msg.indexOf(operatorS.markEnd) != -1)
                    {
                        String balanceStr = value.msg.substring(value.msg.indexOf(operatorS.markStart) + String(operatorS.markStart).length(), value.msg.indexOf(operatorS.markEnd));
                        Gsmserial::balance.balance = balanceStr.toFloat();
                        Gsmserial::balance.lastResultTime = millis();
                        Logger::syslog("GSM balance keyword met. Balance is: " + String(Gsmserial::balance.balance) + ". Config[gsmMinBalance]: " + String(config["gsmMinBalance"].toFloat()));
                        if (Gsmserial::balance.balance < config["gsmMinBalance"].toFloat())
                        {
                            for (auto &user : Alarm::userM)
                            {
                                if (user.second.telegram != "")
                                {
                                    Logger::syslog("User :" + user.second.name + " notified of low ballance");
                                    mqttQueueV.push_back({"GSM balance too low: " + String(Gsmserial::balance.balance), user.second.telegram, Timetools::getUnixTime(), -1, false});
                                }
                            }
                        }
                        vTaskDelay(GSMBALANCECHECKPERIOD);
                        continue;
                    }
                }
                Logger::syslog("GSM balance keyword is NOT met.");
                for (auto &user : Alarm::userM)
                {
                    if (user.second.telegram != "")
                    {
                        Logger::syslog("User :" + user.second.name + " notified of GSM balance check error.");
                        mqttQueueV.push_back({"Unable to check GSM balance. Is module/sim working?", user.second.telegram, Timetools::getUnixTime(), -1, false});
                    }
                }

                vTaskDelay(GSMBALANCECHECKPERIOD);
            }
        }
    }
    static void udpMustiCastTask(void *pvParameters)
    {
        Logger::syslog(__func__);
        vTaskDelay(5000);
        WiFiUDP Udp;
        IPAddress ipBroadcast(10,10,10,255);
        unsigned int portBroadcast = 4444;
        
        while (true)
        {
            Udp.beginPacket(ipBroadcast, portBroadcast); // subnet Broadcast IP and port
            //Udp.write(4);
            Udp.endPacket();
            vTaskDelay(30000);
            //Logger::syslog("Udp broadcast sent");
        }
    }
};
TaskHandle_t Task::timeUpdateHandle;
TaskHandle_t Task::mqttConnectHandle;
TaskHandle_t Task::alarmUpdateRemoteLedHandle;
TaskHandle_t Task::restartNotifyHandle;
TaskHandle_t Task::pokeRemoteRfidHandle;
TaskHandle_t Task::webServerHandle;
TaskHandle_t Task::gsmUpdateBalanceHandle;
#endif