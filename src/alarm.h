#ifndef ALARM_H
#define ALARM_H

#include <map>
#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <vector>
#include "config.h"
#include "logger.h"
#include "gsmserial.h"
#include "gpioswitch.h"

class Alarm
{
  public:
    static std::map<int, user_struct> userM;
    static std::map<int, pin_struct> pinM;
    static std::map<int, zone_struct> zoneM;
    static String webErr;
    static String zoneChangeStateMsg;
    static bool alarmWorking;
    static bool alarmStartLogged;
    static String alarmTriggerMsg;
    static long alarmStartTime;
    static void IRAM_ATTR pinHandleInterrupt(void *arg);
    static std::vector<callQueue_struct> callQueueV;
    static long callNextEvent;
    static std::vector<Gpioswitch> alarmSirenV;
    static long alarmBeepEndTime;
    static std::map<String, bool> alarmUpdateRemoteLedM;
    static std::vector<remote_rfid_struct> remoteRfidIpV;

    /*---------------------- USER ----------------------*/
    static bool userLoad()
    {
        Logger::syslog(__func__);
        File file = SPIFFS.open(USERCONFFILE, "r");
        DynamicJsonBuffer arrJsonBuffer;
        JsonArray &jsonarray = arrJsonBuffer.parseArray(file);
        if (!jsonarray.success())
        {
            Logger::syslog(F("load: Failed to read file, please add"));
            return false;
        }
        for (int i = 0; i < jsonarray.size(); i++)
        {
            userM[jsonarray[i]["id"]].name = jsonarray[i]["name"].as<String>();
            userM[jsonarray[i]["id"]].phone = jsonarray[i]["phone"].as<String>();
            userM[jsonarray[i]["id"]].telegram = jsonarray[i]["telegram"].as<String>();
            userM[jsonarray[i]["id"]].rfid = jsonarray[i]["rfid"].as<String>();
        }
        Logger::syslog("Loaded total: " + String(jsonarray.size()));
        file.close();
        return true;
    }
    static bool userSave()
    {
        Logger::syslog(__func__);
        DynamicJsonBuffer jsonBuffer;
        JsonArray &array = jsonBuffer.createArray();
        for (auto &user : userM)
        {
            JsonObject &nested = array.createNestedObject();
            nested["id"] = user.first;
            nested["name"] = user.second.name;
            nested["phone"] = user.second.phone;
            nested["telegram"] = user.second.telegram;
            nested["rfid"] = user.second.rfid;
        }
        File file = SPIFFS.open(USERCONFFILE, "w");
        array.printTo(file);
        file.close();
        Logger::syslog("Saved: " + String(array.size()));
        return true;
    }

    static bool userAdd(String name, String phone = "", String telegram = "", String rfid = "")
    {
        for (auto &user : userM)
        {
            if (name == user.second.name)
            {
                webErr = "User name: " + name + " already exists under id: " + String(user.first);
                return false;
            }
        }
        int id;
        userM.size() == 0 ? id = 0 : id = userM.rbegin()->first + 1;
        userM[id] = {name, phone, telegram, rfid};
        return userSave() ? true : false;
    }
    static bool userDelete(int id)
    {
        for (auto &zone : zoneM)
        {
            for (auto userId : zone.second.userIds)
            {
                if (userId == id)
                {
                    webErr = "User id: " + String(id) + " attached to Zone id: " + String(zone.first);
                    return false;
                }
            }
        }
        if (userM.erase(id) < 1)
        {
            webErr = "No such id: " + String(id);
            return false;
        }
        else
        {
            userSave();
            return true;
        };
    }
    static bool userUpdate(int id, String name, String phone = "", String telegram = "", String rfid = "")
    {
        for (auto &user : userM)
        {
            if (name == user.second.name && id != user.first)
            {
                webErr = "User name: " + name + " already exists under id: " + String(user.first);
                return false;
            }
        }
        if (userM.find(id) == userM.end())
        {
            webErr = "No such id: " + String(id);
            return false;
        }
        userM[id] = {name, phone, telegram, rfid};
        userSave();
        return true;
    }

