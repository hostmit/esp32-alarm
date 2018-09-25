#ifndef CONF_H
#define CONF_H

#include <ArduinoJson.h>
#include <SPIFFS.h>

#include "config.h"
#include "logger.h"

class Conf
{
  public:
	static int startNum;
	
	static void load()
	{
		config.clear();
		Logger::syslog("Reading Configuration file...");

		File file = SPIFFS.open(CONFIGFILENAME, "r");
		if (!file)
		{
			Logger::syslog("Configuration file: " + String(CONFIGFILENAME) + " does not exists or cant be open. Will use configDefault");
			for (auto &value : configV)
			{
				config.insert({value.name, value.value});
			}
			return;
		}
		DynamicJsonBuffer jsonBuffer;
		JsonObject &root = jsonBuffer.parseObject(file);
		if (!root.success())
		{
			Logger::syslog("Configuration file: " + String(CONFIGFILENAME) + " failed to parse JSON. Will use configDefault");
			for (auto &value : configV)
			{
				config.insert({value.name, value.value});
			}
			return;
		}
		for (auto &value : configV)
		{
			if (!root.containsKey(value.name))
			{
				Logger::syslog("Configuration file: " + String(CONFIGFILENAME) + " doesnt have " + value.name + " value, will go with default value");
				config.insert({value.name, value.value});
			}
			else
			{
				config[value.name] = root[value.name].as<String>();
			}
		}
		Logger::syslog("Configuration file: " + String(CONFIGFILENAME) + " parsed");
		file.close();
	}

	static void save()
	{
		Logger::syslog("Saving Configuration file...");
		File file = SPIFFS.open(CONFIGFILENAME, "w");
		if (!file)
		{
			Logger::syslog("Failed to create config file ");
			return;
		}
		DynamicJsonBuffer jsonBuffer;
		JsonObject &root = jsonBuffer.createObject();
		for (auto &value : configV)
		{
			root[value.name] = config[value.name];
		}
		if (root.printTo(file) == 0)
		{
			Logger::syslog("Failed to write to config file");
		}
		else
		{
			Logger::syslog("Config saved to: " + String(CONFIGFILENAME));
		}

		file.close();
	}

	static String getAsSting()
	{
		String r = "";
		for (auto &item : config)
		{
			r += item.first + ":" + item.second + "\n";
		}
		return r;
	}
	static void updateStartNumber()
	{
		StaticJsonBuffer<512> jsonBuffer;
		File file = SPIFFS.open(STARTNUMFILE, "r");
		if (file && file.size() < STARTNUMFILEMAXSIZE)
		{
			JsonObject &root = jsonBuffer.parseObject(file);
			if (root.success() && root.containsKey("startNum"))
			{
				startNum = root["startNum"];
			}
		}
		file.close();
		file = SPIFFS.open(STARTNUMFILE, "w");
		if (!file)
		{
			Logger::syslog("Unable to open file:" + String(STARTNUMFILE));
			return;
		}
		JsonObject &root = jsonBuffer.createObject();
		startNum++;
		root["startNum"] = startNum;
		root.printTo(file);
		file.close();
	}
};

int Conf::startNum=0;

#endif