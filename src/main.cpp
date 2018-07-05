#include <Arduino.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <JC_Button.h>

#define BUTTON 0
#define RELAY 12
#define LED 13

char host[11];
WiFiManager wiFiManager;
ESP8266WebServer server(80);
Button button = Button(BUTTON, INPUT_PULLUP, true, true);

void setupOTA() {
    ArduinoOTA.setHostname(host);
    ArduinoOTA.onStart([]() {
        Serial.println("Start OTA update");
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();
}


void setup() {
    Serial.begin(115200);
    sprintf(host, "esp_%06x", ESP.getChipId());

    pinMode(RELAY, OUTPUT);
    digitalWrite(RELAY, LOW);

    pinMode(LED, OUTPUT);
    digitalWrite(LED, HIGH);

    // if we have been configured, dont bother starting a config portal, we could be coming back from a power outage and wifi isn't there yet
    if (WiFi.SSID() == "") {
        Serial.println("no config, start portal for 60s");
        if (wiFiManager.autoConnect(host)) {
            Serial.println("connect");
        } else {
            Serial.println("no connect");
            for (int i = 0; i < 5; i++) {
                digitalWrite(LED, LOW);
                delay(200);
                digitalWrite(LED, HIGH);
                delay(200);
            }
        }
    }

    setupOTA();

    if (MDNS.begin(host)) {
        Serial.println("MDNS responder started");
    }

    server.on("/", []() {
        char body[200];
        sprintf(body, "%s\nLED: %s\nRELAY: %s\n",
                host,
                !digitalRead(LED) ? "on" : "off",
                digitalRead(RELAY) ? "on" : "off");

        server.send(200, "text/plain", body);
    });

    server.on("/relay/on", []() {
        digitalWrite(RELAY, HIGH);
        server.send(200, "text/plain", "ok");
    });

    server.on("/relay/off", []() {
        digitalWrite(RELAY, LOW);
        server.send(200, "text/plain", "ok");
    });

    server.on("/led/on", []() {
        digitalWrite(LED, LOW);
        server.send(200, "text/plain", "ok");
    });

    server.on("/led/off", []() {
        digitalWrite(LED, HIGH);
        server.send(200, "text/plain", "ok");
    });

    server.begin();

    Serial.print("ID    -> ");
    Serial.println(host);
}

void loop() {
    digitalWrite(LED, WiFi.status() != WL_CONNECTED);
    ArduinoOTA.handle();
    server.handleClient();

    button.read();
    if (button.wasReleased()) {
        digitalWrite(RELAY, static_cast<uint8_t>(!digitalRead(RELAY)));
    }
    if (button.pressedFor(5000)) {
        for (int i = 0; i < 5; i++) {
            digitalWrite(LED, HIGH);
            delay(200);
            digitalWrite(LED, LOW);
            delay(200);
        }
        wiFiManager.resetSettings();
        delay(300);
        ESP.reset();
        delay(300);
        ESP.restart();
        delay(300);
    }
}