    static int userIdByTelegram(String telegram)
    {
        for (auto &user : userM)
        {
            if (user.second.telegram == telegram)
                return user.first;
        }
        return -1;
    }
    static int userIdByRfid(String rfid)
    {
        for (auto &user : userM)
        {
            if (user.second.rfid.equalsIgnoreCase(rfid))
                return user.first;
        }
        return -1;
    }

    /*---------------------- PIN ----------------------*/

    static bool pinLoad()
    {
        Logger::syslog(__func__);
        File file = SPIFFS.open(PINCONFFILE, "r");
        DynamicJsonBuffer arrJsonBuffer;
        JsonArray &jsonarray = arrJsonBuffer.parseArray(file);
        if (!jsonarray.success())
        {
            Logger::syslog(F("load: Failed to read file, please add"));
            return false;
        }
        for (int i = 0; i < jsonarray.size(); i++)
        {
            int mapId = jsonarray[i]["id"];
            pinM[mapId].name = jsonarray[i]["name"].as<String>();
            pinM[mapId].gpio = jsonarray[i]["gpio"];
            pinM[mapId].mode = static_cast<pinModeEnum>(jsonarray[i]["mode"].as<int>());
            pinM[mapId].normalValue = jsonarray[i]["normalValue"];
            if (pinM[mapId].mode == in)
            {
                pinMode(pinM[mapId].gpio, INPUT);
                if (ALARMUSEINTERNALPULLUP)
                    pinMode(pinM[mapId].gpio, INPUT_PULLUP);
                pinUpdateState(mapId);
            }
            else
            {
                pinMode(pinM[mapId].gpio, OUTPUT);
                digitalWrite(pinM[mapId].gpio, pinM[mapId].normalValue);
            }
        }
        Logger::syslog("Loaded total: " + String(jsonarray.size()));
        file.close();
        pinAttachHandlers();
        return true;
    }
    static bool pinSave()
    {
        Logger::syslog(__func__);
        DynamicJsonBuffer jsonBuffer;
        JsonArray &array = jsonBuffer.createArray();
        for (auto &pin : pinM)
        {
            JsonObject &nested = array.createNestedObject();
            nested["id"] = pin.first;
            nested["name"] = pin.second.name;
            nested["gpio"] = pin.second.gpio;
            nested["mode"] = static_cast<int>(pin.second.mode);
            nested["normalValue"] = pin.second.normalValue;
        }
        File file = SPIFFS.open(PINCONFFILE, "w");
        array.printTo(file);
        file.close();
        pinDetachHandlers();
        pinAttachHandlers();

        Logger::syslog("Saved: " + String(array.size()));
        return true;
    }

    static bool pinAdd(String name, int gpio, pinModeEnum mode, bool normalValue)
    {
        if (!pinGpioAllowed(gpio))
            return false;
        for (auto &pin : pinM)
        {
            if (gpio == pin.second.gpio)
            {
                webErr = "Gpio: " + String(gpio) + " already exists under id: " + String(pin.first);
                return false;
            }
        }
        int id;
        pinM.size() == 0 ? id = 0 : id = pinM.rbegin()->first + 1;
        pinM[id] = {name, gpio, mode, normalValue};
        return pinSave() ? true : false;
    }
    static bool pinAdd(String name, String gpioStr, String modeStr, String normalValueStr)
    {
        int gpio;
        pinModeEnum mode;
        bool normalValue;
        gpio = gpioStr.toInt();
        modeStr == "in" ? mode = in : mode = out;
        normalValueStr == "true" ? normalValue = true : normalValue = false;
        return pinAdd(name, gpio, mode, normalValue) ? true : false;
    }
    static bool pinDelete(int id)
    {
        for (auto &zone : zoneM)
        {
            for (auto pinId : zone.second.pinIds)
            {
                if (pinId == id)
                {
                    webErr = "Pin id: " + String(id) + " attached to Zone id: " + String(zone.first);
                    return false;
                }
            }
        }

        if (pinM.erase(id) < 1)
        {
            webErr = "No such id: " + String(id);
            return false;
        }
        else
        {
            return pinSave() ? true : false;
        }
    }

