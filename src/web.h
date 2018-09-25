#ifndef WEB_H
#define WEB_H

#include <Arduino.h>
#include <WebServer.h>

#include "config.h"
#include "logger.h"
#include "sysinfo.h"
#include "conf.h"
#include "alarm.h"
#include "fstools.h"
#include "voltmeter.h"

#include <driver/adc.h>

class Web
{
  public:
    static WebServer server;

    static void handle()
    {
        server.handleClient();
    }

    static void setup()
    {
        root();
        fs();
        syslog();
        updateConfig();
        sys();
        time();
        alarmUser();
        alarmPin();
        alarmZone();
        rfid();
        gsm();
        test();
        server.begin();
    }

  private:
    static void fs()
    {
        server.on("/fs/list", []() {
            if (!server.authenticate(config["httpUsername"].c_str(), config["httpPassword"].c_str()))
                return server.requestAuthentication();
            String htmlResponse = Fstools::webListRoot();
            server.send(200, "text/html", htmlResponse);
        });
        server.on("/fs/format", []() {
            if (!server.authenticate(config["httpUsername"].c_str(), config["httpPassword"].c_str()))
                return server.requestAuthentication();
            bool result = SPIFFS.format();
            String t = "";
            result == true ? t = "Successfuly formated" : t = "Unable to format";
            server.send(200, "text/plain", t);
        });

        server.on("/fs/read", []() {
            if (!server.authenticate(config["httpUsername"].c_str(), config["httpPassword"].c_str()))
                return server.requestAuthentication();

            if (server.hasArg("name"))
            {
                File file = SPIFFS.open(server.arg("name"));
                server.streamFile(file, "text/plain");
            }
            else
            {
                String htmlResponse = "";
                htmlResponse += "<pre>";
                htmlResponse = +"I need file name";
                htmlResponse += "<a href='/fs/list'>Back to fs list</a>\n";
                htmlResponse += "<a href='/'>Back to /</a>\n";
                htmlResponse += "</pre>";
                server.send(200, "text/html", htmlResponse);
            }
        });

        server.on("/fs/delete", []() {
            if (!server.authenticate(config["httpUsername"].c_str(), config["httpPassword"].c_str()))
                return server.requestAuthentication();
            String htmlResponse = "";
            htmlResponse += "<pre>";
            if (server.hasArg("name"))
            {
                SPIFFS.remove(server.arg("name")) ? htmlResponse += "File removed...\n" : htmlResponse += "Remove file FAILED...\n";
            }
            else
            {
                htmlResponse = +"I need file name";
            }
            htmlResponse += "<hr>";
            htmlResponse += "<a href='/fs/list'>Back to fs list</a>\n";
            htmlResponse += "<a href='/'>Back to /</a>\n";
            htmlResponse += "</pre>";
            server.send(200, "text/html", htmlResponse);
        });
    }

