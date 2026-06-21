#include "MQTTManager.h"

MQTTManager::MQTTManager(const char* ssid, const char* password, const char* broker, int port, const char* clientId, const char* subscribeTopic) {
    _ssid = ssid;
    _password = password;
    _broker = broker;
    _port = port;
    _clientId = clientId;
    _subscribeTopic = subscribeTopic;
    
    // I-link ang WiFiClient sa PubSubClient
    _mqttClient.setClient(_espClient);
}

void MQTTManager::initWiFi() {
    Serial.print("[WiFi] Connecting to: ");
    Serial.println(_ssid);

    WiFi.mode(WIFI_OFF); 
    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\n[WiFi] Connected successfully!");
    Serial.print("[WiFi] IP Address: ");
    Serial.println(WiFi.localIP());
}

void MQTTManager::begin(MQTT_CALLBACK_SIGNATURE) {
    initWiFi();
    _mqttClient.setServer(_broker, _port);
    _mqttClient.setCallback(callback);
}

void MQTTManager::maintain() {
    if (WiFi.status() != WL_CONNECTED) {
        return; 
    }

    if (!_mqttClient.connected()) {
        Serial.print("[MQTT] Connecting to broker at ");
        Serial.println(_broker);
        
        if (_mqttClient.connect(_clientId)) {
            Serial.println("[MQTT] Connected to Broker!");
            if (strlen(_subscribeTopic) > 0) {
                _mqttClient.subscribe(_subscribeTopic);
                Serial.print("[MQTT] Subscribed to: ");
                Serial.println(_subscribeTopic);
            }
        } else {
            Serial.print("[MQTT] Failed, rc=");
            Serial.println(_mqttClient.state());
        }
    }
}

void MQTTManager::loop() {
    _mqttClient.loop();
}

bool MQTTManager::publish(const char* topic, const char* payload) {
    if (_mqttClient.connected()) {
        return _mqttClient.publish(topic, payload);
    }
    return false;
}

bool MQTTManager::isConnected() {
    return _mqttClient.connected();
}