    static bool pinUpdate(int id, String name, int gpio, pinModeEnum mode, bool normalValue)
    {
        if (!pinGpioAllowed(gpio))
            return false;
        for (auto &pin : pinM)
        {
            if (gpio == pin.second.gpio && id != pin.first)
            {
                webErr = "Pin gpio: " + String(gpio) + " already exists under id: " + String(pin.first);
                return false;
            }
        }
        if (pinM.find(id) == pinM.end())
        {
            webErr = "No such id: " + String(id);
            return false;
        }
        pinM[id] = {name, gpio, mode, normalValue};

        return pinSave() ? true : false;
    }
    static bool pinUpdate(String idStr, String name, String gpioStr, String modeStr, String normalValueStr)
    {
        int id = idStr.toInt();
        int gpio = gpioStr.toInt();
        pinModeEnum mode;
        bool normalValue;
        modeStr == "in" ? mode = in : mode = out;
        normalValueStr == "true" ? normalValue = true : normalValue = false;
        return pinUpdate(id, name, gpio, mode, normalValue) ? true : false;
    }

    static bool pinGpioAllowed(int gpio)
    {
        bool matched = false;
        for (auto pinA : PINSALLOWED)
        {
            if (pinA == gpio)
                matched = true;
        }
        if (!matched)
        {
            webErr = "Gpio: " + String(gpio) + " is not within PINSALLOWED: {";
            for (auto pinA : PINSALLOWED)
            {
                webErr += String(pinA) + ",";
            }
            webErr = webErr.substring(0, webErr.length() - 1) + "}";
            return false;
        }
        return true;
    }

    static bool pinAttachHandlers()
    {
        Logger::syslog(__func__);
        int k = 0;

        gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
        for (auto &pin : pinM)
        {
            gpio_config_t gpioConfig;
            esp_err_t configErr;
            if (pin.second.mode == in && !pin.second.attached)
            {
                gpioConfig.pin_bit_mask = BIT(pin.second.gpio);
                gpioConfig.mode = GPIO_MODE_INPUT;
                gpioConfig.pull_up_en = GPIO_PULLUP_ENABLE;
                gpioConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
                pin.second.normalValue == 0 ? gpioConfig.intr_type = GPIO_INTR_POSEDGE : gpioConfig.intr_type = GPIO_INTR_NEGEDGE;
                //gpioConfig.intr_type = GPIO_INTR_ANYEDGE;
                configErr = gpio_config(&gpioConfig);
                Logger::syslog("gpio : " + String(pin.second.gpio) + "has been configured result (normal = 0): " + String(configErr));
                pin_struct *p;
                p = &pin.second;
                gpio_isr_handler_add((gpio_num_t)pin.second.gpio, pinHandleInterrupt, (void *)p);
                Logger::syslog("Attached rising/falling handleInterrupt to gpio:" + String(pin.second.gpio) + ", id:" + pin.first);
                pin.second.attached = true;
                pin.second.interruptHandled = true;
                k++;
            }
            else if (pin.second.mode == out)
            {
                alarmSirenV.emplace_back(pin.second.gpio, !pin.second.normalValue);
                Logger::syslog("Gpio switcher has been created on gpio : " + String(pin.second.gpio));
            }
        }
        if (k == 0)
        {
            Logger::syslog("No interrupts started. All attached or now pin to work with?");
            return false;
        }
        else
        {
            Logger::syslog("Total Attached: " + String(k) + " pins..");
            return true;
        }
        return true;
    };
    static bool pinDetachHandlers()
    {
        Logger::syslog(__func__);
        gpio_uninstall_isr_service();
        int k = 0;
        for (auto &pin : pinM)
        {
            if (pin.second.attached)
            {
                detachInterrupt(pin.second.gpio);
                pin.second.attached = false;
                Logger::syslog("Detached handleInterrupt to gpio:" + String(pin.second.gpio) + ", id:" + pin.first);
                k++;
            }
        }
        alarmSirenV.clear();

        if (k == 0)
        {
            Logger::syslog("No interrupts detached. All detached?");
            return false;
        }
        else
        {
            Logger::syslog("Total Detached: " + String(k) + " pins..");
            return true;
        }
        return true;
    };