    static void root()
    {
        server.on("/", []() {
            String t = "";
            t += "<pre>";
            //t += "--- OTAHOSTNAME: " + String(config["otaHostname"]) + "\n";
            t += "\n";
            t += "<strong><a href='/fs/read?name=/syslog.log'>syslog</a></strong>\n";
            t += "\n";
            t += "/fs/{<a href='/fs/list'>list</a>,<a href='/fs/format' onclick=\"return confirm('Are you sure?')\">format</a>}\n";
            t += "\n";
            t += "<strong>--- Available commands are: --- </strong>\n";
            t += "\n";
            t += "/log/{<a href='/log/putMark'>putMark</a>}\n";
            t += "\n";
            t += "/time/{<a href='/time/update'>update</a>}\n";
            t += "\n";
            t += "/config/{<a href='/config/update'>update</a>}\n";
            t += "\n";
            t += "/user/{<a href='/user/list'>list(update/delete)</a>,<a href='/user/add'>add</a>}\n";
            t += "\n";
            t += "/pin/{<a href='/pin/list'>list(update/delete)</a>,<a href='/pin/add'>add</a>,<a href='/pin/attach'>attach</a>,<a href='/pin/detach'>detach</a>}\n";
            t += "\n";
            t += "/zone/{<a href='/zone/list'>list(update/delete)</a>,<a href='/zone/add'>add</a>}\n";
            t += "\n";
            t += "/gsm/{<a href='/gsm/at'>at</a>,<a href='/gsm/log'>log</a>}\n";
            t += "\n";
            t += "\n";
            t += "/sys/{<a href='/sys/restart' onclick=\"return confirm('Are you sure?')\">restart</a>}\n";
            t += "\n";
            t += Sysinfo::getRamInfo();
            t += Sysinfo::getFsInfo();
            t += Sysinfo::getResetReason();
            t += "<strong>--- Current time: </strong>" + Timetools::formattedTime();
            t += "\n";
            t += "<strong>--- Uptime: </strong>" + Timetools::formattedInterval(millis()) + "\n";
            t += F("<strong>--- WiFi: </strong>");
            t += Sysinfo::getWifi();
            t += "<strong>--- startNum: </strong>" + String(Conf::startNum) + "\n";
            t += Sysinfo::getLoopStat();
            t += "\n";
            t += "<strong>--- zoneStatus: </strong> \n";
            t += Alarm::zoneStatus();
            t += "\n";
            t += "<strong>--- pinOutStatus: </strong>";
            t += Alarm::pinOutStatus();
            t += "\n";
            //t += "<strong>--- remoteRfidIpV: </strong>\n";
            // t += Alarm::remoteRfidIpVToStr();
            // t += "\n";
            t += "<strong>--- gsmBalance: </strong> ";
            Gsmserial::balance.lastResultTime == 0 ? t += " never" : t += String(Gsmserial::balance.balance) + " , LastResult: " + Timetools::formattedInterval(millis() - Gsmserial::balance.lastResultTime) + " ago";
            t += "\n";
            t += "<strong>--- alarmUpdateRemoteLedM: </strong> ";
            for (auto &value : Alarm::alarmUpdateRemoteLedM)
            {
                t += String(value.first) + ":" + String(value.second) + "\n";
            }
            t += "<strong>--- voltMeter: </strong> " + String(Voltmeter::get()) + "\n";
            t += "\n";
            t += "</pre>";

            server.send(200, "text/html", t);
        });
    }

