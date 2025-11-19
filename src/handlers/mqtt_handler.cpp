#include "mqtt_handler.h"

bool MqttHandler::connect(IPAddress host, uint16_t port, const char *user, const char *pass)
{
    Serial.print("MQTT: connecting to ");
    Serial.print(host);
    Serial.print(":");
    Serial.println(port);

    // Set MQTT server using IPAddress
    client.setServer(host, port);

    // Try connecting
    bool ok;

    if (user && strlen(user) > 0)
        ok = client.connect(device_name, user, pass);
    else
        ok = client.connect(device_name);

    if (ok)
    {
        Serial.println("MQTT: connected");
        client.publish(statusTopic, "online", true);
        client.subscribe(cmdTopic);
    }
    else
    {
        Serial.print("MQTT: failed, rc=");
        Serial.println(client.state());
    }

    return ok;
}
