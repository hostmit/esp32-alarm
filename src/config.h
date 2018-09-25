#ifndef CONFIG_H
#define CONFIG_H


// #include <ESPAsyncWebServer.h>
#include <map>
#include "gpioswitch.h"


//wifi section
long wifiConnectTime = 0;
long wifiConnectCount = 0;
String confWifiSsid;
String confWifiPassword;


//time
#define NTPSERVER "0.ua.pool.ntp.org"
#define NTPGMTOFFSETSEC 7200
#define NTPDAYLIGHTOFFSETSEC 3600
#define NTPRETRYDELAY 10000


//logger
#define LOGUSESERIAL true
#define LOGFILENAME "/syslog.log"
#define LOGLINEMAXLENGTH 1000
#define LOGFILESIZE 30000
#define LOGPREVEXT "prev"



//ota
unsigned int OtaProgress;
unsigned int OtaProgressPrev=0;

//filesystem
#define FORMAT_SPIFFS_IF_FAILED true

//config
#define CONFIGFILENAME "/conf.conf"
struct ConfigStruct {
    String name;
    String value;
    String label;
    bool required;
    String regex;
};
std::map<String,String> config;
//name,defaultvaluem,htmlFormLabel,required?,html5FormRegex
std::vector<ConfigStruct> configV = {
    {"wifiSsid","luchfree","wifi ssid {1..20} alphanumeric",true,"^(\\w{1,20})$"},
    {"wifiPassword","","wifi password {1..20}",false,"^(.{1,20})$"},
    {"httpUsername","admin","http username {3..20} alphanumeric",true,"^(\\w{3,20})$"},
    {"httpPassword","admin","http password {3..20}",true,"^(.{3,20})$"},
    {"otaHostname","","ota hostname {1..20} alphanumeric",false,"^(\\w{1,20})$"},
    {"otaPassword","","ota password {1..20} alphanumeric",false,"^(\\w{1,20})$"},
    {"mqttHost","","mqtt host",false,""},
    {"mqttPort","","mqtt port",false,""},
    {"mqttUser","","mqtt user",false,""},
    {"mqttPassword","","mqtt password",false,""},
    {"mqttInTopic","","mqttInTopic",false,""},
    {"mqttOutTopic","","mqttOutTopic",false,""},
    {"gsmOperator","mts","gsmOperator:mts,life",true,"^(mts|life)$"},
    {"gsmMinBalance","0","gsmMinBalance",true,""},
    {"alarmBeep","yes","alarmBeep:yes,no",true,"^(yes|no)$"},
    {"batteryGpio","","baggeryGpio",false,""},
    {"batteryVCoff","1","batteryVCoff",false,""},
    {"batteryAdcOffV","13","batteryAdcOffV",false,""},
    {"batteryLowV","12.1","batteryLowV",false,""}
};
#define STARTNUMFILE "/start.conf"
#define STARTNUMFILEMAXSIZE 100



//web
String updateConfigHtml	=	F(R"~(<!DOCTYPE html><html><style>button,label{display:block}label{padding-bottom:2px;padding-top:10px}button{margin-top:10px}</style><body><form action="">__FORM_BLOCK__ <button type="submit">submit</button></form></body></html>)~");

//sys
#define ESPRESTARTDELAY 2000
bool espRestartRequest = false;


//sysino
#define LOOPSTATSIZE 10
#define LOOPSTATDURATION 1000

//gpioswitcher
#define ONBOARDLEDPIN 2
Gpioswitch builtInLed(BUILTIN_LED);

//alarm

struct user_struct {
            String name;
            String phone;
            String telegram;
            String rfid;
};

enum pinModeEnum {in, out};
struct pin_struct {
            String name;
            int gpio;
            pinModeEnum mode;
            bool normalValue;
            
            //below runtime data, not saved to config
            bool state;
            long updated;
            bool attached;
            bool armed;
            long interruptTime;
            bool interruptHandled;
};

struct zone_struct {
    String name;
    std::vector<int> pinIds;
    std::vector<int> userIds;
    std::vector<String> rfidIps;
    //below runtime data, not saved to config
    bool armed = false;
};

#define USERCONFFILE "/user.json"
#define USERADDHTMLFILE "/user_add.html"
String userUpdateHtml	= F(R"~(<!DOCTYPE html><html><style>button,label{display:block}label{padding-bottom:2px;padding-top:10px}button{margin-top:10px}hr{margin-top:30px;margin-bottom:30px}</style><body><form action=""><label>name 20 alphanum+undersocre</label><input type="text" name="name" value="name_value" maxlength="20" pattern="(^\w{1,20}$)" required><label>phone +380*********</label><input type="text" name="phone" value="phone_value" maxlength="13" pattern="(^$|^\+380\d{9}$)"><label>telegram</label><input type="text" name="telegram" value="telegram_value" maxlength="32" pattern="(^$|^\d{5,32}$)"><label>rfid</label><input type="text" size="32" name="rfid" value="rfid_value" maxlength="32" pattern="(^$|^[a-zA-Z0-9_]{32}$)"> <input type="hidden" name="id" value="id_value"> <button type="submit">submit</button></form></body></html>)~");

#define PINCONFFILE "/pin.json"
#define ZONECONFFILE "/zone.json"
#define PINADDHTMLFILE "/pin_add.html"