    static void syslog()
    {
        server.on("/log/putMark", []() {
            if (!server.authenticate(config["httpUsername"].c_str(), config["httpPassword"].c_str()))
                return server.requestAuthentication();
            String htmlResponse = "";
            Logger::syslog("*************** MARK: logMarkId: " + String(Logger::logMarkId) + ", startNum: " + String(Conf::startNum));
            htmlResponse += "<pre>";
            htmlResponse += "*************** MARK: logMarkId: " + String(Logger::logMarkId) + ", startNum: " + String(Conf::startNum) + " will be put in log";
            htmlResponse += "<hr>";
            htmlResponse += "<a href='/'>Back to /</a>\n";
            htmlResponse += "</pre>";
            server.send(200, "text/html", htmlResponse);
            Logger::logMarkId++;
        });
    }
    static void updateConfig()
    {
        server.on("/config/update", []() {
            if (!server.authenticate(config["httpUsername"].c_str(), config["httpPassword"].c_str()))
                return server.requestAuthentication();
            int params = server.args();
            String htmlResponse = "";
            //no data POST/GET, will list params
            if (params == 0)
            {
                String blockToReplace = "";
                for (auto &value : configV)
                {
                    blockToReplace += "<label>" + value.label + "</label>";
                    blockToReplace += "<input type=\"text\" name=\"" + value.name + "\" value=\"" + config[value.name] + "\"";
                    if (value.regex != "")
                        blockToReplace += " pattern=\"" + value.regex + "\"";
                    if (value.required)
                        blockToReplace += " required";
                    blockToReplace += ">";
                }
                htmlResponse = updateConfigHtml;
                htmlResponse.replace("__FORM_BLOCK__", blockToReplace);
                htmlResponse += "<hr>";
                htmlResponse += "<a href='/'>Back to /</a>\n";
            }
            else
            {
                for (int i = 0; i < params; i++)
                {
                    config[server.argName(i)] = server.arg(i);
                }
                Conf::save();
                htmlResponse += "<pre>";
                htmlResponse += "Config file save been saved with following contents: \n";
                htmlResponse += "<hr>";
                htmlResponse += Fstools::fileToString(CONFIGFILENAME);
                htmlResponse += "<hr>";
                htmlResponse += F("--- You might consider <a href='/sys/restart' onclick=\"return confirm('Are you sure?')\">restart</a>\n");
                htmlResponse += "<a href='/'>Back to /</a>\n";
                htmlResponse += "</pre>";
            }
            server.send(200, "text/html", htmlResponse);
        });
    }
    static void sys()
    {
        server.on("/sys/restart", []() {
            if (!server.authenticate(config["httpUsername"].c_str(), config["httpPassword"].c_str()))
                return server.requestAuthentication();
            if (!server.authenticate(config["httpUsername"].c_str(), config["httpPassword"].c_str()))
                return server.requestAuthentication();
            String htmlResponse = "";
            htmlResponse += "<pre>";
            htmlResponse += "ESP.restart() will be issued after delay:" + String(ESPRESTARTDELAY);
            htmlResponse += "<hr>";
            htmlResponse += "<a href='/'>Back to /</a>\n";
            htmlResponse += "</pre>";
            server.send(200, "text/html", htmlResponse);
            espRestartRequest = true;
        });
    }
    static void time()
    {
        server.on("/time/update", []() {
            auto result = Timetools::update();
            if (!server.authenticate(config["httpUsername"].c_str(), config["httpPassword"].c_str()))
                return server.requestAuthentication();
            String htmlResponse;
            htmlResponse += F("<pre>");

            result ? htmlResponse += ("Time updated -> " + Timetools::formattedTime()) : htmlResponse += F("Timetools::getDateTimeStr() returned false");
            htmlResponse += F("<hr><a href='/'>Back to /</a>\n");
            htmlResponse += F("</pre>");
            server.send(200, "text/html", htmlResponse);
        });
    }