    static String pinOutStatus()
    {
        String response;
        for (auto &pin : pinM)
        {
            if (pin.second.mode == out)
            {
                pinUpdateState(pin.first);
                response += "Output pin: " + pin.second.name + ", gpio: " + String(pin.second.gpio) + ", normalValue: " + String(pin.second.normalValue) + ", state: " + String(pin.second.state);
            }
        }
        return response;
    }

    static int pinIdByGpio(int gpio)
    {
        for (auto &pin : pinM)
        {
            if (pin.second.gpio == gpio)
                return pin.first;
        }
        return -1;
    }
    static void pinUpdateState(int id)
    {
        pinM[id].state = digitalRead(pinM[id].gpio);
        pinM[id].updated = millis();
    }

    /*----ZONE--*/

    static bool zoneLoad()
    {
        Logger::syslog(__func__);
        File file = SPIFFS.open(ZONECONFFILE, "r");
        DynamicJsonBuffer arrJsonBuffer;
        JsonArray &jsonarray = arrJsonBuffer.parseArray(file);
        if (!jsonarray.success())
        {
            Logger::syslog(F("load: Failed to read file, please add users"));
            return false;
        }
        for (int i = 0; i < jsonarray.size(); i++)
        {
            int zoneId = jsonarray[i]["id"];
            zoneM[zoneId].name = jsonarray[i]["name"].as<String>();

            for (int k = 0; k < jsonarray[i]["pinIds"].size(); k++)
            {
                zoneM[zoneId].pinIds.push_back(jsonarray[i]["pinIds"][k]);
            }
            for (int k = 0; k < jsonarray[i]["userIds"].size(); k++)
            {
                zoneM[zoneId].userIds.push_back(jsonarray[i]["userIds"][k]);
            }
            for (int k = 0; k < jsonarray[i]["rfidIps"].size(); k++)
            {
                zoneM[zoneId].rfidIps.push_back(jsonarray[i]["rfidIps"][k]);
            }
        }

        file.close();
        Logger::syslog("Loaded total: " + String(jsonarray.size()));
        return true;
    }
    static bool zoneSave()
    {
        Logger::syslog(__func__);
        DynamicJsonBuffer jsonBuffer;
        JsonArray &array = jsonBuffer.createArray();
        for (auto &zone : zoneM)
        {
            JsonObject &nested = array.createNestedObject();
            nested["id"] = zone.first;
            nested["name"] = zone.second.name;

            JsonArray &nestedPinIds = jsonBuffer.createArray();
            for (auto const &value : zone.second.pinIds)
            {
                nestedPinIds.add(value);
            }
            nested["pinIds"] = nestedPinIds;

            JsonArray &nestedUserIds = jsonBuffer.createArray();
            for (auto const &value : zone.second.userIds)
            {
                nestedUserIds.add(value);
            }
            nested["userIds"] = nestedUserIds;

            JsonArray &nestedRfidIps = jsonBuffer.createArray();
            for (auto const &value : zone.second.rfidIps)
            {
                nestedRfidIps.add(value);
            }
            nested["rfidIps"] = nestedRfidIps;
        }
        File file = SPIFFS.open(ZONECONFFILE, "w");
        array.printTo(file);
        file.close();
        Logger::syslog("Saved: " + String(array.size()) + " users");
        //updateRfidIpV();
        return true;
    }
    static bool zoneAdd(String name, std::vector<int> pinIds, std::vector<int> userIds, std::vector<String> rfidIps)
    {
        Logger::syslog(__func__);
        int id;
        zoneM.size() == 0 ? id = 0 : id = zoneM.rbegin()->first + 1;
        zoneM[id].name = name;
        zoneM[id].pinIds = pinIds;
        zoneM[id].userIds = userIds;
        zoneM[id].rfidIps = rfidIps;
        return zoneSave() ? true : false;
    }
    static bool zoneAdd(String name, String pinIdsStr, String userIdsStr, String rfidIpsStr)
    {
        Logger::syslog(__func__);
        std::vector<int> pinIds;
        std::vector<int> userIds;
        std::vector<String> rfidIps;
        std::vector<String> t;
        t = zoneExplodeString(pinIdsStr);
        for (auto const &value : t)
        {
            pinIds.push_back(value.toInt());
        }
        t = zoneExplodeString(userIdsStr);
        for (auto const &value : t)
        {
            userIds.push_back(value.toInt());
        }
        t = zoneExplodeString(rfidIpsStr);
        for (auto const &value : t)
        {
            rfidIps.push_back(value);
        }

        return zoneAdd(name, pinIds, userIds, rfidIps) ? true : false;
    };

