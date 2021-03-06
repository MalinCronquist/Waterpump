#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>


// Replace with your network credentials
const char* ssid = "Jarnsignal";
const char* password = "jarnaker";
const char* mqttServer = "192.168.68.112";
const int mqttPort = 1883;

const int analogInPin = A0;
const int pumpPin = 4;

long stopTime = 0;        

unsigned long previousMillis = 0;
unsigned long interval = 3000;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);

  pinMode(pumpPin, OUTPUT);     // Initialize the pin as an output
  pinMode(pumpPin, LOW);        // Start turned off

  WiFi.begin(ssid, password);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  Serial.println(WiFi.localIP());

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");

    if (client.connect("ESP8266Client" )) {

      Serial.println("connected");


    } else {

      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);

    }
  }

  client.subscribe("esp/test");
  client.publish("esp/test", "Hello from ESP8266");

  client.subscribe("pump_water");
  client.subscribe("measure_soil");
  client.subscribe("update_status");

}

char* preparePayload(String message) {
  int message_len = message.length() + 1;

  char message_array[message_len];
  message.toCharArray(message_array, message_len);
  Serial.print("Original: " + message);
  Serial.print("New: " + String(message_array));
  return message_array;
}

void callback(char* topic, byte* payload, unsigned int length) {

  Serial.print("Message arrived in topic: ");
  Serial.println(topic);

  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }

  payload[length] = '\0';

  if (String(topic) == "pump_water") {
    String str_val = (char*)payload;
    int time = str_val.toInt();
    Serial.println(str_val + " seconds");

    if (time == -1) {
      stopTime = 0;
    } else {
      stopTime = millis() + time*1000;
    }
  } else if (String(topic) == "measure_soil") {
    measure_soil();
  } else if (String(topic) == "update_status") {
    update_status();
  }

  Serial.println();
  Serial.println("-----------------------");
}

void measure_soil() {
  int sensorValue = analogRead(A0);
  String val = String(map(sensorValue, 0, 1023, 0, 255));
  client.publish("soil_value", preparePayload(val));
}

void update_status(){
  client.publish("status_message", "");
}

int pumpStatus() {
  if (stopTime <  millis()) {
    return 0; // Not pumping water
  } else {
    return 1; // Pumping water
  }
}

boolean pumpRunning = false;

void handlePump() {
  // Pump status is what it should be.
  // Basically, if they differ we take some action.
  
  if (pumpStatus() == 0 && pumpRunning) {
    String pump_done = "Pump cancelled";
    client.publish("pump_done", preparePayload(pump_done));
    pinMode(pumpPin, LOW);     // Turn off pump
    pumpRunning = false;
    
  } else if (pumpStatus() == 1 && !pumpRunning) {
    pinMode(pumpPin, HIGH);     // Turn on pump
    pumpRunning = true;
  }
}

void loop() {
    unsigned long currentMillis = millis();
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    ESP.restart();
    //WiFi.disconnect();
    //WiFi.begin(ssid, password);
   // previousMillis = currentMillis;
   // Serial.println(WiFi.localIP());
  }
  
  client.loop();
  handlePump();
}