    static void alarmUser()
    {
        server.on("/user/add", []() {
            if (!server.authenticate(config["httpUsername"].c_str(), config["httpPassword"].c_str()))
                return server.requestAuthentication();
            int params = server.args();
            if (params == 0)
            {
                File file = SPIFFS.open(USERADDHTMLFILE);
                server.streamFile(file, "text/html");
                return;
            }
            else
            {
                String htmlResponse;
                htmlResponse = "<pre>";
                if (server.hasArg("name") && server.hasArg("phone") && server.hasArg("telegram") && server.hasArg("rfid"))
                {
                    Alarm::userAdd(server.arg("name"), server.arg("phone"), server.arg("telegram"), server.arg("rfid"))
                        ? htmlResponse += "User config updated...\n"
                        : htmlResponse += Alarm::webErr;
                }
                else
                {
                    htmlResponse += "Missing params\n";
                }
                htmlResponse += "<a href='/user/list'>Back to user list</a>\n";
                htmlResponse += "<a href='/'>Back to /</a>\n";
                htmlResponse += "</pre>";
                server.send(200, "text/html", htmlResponse);
            }
        });
        server.on("/user/list", []() {
            if (!server.authenticate(config["httpUsername"].c_str(), config["httpPassword"].c_str()))
                return server.requestAuthentication();
            String htmlResponse = "";
            htmlResponse += "<pre>";
            htmlResponse += "--- Current User List -- \n";
            for (auto &user : Alarm::userM)
            {
                htmlResponse += "ID: " + String(user.first) + ", Name: " + user.second.name + ", Phone: " + user.second.phone +
                                ", Telegram: " + user.second.telegram + ", rfid: " + user.second.rfid +
                                " <a href='/user/update?id=" + String(user.first) + "'>update</a>, <a href='/user/delete?id=" + String(user.first) + "' onclick=\"return confirm('Are you sure?')\">delete</a> \n";
            }
            htmlResponse += "<a href='/user/add'>Add user</a>\n";
            htmlResponse += "<a href='/'>Back to /</a>\n";
            htmlResponse += "</pre>";

            server.send(200, "text/html", htmlResponse);
        });
        server.on("/user/delete", []() {
            if (!server.authenticate(config["httpUsername"].c_str(), config["httpPassword"].c_str()))
                return server.requestAuthentication();
            String htmlResponse;
            htmlResponse += "<pre>";
            if (server.hasArg("id"))
            {
                Alarm::userDelete(server.arg("id").toInt())
                    ? htmlResponse += "User deleted... \n"
                    : htmlResponse += Alarm::webErr;
            }
            else
            {
                htmlResponse = "I need ID\n";
            }
            htmlResponse += "<a href='/user/list'>Back to user list</a>\n";
            htmlResponse += "<a href='/'>Back to /</a>\n";
            htmlResponse += "</pre>";
            server.send(200, "text/html", htmlResponse);
        });

        server.on("/user/update", []() {
            if (!server.authenticate(config["httpUsername"].c_str(), config["httpPassword"].c_str()))
                return server.requestAuthentication();
            String htmlResponse;
            if (server.hasArg("id") && server.hasArg("name") && server.hasArg("phone") && server.hasArg("telegram") && server.hasArg("rfid"))
            {
                htmlResponse += F("<pre>");
                Alarm::userUpdate(server.arg("id").toInt(), server.arg("name"), server.arg("phone"), server.arg("telegram"), server.arg("rfid"))
                    ? htmlResponse += F("User updated.\n")
                    : htmlResponse += Alarm::webErr;
                htmlResponse += F("<a href='/user/list'>Back to user list</a>\n");
                htmlResponse += "<a href='/'>Back to /</a>\n";
                htmlResponse += "</pre>";
            }
            else if (server.hasArg("id"))
            {
                int userId = server.arg("id").toInt();
                htmlResponse = userUpdateHtml;
                htmlResponse.replace("name_value", Alarm::userM[userId].name);
                htmlResponse.replace("phone_value", Alarm::userM[userId].phone);
                htmlResponse.replace("telegram_value", Alarm::userM[userId].telegram);
                htmlResponse.replace("rfid_value", Alarm::userM[userId].rfid);
                htmlResponse.replace("id_value", server.arg("id"));
            }
            else
            {
                htmlResponse += "<pre>";
                htmlResponse += "I need ID...\n";
                htmlResponse += "<a href='/user/list'>Back to user list</a>\n";
                htmlResponse += "<a href='/'>Back to /</a>\n";
                htmlResponse += "</pre>";
            }
            server.send(200, "text/html", htmlResponse);
        });
    }
    static void alarmPin()
    {

        server.on("/pin/add", []() {
            if (!server.authenticate(config["httpUsername"].c_str(), config["httpPassword"].c_str()))
                return server.requestAuthentication();
            int params = server.args();

            if (params == 0)
            {
                File file = SPIFFS.open(PINADDHTMLFILE);
                server.streamFile(file, "text/html");
                return;
            }
            else
            {
                String htmlResponse;
                htmlResponse += F("<pre>");
                if (server.hasArg("gpio") && server.hasArg("mode") && server.hasArg("normalValue") && server.hasArg("name"))
                {
                    Alarm::pinAdd(server.arg("name"), server.arg("gpio"), server.arg("mode"), server.arg("normalValue"))
                        ? htmlResponse += F("Pin added...\n")
                        : htmlResponse += Alarm::webErr;
                }
                else
                {
                    htmlResponse += F("Missing params");
                }
                htmlResponse += F("<hr>");
                htmlResponse += F("<a href='/pin/list'>Back to pin list</a>\n");
                htmlResponse += F("<a href='/'>Back to /</a>\n");
                htmlResponse += F("</pre>");
                server.send(200, "text/html", htmlResponse);
            }
        });

        server.on("/pin/list", []() {
            if (!server.authenticate(config["httpUsername"].c_str(), config["httpPassword"].c_str()))
                return server.requestAuthentication();
            String htmlResponse = "";
            htmlResponse += F("<pre>");
            htmlResponse += F("--- Current Pin List -- \n");
            for (auto &pin : Alarm::pinM)
            {
                htmlResponse += "ID: " + String(pin.first) + ", GPIO: " + String(pin.second.gpio) + ", mode: ";
                pin.second.mode == in ? htmlResponse += "in" : htmlResponse += "out";
                htmlResponse += ", normalValue: " + String(pin.second.normalValue) + ", Name: " + pin.second.name;
                if (pin.second.mode == in)
                {
                    Alarm::pinUpdateState(pin.first);
                    htmlResponse += ", State: " + String(pin.second.state);
                }
                htmlResponse += ", last changed: " + Timetools::formattedInterval(millis() - pin.second.updated);
                htmlResponse += ", attached: " + String(pin.second.attached);
                htmlResponse += ", armed: " + String(pin.second.armed);
                htmlResponse += " <a href='/pin/update?id=" + String(pin.first) + "'>update</a>, <a href='/pin/delete?id=" + String(pin.first) + "' onclick=\"return confirm('Are you sure?')\">delete</a> \n";
            }
            htmlResponse += F("<hr>");
            htmlResponse += F("<a href='/pin/add'>Add pin</a>\n");
            htmlResponse += F("<a href='/'>Back to /</a>\n");
            htmlResponse += F("</pre>");
            server.send(200, "text/html", htmlResponse);
        });

        server.on("/pin/delete", []() {
            if (!server.authenticate(config["httpUsername"].c_str(), config["httpPassword"].c_str()))
                return server.requestAuthentication();
            String htmlResponse;
            htmlResponse += F("<pre>");
            if (server.hasArg("id"))
            {
                Alarm::pinDelete(server.arg("id").toInt())
                    ? htmlResponse += F("Pin deleted... \n")
                    : htmlResponse += Alarm::webErr;
            }
            else
            {
                htmlResponse += F("Invalid request...\n");
            }

            htmlResponse += F("<hr>");
            htmlResponse += F("<a href='/pin/list'>Back to pin list</a>\n");
            htmlResponse += F("<a href='/'>Back to /</a>\n");
            htmlResponse += F("</pre>");
            server.send(200, "text/html", htmlResponse);
        });

        server.on("/pin/update", []() {
            if (!server.authenticate(config["httpUsername"].c_str(), config["httpPassword"].c_str()))
                return server.requestAuthentication();

            String htmlResponse;
            htmlResponse += F("<pre>");
            if (server.hasArg("id") && server.hasArg("name") && server.hasArg("gpio") && server.hasArg("mode") && server.hasArg("normalValue"))
            {
                htmlResponse += F("<pre>");
                Alarm::pinUpdate(server.arg("id"), server.arg("name"), server.arg("gpio"), server.arg("mode"), server.arg("normalValue"))
                    ? htmlResponse += F("Pin updated.")
                    : htmlResponse += Alarm::webErr;
                htmlResponse += F("<hr>");
                htmlResponse += F("<a href='/pin/list'>Back to pin list</a>\n");
                htmlResponse += F("<a href='/'>Back to /</a>\n");
                htmlResponse += F("</pre>");
            }
            else if (server.hasArg("id"))
            {
                int pinId = server.arg("id").toInt();
                htmlResponse = pinUpdateHtml;
                htmlResponse.replace("gpio_value", String(Alarm::pinM[pinId].gpio));
                Alarm::pinM[pinId].mode == in ? htmlResponse.replace(">in<", "selected >in<") : htmlResponse.replace(">out<", "selected >out<");
                Alarm::pinM[pinId].normalValue == true ? htmlResponse.replace(">true<", "selected >true<") : htmlResponse.replace(">false<", "selected >false<");
                htmlResponse.replace("name_value", String(Alarm::pinM[pinId].name));
                htmlResponse.replace("id_value", server.arg("id"));
            }
            else
            {
                htmlResponse += F("<pre>");
                htmlResponse += F("Wrong request");
                htmlResponse += F("<hr>");
                htmlResponse += F("<a href='/pin/list'>Back to pin list</a>\n");
                htmlResponse += F("<a href='/'>Back to /</a>\n");
                htmlResponse += F("</pre>");
            }
            server.send(200, "text/html", htmlResponse);
        });
    }