    static bool zoneDelete(int id)
    {
        if (zoneM.erase(id) < 1)
        {
            webErr = "No such id: " + String(id);
            return false;
        }
        else
        {
            return zoneSave() ? true : false;
        }
    }
    static bool zoneUpdate(int id, String name, std::vector<int> pinIds, std::vector<int> userIds, std::vector<String> rfidIps)
    {
        for (auto &zone : zoneM)
        {
            if (name == zone.second.name && id != zone.first)
            {
                webErr = "Zone name: " + String(name) + " already exists under id: " + String(zone.first);
                return false;
            }
        }
        if (zoneM.find(id) == zoneM.end())
        {
            webErr = "No such id: " + String(id);
            return false;
        }
        zoneM[id].name = name;
        zoneM[id].pinIds = pinIds;
        zoneM[id].userIds = userIds;
        zoneM[id].rfidIps = rfidIps;
        return zoneSave() ? true : false;
    }

    static bool zoneUpdate(String idStr, String name, String pinIdsStr, String userIdsStr, String rfidIpsStr)
    {
        Logger::syslog(__func__);
        int id = idStr.toInt();
        std::vector<int> pinIds;
        std::vector<int> userIds;
        std::vector<String> rfidIps;
        std::vector<String> t;
        t = zoneExplodeString(pinIdsStr);
        for (auto const &value : t)
        {
            pinIds.push_back(value.toInt());
        }
        t = zoneExplodeString(userIdsStr);
        for (auto const &value : t)
        {
            userIds.push_back(value.toInt());
        }
        t = zoneExplodeString(rfidIpsStr);
        for (auto const &value : t)
        {
            rfidIps.push_back(value);
        }
        return zoneUpdate(id, name, pinIds, userIds, rfidIps) ? true : false;
    };
    static bool zoneChangeState(bool wantedState, int id = -1, String ip = "")
    {
        Logger::syslog(__func__);
        Logger::syslog("wantedState: " + String(wantedState) + ", zoneId: " + String(id) + ", ip: " + ip);
        String s;
        zoneChangeStateMsg = "";
        if (id != -1 && zoneM.find(id) == zoneM.end())
        {
            zoneChangeStateMsg += "Zone not found or none exists";
            return false;
        }

        //create vector off zones we work with (1 or all)
        std::vector<int> zoneIds;
        if (id != -1)
        {
            zoneIds.push_back(id);
        }
        else
        {
            for (auto &zone : zoneM)
                zoneIds.push_back(zone.first);
        }
        if (wantedState == false)
        {
            if (alarmWorking == true)
                alarmStop();
            for (auto &zoneId : zoneIds)
            {
                if (zoneM[zoneId].armed == false)
                {
                    s = "Zone name: " + zoneM[zoneId].name + " already disarmed \n";
                    zoneChangeStateMsg += s;
                    Logger::syslog(s);
                }
                else
                {
                    zoneM[zoneId].armed = false;
                    for (auto &pinId : zoneM[zoneId].pinIds)
                    {
                        pinM[pinId].armed = false;
                    }
                    s = "Zone name: " + zoneM[zoneId].name + " has been DISARMED \n";
                    zoneChangeStateMsg += s;
                    Logger::syslog(s);
                }
            }
            alarmBeep(2);
            zoneSaveState();
            zoneUpdateRemoteLed(zoneIds,ip);
            return true;
        }
        else
        {
            bool testPinStateResult = true;

            for (auto &zoneId : zoneIds)
            {
                if (zoneM[zoneId].armed == true)
                {
                    s = "Zone name: " + zoneM[zoneId].name + " already ARMED\n";
                    zoneChangeStateMsg += s;
                    Logger::syslog(s);
                }
                else
                {
                    for (auto &pinId : zoneM[zoneId].pinIds)
                    {
                        pinUpdateState(pinId);
                        if (pinM[pinId].state != pinM[pinId].normalValue)
                        {
                            testPinStateResult = false;
                            s = "Zone name: " + zoneM[zoneId].name + " has pin name: " + pinM[pinId].name +
                                " which current state is not normalValue: " + String(pinM[pinId].normalValue) + "\n";
                            zoneChangeStateMsg += s;
                            Logger::syslog(s);
                        }
                    }
                }
            }
            if (!testPinStateResult)
                return false;
            alarmBeep(3);
            for (auto &zoneId : zoneIds)
            {
                if (zoneM[zoneId].armed == true)
                    continue;
                zoneM[zoneId].armed = true;
                for (auto &pinId : zoneM[zoneId].pinIds)
                {
                    pinM[pinId].armed = true;
                }
                s = "Zone name: " + zoneM[zoneId].name + " has been ARMED \n";
                Logger::syslog(s);
                zoneChangeStateMsg += s;
            }
            alarmBeep(1);
            zoneSaveState();
            zoneUpdateRemoteLed(zoneIds,ip);
            return true;
        }
    }

