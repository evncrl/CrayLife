#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

class MQTTManager {
private:
    WiFiClient _espClient;
    PubSubClient _mqttClient;
    
    const char* _ssid;
    const char* _password;
    const char* _broker;
    int _port;
    const char* _clientId;
    const char* _subscribeTopic;

    void initWiFi();

public:
    // Constructor
    MQTTManager(const char* ssid, const char* password, const char* broker, int port, const char* clientId, const char* subscribeTopic);

    void begin(MQTT_CALLBACK_SIGNATURE);

    void maintain();

    bool publish(const char* topic, const char* payload);

    void loop();

    bool isConnected();
};

#endif // MQTT_MANAGER_H