    static void alarmZone()
    {
        server.on("/zone/add", []() {
            if (!server.authenticate(config["httpUsername"].c_str(), config["httpPassword"].c_str()))
                return server.requestAuthentication();
            int args = server.args();
            String htmlResponse = "";
            if (args == 0)
            {
                htmlResponse = zoneAddHtml;
                String pinsBlock = "";
                for (auto &pin : Alarm::pinM)
                {
                    if (pin.second.mode == in)
                        pinsBlock += "<option value='" + String(pin.first) + "'>gpio:" + String(pin.second.gpio) + " -> name: " + String(pin.second.name) + "</option>";
                }
                htmlResponse.replace("pins_options", pinsBlock);
                String usersBlock = "";
                for (auto &user : Alarm::userM)
                {
                    usersBlock += "<option value='" + String(user.first) + "'>" + String(user.second.name) + "</option>";
                }
                htmlResponse.replace("users_options", usersBlock);
            }
            else if (server.hasArg("name") && server.hasArg("pins") && server.hasArg("users") && server.hasArg("rfidips"))
            {
                String pinsStr = "";
                String usersStr = "";

                for (int l = 0; l < args; l++)
                {
                    if (String(server.argName(l).c_str()) == "pins")
                    {
                        if (pinsStr != "")
                            pinsStr += ",";
                        pinsStr += server.arg(l).c_str();
                    }
                    else if (String(server.argName(l).c_str()) == "users")
                    {
                        if (usersStr != "")
                            usersStr += ",";
                        usersStr += server.arg(l).c_str();
                    }
                }
                htmlResponse += "<pre>";
                Alarm::zoneAdd(server.arg("name"), pinsStr, usersStr, server.arg("rfidips"))
                    ? htmlResponse += "Zone added\n"
                    : htmlResponse += "Failed to add zone: " + Alarm::webErr + "\n";

                htmlResponse += F("<hr>");
                htmlResponse += "<a href='/zone/list'>Back to zone list?</a>\n";
                htmlResponse += "<a href='/'>Back to /?</a>\n";
                htmlResponse += "</pre>";
            }
            else
            {
                htmlResponse += "<pre>";
                htmlResponse += "Invalid request.\n";
                htmlResponse += F("<hr>");
                htmlResponse += "<a href='/'>Back to /?</a>";
                htmlResponse += "</pre>";
            }
            server.send(200, "text/html", htmlResponse);
        });

        server.on("/zone/list", []() {
            if (!server.authenticate(config["httpUsername"].c_str(), config["httpPassword"].c_str()))
                return server.requestAuthentication();
            String htmlResponse = "";
            htmlResponse += F("<pre>");
            htmlResponse += F("--- Current Zone List -- \n");
            for (auto &zone : Alarm::zoneM)
            {
                htmlResponse += "ID: " + String(zone.first) + ", name: " + zone.second.name + ", pinId->gpio: [";
                int k = 0;
                for (auto &pinId : zone.second.pinIds)
                {
                    if (k != 0)
                        htmlResponse += "; ";
                    htmlResponse += String(pinId) + "->" + String(Alarm::pinM[pinId].gpio);
                    k++;
                }
                htmlResponse += "], userId->Name: [";
                k = 0;
                for (auto &userId : zone.second.userIds)
                {
                    if (k != 0)
                        htmlResponse += "; ";
                    htmlResponse += String(userId) + "->" + Alarm::userM[userId].name;
                    k++;
                }
                htmlResponse += "], rfidIp [";
                k = 0;
                for (auto &rfidIp : zone.second.rfidIps)
                {
                    if (k != 0)
                        htmlResponse += "; ";
                    htmlResponse += String(rfidIp);
                    k++;
                }
                htmlResponse += "]";
                htmlResponse += ", armed: " + String(zone.second.armed);
                htmlResponse += "   --->   ";
                htmlResponse += " <a href='/zone/update?id=" + String(zone.first) + "'>update</a>, <a href='/zone/delete?id=" + String(zone.first) + "' onclick=\"return confirm('Are you sure?')\">delete</a> \n";
            }
            htmlResponse += F("<hr>");
            htmlResponse += F("<a href='/zone/add'>Add zone</a>\n");
            htmlResponse += F("<a href='/'>Back to /</a>\n");
            htmlResponse += F("</pre>");
            server.send(200, "text/html", htmlResponse);
        });

        server.on("/zone/delete", []() {
            if (!server.authenticate(config["httpUsername"].c_str(), config["httpPassword"].c_str()))
                return server.requestAuthentication();

            String htmlResponse;
            htmlResponse += F("<pre>");
            if (server.hasArg("id"))
            {
                Alarm::zoneDelete(server.arg("id").toInt()) ? htmlResponse += F("Zone deleted... \n") : htmlResponse += Alarm::webErr;
            }
            else
            {
                htmlResponse += F("Invalid Rquest\n");
            }

            htmlResponse += F("<hr>");
            htmlResponse += F("<a href='/zone/list'>Back to zone list</a>\n");
            htmlResponse += F("<a href='/'>Back to /</a>\n");
            htmlResponse += F("</pre>");
            server.send(200, "text/html", htmlResponse);
        });

        server.on("/zone/update", []() {
            if (!server.authenticate(config["httpUsername"].c_str(), config["httpPassword"].c_str()))
                return server.requestAuthentication();
            int args = server.args();
            String htmlResponse;
            htmlResponse += F("<pre>");
            if (server.hasArg("id") && server.hasArg("name") && server.hasArg("pins") && server.hasArg("users") && server.hasArg("rfidips"))
            {
                String pinsStr = "";
                String usersStr = "";

                for (int l = 0; l < args; l++)
                {
                    if (String(server.argName(l).c_str()) == "pins")
                    {
                        if (pinsStr != "")
                            pinsStr += ",";
                        pinsStr += server.arg(l).c_str();
                    }
                    else if (String(server.argName(l).c_str()) == "users")
                    {
                        if (usersStr != "")
                            usersStr += ",";
                        usersStr += server.arg(l).c_str();
                    }
                }
                htmlResponse += F("<pre>");
                Alarm::zoneUpdate(server.arg("id"), server.arg("name"), pinsStr, usersStr, server.arg("rfidips"))
                    ? htmlResponse += F("Zone updated.")
                    : htmlResponse += Alarm::webErr;
                htmlResponse += F("<hr>");
                htmlResponse += F("<a href='/zone/list'>Back to zone list</a>\n");
                htmlResponse += F("<a href='/'>Back to /</a>\n");
                htmlResponse += F("</pre>");
            }
            else if (server.hasArg("id"))
            {
                int zoneId = server.arg("id").toInt();
                htmlResponse = zoneUpdateHtml;
                htmlResponse.replace("name_value", Alarm::zoneM[zoneId].name);

                String pinsBlock = "";
                for (auto &pin : Alarm::pinM)
                {
                    if (pin.second.mode == out)
                        continue;
                    pinsBlock += "<option value='" + String(pin.first) + "'";
                    for (auto const &pinId : Alarm::zoneM[zoneId].pinIds)
                    {
                        if (pinId == pin.first)
                            pinsBlock += " selected ";
                    }
                    pinsBlock += ">gpio:" + String(pin.second.gpio) + " -> name: " + pin.second.name + "</option>";
                }
                htmlResponse.replace("pins_options", pinsBlock);

                String usersBlock = "";
                for (auto &user : Alarm::userM)
                {
                    usersBlock += "<option value='" + String(user.first) + "'";
                    for (auto const &userId : Alarm::zoneM[zoneId].userIds)
                    {
                        if (userId == user.first)
                            usersBlock += " selected ";
                    }
                    usersBlock += ">" + user.second.name + "</option>";
                }
                htmlResponse.replace("users_options", usersBlock);

                String rfidIpsBlock = "";
                int l = 0;
                for (auto const &rfidIp : Alarm::zoneM[zoneId].rfidIps)
                {
                    if (l != 0)
                        rfidIpsBlock += ",";
                    rfidIpsBlock += rfidIp;
                    l++;
                }
                htmlResponse.replace("rfidips_value", rfidIpsBlock);

                htmlResponse.replace("id_value", server.arg("id"));
            }
            else
            {
                htmlResponse += F("<pre>");
                htmlResponse += F("Wrong request");
                htmlResponse += F("<hr>");
                htmlResponse += F("<a href='/zone/list'>Back to zone list</a>\n");
                htmlResponse += F("<a href='/'>Back to /</a>\n");
                htmlResponse += F("</pre>");
            }
            server.send(200, "text/html", htmlResponse);
        });
    }
    static void rfid()
    {
        server.on("/rfid/status", []() {
            auto zoneId = Alarm::zoneIdByIp(server.client().remoteIP().toString());
            String response;
            if (zoneId == -1)
            {
                response = "Your ip doesnt belong to any zone";
            }
            else
            {

                Alarm::zoneM[zoneId].armed == true ? response = "ledon" : response = "ledoff";
            }
            server.send(200, "text/plain", response);
            Logger::syslog("/rfid/status request is recieved from IP:" + server.client().remoteIP().toString() + ", response: " + response);
        });
        server.on("/rfid/request", []() {
            if (server.hasArg("tag"))
            {
                String s = Alarm::zoneChangeState(server.client().remoteIP().toString(), server.arg("tag"));
                Logger::syslog("/rfid/request request is received from IP:" + server.client().remoteIP().toString() + ", tag: " + server.arg("tag") + ". Response provided: " + s);
                server.send(200, "text/plain", s);
            }
            else
            {
                server.send(200, "text/plain", "I need a tag");
            }
        });
    }
    static void gsm()
    {
        server.on("/gsm/log", []() {
            if (!server.authenticate(config["httpUsername"].c_str(), config["httpPassword"].c_str()))
                return server.requestAuthentication();
            String htmlResponse = "";
            htmlResponse += "<pre>";
            htmlResponse += Gsmserial::serialLogToString();
            htmlResponse += "<hr>";
            htmlResponse += "<a href='/'>Back to /</a>\n";
            htmlResponse += "</pre>";
            server.send(200, "text/html", htmlResponse);
        });

        server.on("/gsm/at", []() {
            if (!server.authenticate(config["httpUsername"].c_str(), config["httpPassword"].c_str()))
                return server.requestAuthentication();

            if (server.hasArg("cmd"))
            {
                String htmlResponse = "";
                htmlResponse += "<pre>";
                Serial2.println(server.arg("cmd"));
                htmlResponse += "at cmd : " + server.arg("cmd") + " sent. Check logs to response";
                htmlResponse += "<hr>";
                htmlResponse += "<a href='/gsm/log'>gsm log</a>\n";
                htmlResponse += "<a href='/'>Back to /</a>\n";
                htmlResponse += "</pre>";
                server.send(200, "text/html", htmlResponse);
            }
            else
            {
                File file = SPIFFS.open(GSMSERIALATHTMLFILE);
                server.streamFile(file, "text/html");
            }
        });
    }

    static void test () {
        server.on("/test", []() {
           String r;

           r += String(Voltmeter::readings.size());
           r += "\n";
           r += String(VOLTMETERPOOLSIZE);
           r += "\n";
           r += String(Voltmeter::adcOffV);
           r += "\n";
           r += String(Voltmeter::batteryLowV);
           r += "\n";
           r += String(Voltmeter::lastCheckTime);
           
           server.send(200, "text/plain", r);
        });
    }
};

WebServer Web::server(80);

#endif