    static String zoneChangeState(String ip, String tag)
    {
        int zoneId = zoneIdByIp(ip);
        if (userIdByRfid(tag) == -1 || zoneId == -1)
            return "unathorized";
        if (zoneM[zoneId].armed == true)
        {
            zoneChangeState(false, zoneId, ip);
            return "ledoff";
        }
        else
        {
            alarmBeep(3);
            return zoneChangeState(true, zoneId, ip) ? "ledon" : "blink3";
        }
    }
    static void zoneSaveState()
    {
        File file = SPIFFS.open(ALARMZONESATEFILE, "w");
        if (!file)
        {
            Logger::syslog("Failed to create file:" + String(ALARMZONESATEFILE));
            return;
        }
        DynamicJsonBuffer jsonBuffer;
        JsonObject &root = jsonBuffer.createObject();
        for (auto &zone : zoneM)
        {
            root[String(zone.first)] = zone.second.armed;
        }
        root.printTo(file);
        file.close();
    }

    static void zoneLoadState()
    {
        Logger::syslog(__func__);

        File file = SPIFFS.open(ALARMZONESATEFILE, "r");
        if (!file)
        {
            Logger::syslog("Failed to read file:" + String(ALARMZONESATEFILE));
            return;
        }
        DynamicJsonBuffer jsonBuffer;
        JsonObject &root = jsonBuffer.parseObject(file);
        if (!root.success())
        {
            Logger::syslog("Failed to parse JSON.");
            return;
        }

        for (auto &zone : root)
        {
            int zoneId = String(zone.key).toInt();
            if (zoneM.find(zoneId) != zoneM.end())
            {
                zoneM[zoneId].armed = zone.value;
                for (auto pinId : zoneM[zoneId].pinIds)
                {
                    pinM[pinId].armed = zone.value;
                }
                Logger::syslog("Zone id: " + String(zoneId) + ", Loeaded state: " + String(zoneM[zoneId].armed));
            }
        }
        Logger::syslog("File: " + String(ALARMZONESATEFILE) + " parsed");
        file.close();
    }

    /*
    sets alarmUpdateRemoteLedM[ip] = false for zone, which need to update led
    if no argument passed - all zones will be updated (from setup loop for instance)
    excludeIp - ip which requested zoneChange, it will get update on it's own request
    */
    static void zoneUpdateRemoteLed(std::vector<int> zoneIdV = {}, String excludeIp = "")
    {
        Logger::syslog(__func__);
        if (zoneIdV.size() == 0) //no zone ID passed, will mark all ip to updatedNeeded
        {
            for (auto &zone : zoneM)
            {
                for (auto &ip : zone.second.rfidIps)
                {
                    alarmUpdateRemoteLedM[ip] = false;
                }
            }
        } else {
            for (auto &zoneId : zoneIdV) {
                for (auto &ip : zoneM[zoneId].rfidIps) {
                    if (ip!=excludeIp) alarmUpdateRemoteLedM[ip] = false;
                }
            }
        }
    }

