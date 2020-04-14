/*
https://github.com/Fabio3rs/Monomotronic-MA1.7-Scanner

Test code

MIT License

Copyright (c) 2019 Fabio3rs

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <WiFiClient.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <cstring>
#include "Log.h"

#include "WiFiConfig.h"
#include "ESP32Monomotronic.h"

ESP32Monomotronic scanner;

AsyncWebServer server(80);

String mkstatus()
{
	int errorCode = scanner.getThreadErrorCode();
	auto thrstate = scanner.getThreadState();
	String result = "Erro: ";
	result += String(errorCode);
	result += "<br />Thread state: ";

	switch(thrstate)
	{
	case eRunning:
		 result += "eRunning";
		 break;
	case eReady:
		 result += "eReady";
		 break;
	case eBlocked:
		 result += "eBlocked";
		 break;
	case eSuspended:
		 result += "eSuspended";
		 break;
	case eDeleted:
		 result += "eDeleted";
		 break;
		 
	default:
		 result += "Unknown " + String(thrstate);
		 break;
	}
	
	result += "<br />";
	if (errorCode > 0)
	{
		result += "<br />Linha do erro: ";
		result += String(scanner.getDebugLine());
	}
	auto initPackets = scanner.getInitPackets();
	
	if (initPackets)
	{
		for (auto &p : *initPackets)
		{
			if (p.frametypeid == 0xF6)
			{
				char tmp[32] = { 0 };
				strncpy(tmp, (const char*)&p.data[0], p.data.size());
				result += tmp;
				result += "<br />";
			}
		}
	}
	
	result += "<br />ECU Connection: ";
	result += String(scanner.isECUConnected());
	result += "<br />Echo bytes: ";
	result += String(scanner.isEchoBytePresent()? "True" : "False");
	result += "<br />Task state: ";
	result += String(scanner.getThreadTaskState());
	result += "<br />";
	return result;
}

void setup()
{
	pinMode (2, OUTPUT);
	pinMode (0, OUTPUT);
	Serial.begin(115200);
	Serial2.begin(4800, SERIAL_8N1);
	pinMode (LED_BUILTIN, OUTPUT);
 
	if(!SPIFFS.begin())
	{
		Serial.println("An Error has occurred while mounting SPIFFS");
		return;
	}
	WiFi.begin(ssid, password);
	digitalWrite(LED_BUILTIN, LOW);
 
	/*while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.println("Connecting to WiFi..");
		digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
	}
	
	digitalWrite(LED_BUILTIN, HIGH);
	Serial.println("Connected to the WiFi network");
	Serial.println(WiFi.localIP());*/
	
	Serial.println("Setting AP (Access Point)…");
	// Remove the password parameter, if you want the AP (Access Point) to be open
	WiFi.softAP("Test_WiFi", "d3e4a0b1c2");

	IPAddress IP = WiFi.softAPIP();
	
	Serial.print("AP IP address: ");
	Serial.println(IP);

	
	// server
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/index.html");
	});
	server.on("/highcharts.js", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/highcharts.js");
	});
	server.on("/ram", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send_P(200, "text/plain", String(ESP.getFreeHeap()).c_str());
	});
	server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send_P(200, "text/plain", mkstatus().c_str());
	});
	server.on("/connect", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		scanner.init();
		request->send_P(200, "text/plain", String("Initing").c_str());
	});
	server.on("/log", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		const std::string &a = CLog::l().getContent();
		request->send_P(200, "text/plain", a.c_str());
		CLog::l().logclear();
	});

	//server.onNotFound(handleNotFound);

	server.begin();
	Serial.println("HTTP server started");
}
 
void loop()
{
	
}
