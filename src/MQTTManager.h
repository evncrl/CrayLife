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

    // I-setup ang network at callback function
    void begin(MQTT_CALLBACK_SIGNATURE);

    // Panatilihing buhay ang koneksyon (dapat tawagin sa loop)
    void maintain();

    // Mag-publish ng data patungo sa isang partikular na topic
    bool publish(const char* topic, const char* payload);

    // I-expose ang loop function ng PubSubClient
    void loop();

    // Suriin kung konektado
    bool isConnected();
};

#endif // MQTT_MANAGER_H