    static int zoneIdByIp(String ip)
    {
        for (auto &zone : zoneM)
        {
            for (auto rfidIp : zone.second.rfidIps)
            {
                if (rfidIp == ip)
                    return zone.first;
            }
        }
        return -1;
    }
    static std::vector<String> zoneExplodeString(String str)
    {
        Logger::syslog(__func__);
        std::vector<String> v;
        String s = "";
        int index;
        while (str != "")
        {
            index = str.indexOf(DELIMITER);
            if (index == -1)
            {
                s = str;
                str = "";
            }
            else
            {
                s = str.substring(0, index);
                str = str.substring(index + 1, str.length());
            }
            v.push_back(s);
        }
        return v;
    }

    static String zoneStatus()
    {
        String response;
        for (auto &zone : zoneM)
        {
            response += "---> Zone : " + zone.second.name + ", armed: " + String(zone.second.armed) + "\n";
            for (auto &pinId : zone.second.pinIds)
            {
                pinUpdateState(pinId);
                response += "gpio : " + String(pinM[pinId].gpio) +
                            ",\t armed: " + String(pinM[pinId].armed) + ",\t normalValue: " + String(pinM[pinId].normalValue) +
                            ",\t state: " + String(pinM[pinId].state) + ",\t name:" + pinM[pinId].name + "\n";
            }
        }
        return response;
    }

    /* ------------ ALARM */

    //you cant put any time delaying action here
    static void alarmStart(int gpio)
    {
        alarmWorking = true;
        alarmTriggerMsg = "Triggered by gpio: " + String(gpio) + "-> " + pinM[pinIdByGpio(gpio)].name;
    }
    static void alarmStop()
    {
        Logger::syslog(__func__);
        alarmWorking = false;
        alarmStartLogged = false;
        for (auto &pin : pinM)
        {
            if (pin.second.mode == out)
                digitalWrite(pin.second.gpio, pin.second.normalValue);
        }

        for (auto &zone : zoneM)
        {
            for (auto &userId : zone.second.userIds)
            {
                if (userM[userId].telegram != "")
                {
                    mqttQueueV.push_back({"Alarm stopped", userM[userId].telegram, Timetools::getUnixTime(), -1, false});
                }
            }
        }
        Serial2.println("ath;");
    }
    static void alarmHandle()
    {
        //to avoid flase alarms caused by noise, we do give some time (ALARMMINIMALALGPIOARMEDINTERVAL) before launching
        // if (alarmWorking) {
        //     for (auto pin : pinM) {
        //         if (!pin.second.interruptHandled && millis()-pin.second.interruptTime<ALARMMINIMALALGPIOARMEDINTERVAL)
        //         return;
        //     }
        // }

        //do one time task
        if (alarmWorking && !alarmStartLogged)
        {
            Logger::syslog("Alarm started, trigger: " + alarmTriggerMsg);
            alarmStartLogged = true;
            alarmStartTime = millis();
            for (auto &pin : pinM)
            {
                if (pin.second.mode == out)
                    digitalWrite(pin.second.gpio, !pin.second.normalValue);
            }
            callQueueV.clear();
            for (auto &user : userM)
            {
                if (user.second.telegram != "")
                {
                    mqttQueueV.push_back({alarmTriggerMsg, user.second.telegram, Timetools::getUnixTime(), -1, false});
                }
                if (user.second.phone != "")
                {
                    callQueueV.push_back({user.second.phone, false, false, false, 0});
                }
            }
            callNextEvent = millis();
        }

        if (alarmWorking && callQueueV.size() != 0)
            alarmCallQueuHandle();

        //handle timeout
        if (alarmWorking)
        {
            if ((millis() - alarmStartTime) > ALARMDURATION)
            {
                Logger::syslog("Alarm timed out");
                alarmStop();
                return;
            }
        }
    }