//GPIO34 - onboard led
//GPIO33 - Serial?
//GPIO 32 - doesnt respond on handle
//GPIO 35 - doesnt respond on handle, 35 missing PULLUP option?
//working 12,14,25,26,27 as PIR
//working 13
//19 - got reseted once
static const int PINSALLOWED[] = {4,5,12,13,14,18,19,21,22,23,25,26,27,34};
String pinUpdateHtml = F(R"~(<!DOCTYPE html><html><style>button,label{display:block}label{padding-bottom:2px;padding-top:10px}button{margin-top:10px}hr{margin-top:30px;margin-bottom:30px}</style><body><form action=""><label>gpio</label><input type="text" name="gpio" maxlength="2" value="gpio_value" pattern="(^\d{1,2}$)" required><label>mode</label><select name="mode"><option value="in">in</option><option value="out">out</option></select><label>normalValue</label><select name="normalValue"><option value="true">true</option><option value="false">false</option></select><label>name</label><input type="text" size="20" name="name" maxlength="20" value="name_value" pattern="(^$|^[a-zA-Z0-9_ ]{0..20}$)"> <input type="hidden" name="id" value="id_value"> <button type="submit">submit</button></form></body></html>)~");
#define DELIMITER ","
String zoneAddHtml = F(R"~(<!DOCTYPE html><html><style>button,select{display:block;margin-top:10px}label{display:block;padding-bottom:2px;padding-top:10px}hr{margin-top:30px;margin-bottom:30px}select{width:200px}</style><body><form action=""><label>name</label><input type="text" name="name" maxlength="20" pattern="(^\w{1,20}$)" required><select name="pins" multiple>pins_options</select><select name="users" multiple>users_options</select><label>ip address, coma separated</label><input type="text" size="50" name="rfidips" maxlength="100" pattern="^$|^((([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])[,]?)+$"> <button type="submit">submit</button></form></body></html>)~");
String zoneUpdateHtml = F(R"~(<!DOCTYPE html><html><style>button,select{display:block;margin-top:10px}label{display:block;padding-bottom:2px;padding-top:10px}hr{margin-top:30px;margin-bottom:30px}select{width:200px}</style><body><form action=""><label>name</label><input type="text" name="name" maxlength="20" value="name_value" pattern="(^\w{1,20}$)" required><select name="pins" multiple>pins_options</select><select name="users" multiple>users_options</select><label>ip address, coma separated</label><input type="text" size="50" name="rfidips" maxlength="100" value="rfidips_value" pattern="^$|^((([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])[,]?)+$"> <input type="hidden" name="id" value="id_value"> <button type="submit">submit</button></form></body></html>)~");
#define ESP_INTR_FLAG_DEFAULT 0
#define INTERRUPTHANDLERDELAY 500
#define ALARMDURATION 120000
#define ALARMBEEPDURATION 100
#define ALARMAFTERBEEPINTERRUPTDELAY 1000
#define ALARMZONESATEFILE "/zonestate.json"
#define ALARMUSEINTERNALPULLUP true
#define ALARMMINIMALALGPIOARMEDINTERVAL 300
struct remote_rfid_struct {
    String ip;
    long updated;
    int httpCode;
};

//mqtt
#define MQTTINTOPICQOS 0
#define MQTTOUTTOPICQOS 2
#define TGMAXMSGDELAYSEC 10
struct mqtt_message_queue_struct {
    String text;
    String chatid;
    long date;
    int packetId;
    bool delivered;
};
std::vector<mqtt_message_queue_struct> mqttQueueV;
#define MQTTQUEUESIZE 100
long mqttHandleLastAttempt;
#define MQTTHANDLEDELAY 5000


//gsm
struct callQueue_struct {
    String phone;
    bool ath;
    bool atd;
    bool busyReceived;
    long atdTime;

};
#define CALLATHAFTERDELAY 1000;
#define CALLATDAFTERDELAY 30000;

struct balance_struct {
    float balance;
    long lastAttempt;
    long lastResultTime;
};
#define GSMBALANCECHECKPERIOD 3600*24*1000
#define GSMBALANCEONSTARTDELAY 10000
#define GSMBALANCECMDDELAY 20000
#define GSMBALANCENOTIFYHOURSTART 9
#define GSMBALANCENOTIFYHOUREND 19
struct gsm_operator_struct {
    String cmdPrep;
    String cmdBalance;
    String markStart;
    String markEnd;
};


std::map <String,gsm_operator_struct> gsmOperatorM = {
    {"life",{
        "AT+CSCS=\"GSM\"",
        "AT+CUSD=1,\"*111#\"",
        "Balans ",
        " hrn."
    }},
    {"mts",{
        "",
        "AT+CUSD=1,\"*101#\"",
        "rahunku ",
        " grn."
    }},
};



struct serialLog_struct {
    unsigned long time;
    String msg;
};
#define SERIALLOGMAXSIZE 30
#define SERIALCALLRECIEVEDKEYWORD "BUSY"
#define GSMSERIALATHTMLFILE "/gsm_at.html"

//task
#define HTTPUPDATERTIMEOUT 500
#define HTTPUPDATERREUSE true

//voltmeter
#define VOLTMETERREADINTERVAL 50
#define VOLTMETERPOOLSIZE 50
//#define VOLTMETERCHECKINTERVAL 5*60*1000
#define VOLTMETERCHECKINTERVAL 20*1000

#endif