    static void alarmCallQueuHandle()
    {
        if (millis() > callNextEvent)
        {
            for (auto &value : Gsmserial::serialLogV)
            {
                if (callQueueV.begin()->atdTime != 0 && value.time > callQueueV.begin()->atdTime && value.msg.indexOf(SERIALCALLRECIEVEDKEYWORD) != -1)
                {
                    callQueueV.erase(callQueueV.begin());
                    Logger::syslog("GSM keyword met: " + String(SERIALCALLRECIEVEDKEYWORD));
                    return;
                }
            }
            if (callQueueV.begin()->atdTime != 0)
            {
                callQueueV.begin()->ath = false;
                callQueueV.begin()->atd = false;
                callQueueV.begin()->atdTime = 0;
            }
            if (!callQueueV.begin()->ath)
            {
                Serial2.println("ath;");
                callNextEvent = millis() + CALLATHAFTERDELAY;
                callQueueV.begin()->ath = true;
                return;
            }
            if (!callQueueV.begin()->atd)
            {
                Serial2.println("atd" + callQueueV.begin()->phone + ";");
                callQueueV.begin()->atdTime = millis();
                callQueueV.begin()->busyReceived = false;
                callNextEvent = millis() + CALLATDAFTERDELAY;
                return;
            }
        }
    }

    static void alarmBeep(int num)
    {
        if (config["alarmBeep"]=="no") return;
        
        Logger::syslog(__func__);
        for (auto &siren : alarmSirenV)
        {
            siren.start(num, ALARMBEEPDURATION);
        }
        alarmBeepEndTime = millis() + num * ALARMBEEPDURATION * 2;
    }

    /*---------------MISC  */
    static void load()
    {
        userLoad();
        pinLoad();
        zoneLoad();
        //updateRfidIpV();
        zoneUpdateRemoteLed(); //update remote all leds state
    }

    // static void updateRfidIpV()
    // {
    //     remoteRfidIpV.clear();
    //     for (auto zone : Alarm::zoneM)
    //     {
    //         for (auto ip : zone.second.rfidIps)
    //         {
    //             remoteRfidIpV.push_back({ip, 0, 0});
    //         }
    //     }
    // }
    // static String remoteRfidIpVToStr()
    // {
    //     String r;
    //     for (auto &value : remoteRfidIpV)
    //     {
    //         r += "ip: " + value.ip + ", updated: " + Timetools::formattedInterval(millis() - value.updated) + ", httpCode:" + String(value.httpCode) + "\n";
    //     }
    //     return r;
    // }
};

std::map<int, user_struct> Alarm::userM;
std::map<int, pin_struct> Alarm::pinM;
std::map<int, zone_struct> Alarm::zoneM;
String Alarm::webErr;
String Alarm::zoneChangeStateMsg;
bool Alarm::alarmWorking = false;
bool Alarm::alarmStartLogged = false;
String Alarm::alarmTriggerMsg;
long Alarm::alarmStartTime;
std::vector<callQueue_struct> Alarm::callQueueV;
long Alarm::callNextEvent;
//std::vector<Gpioswitch*> Alarm::alarmSirenV;
std::vector<Gpioswitch> Alarm::alarmSirenV;
long Alarm::alarmBeepEndTime;

std::map<String, bool> Alarm::alarmUpdateRemoteLedM;
//std::vector<remote_rfid_struct> Alarm::remoteRfidIpV;

void IRAM_ATTR Alarm::pinHandleInterrupt(void *arg)
{
    //You cant do any time consuming task here or it will crash

    //delay after siren beeps, which provokes false GPIO interrupts due to electric noise
    if (millis() < alarmBeepEndTime + ALARMAFTERBEEPINTERRUPTDELAY)
        return;

    pin_struct *pinStruct = static_cast<pin_struct *>(arg);
    if ((millis() - pinStruct->interruptTime) < INTERRUPTHANDLERDELAY)
        return;
    pinStruct->interruptTime = millis();
    pinStruct->interruptHandled = false;
    if (pinStruct->armed)
        //alarmStart("Triggered by GPIO: " + String(pinStruct->gpio));
        alarmStart(pinStruct->gpio);
};

#endif
