/******************  LIBRARY SECTION *************************************/
#include <PubSubClient.h>         //https://github.com/knolleary/pubsubclient
#include <ESP8266WiFi.h>          //if you get an error here you need to install the ESP8266 board manager 
#include <ESP8266mDNS.h>          //if you get an error here you need to install the ESP8266 board manager 
#include <ArduinoOTA.h>           //ArduinoOTA is now included with the ArduinoIDE
#include <SimpleTimer.h>          
#include <ArduinoJson.h>          //ArduinoJson

/*****************  START USER CONFIG SECTION *********************************/

#define USER_SSID                 "hahn-2g"
#define USER_PASSWORD             "11ab3f4ef3"
#define USER_MQTT_SERVER          "10.232.0.204"
#define USER_MQTT_PORT            1883
#define USER_MQTT_USERNAME        "mqtt"
#define USER_MQTT_PASSWORD        "mqtt"
#define USER_MQTT_CLIENT_NAME     "mqttBasic"           //used to define MQTT topics, MQTT Client ID, and ArduinoOTA
#define LED_PIN 5                                        //pin where the led strip is hooked up


/*****************  END USER CONFIG SECTION *********************************/

/***********************  WIFI AND MQTT SETUP *****************************/
/***********************  DON'T CHANGE THIS INFO *****************************/

const char* ssid = USER_SSID ; 
const char* password = USER_PASSWORD ;
const char* mqtt_server = USER_MQTT_SERVER ;
const int mqtt_port = USER_MQTT_PORT ;
const char *mqtt_user = USER_MQTT_USERNAME ;
const char *mqtt_pass = USER_MQTT_PASSWORD ;
const char *mqtt_client_name = USER_MQTT_CLIENT_NAME ; 

/*****************  DEFINE JSON SIZE *************************************/

const int capacity = JSON_OBJECT_SIZE(3) + 2 * JSON_OBJECT_SIZE(1);
StaticJsonDocument<capacity> doc;

/*****************  DECLARATIONS  ****************************************/
WiFiClient espClient;
PubSubClient client(espClient);
SimpleTimer timer;

/*****************  GENERAL VARIABLES  *************************************/

bool boot = true;
char charPayload[50];
int pinids[12];
int pinstate[12];
int analogTriggerValue = 5;


void setup_wifi() 
{
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.hostname(USER_MQTT_CLIENT_NAME);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() 
{
  // Loop until we're reconnected
  int retries = 0;
  while (!client.connected()) {
    if(retries < 150)
    {
        Serial.print("Attempting MQTT connection...");
      // Attempt to connect
      if (client.connect(mqtt_client_name, mqtt_user, mqtt_pass)) 
      {
        Serial.println("connected");
        // Once connected, publish an announcement...
        if(boot == true)
        {
          client.publish(USER_MQTT_CLIENT_NAME"/checkIn","Rebooted");
          boot = false;
        }
        if(boot == false)
        {
          client.publish(USER_MQTT_CLIENT_NAME"/checkIn","Reconnected"); 
        }
        client.subscribe(USER_MQTT_CLIENT_NAME"/getpinstatus");
        client.subscribe(USER_MQTT_CLIENT_NAME"/setpinstate");
        client.subscribe(USER_MQTT_CLIENT_NAME"/setpinoption");
      } 
      else 
      {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        retries++;
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
    else
    {
      ESP.restart();
    }
  }
}

void setPinIds()
{
  pinids[0] = D0;
  pinids[1] = D1;
  pinids[2] = D2;
  pinids[3] = D3;
  pinids[4] = D4;
  pinids[5] = D5;
  pinids[6] = D6;
  pinids[7] = D7;
  pinids[8] = D8;
  pinids[9] = D9;
  pinids[10] = A0;
}

/************************** GET DIGITAL STATE **********************/

String getDigitalState()
{
  String statusString = "{";
  statusString = statusString + "D0:" + digitalRead(D0);
  statusString = statusString + ",D1:" + digitalRead(D1);
  statusString = statusString + ",D2:" + digitalRead(D2);
  statusString = statusString + ",D3:" + digitalRead(D3);
  statusString = statusString + ",D4:" + digitalRead(D4);
  statusString = statusString + ",D5:" + digitalRead(D5);
  statusString = statusString + ",D6:" + digitalRead(D6);
  statusString = statusString + ",D7:" + digitalRead(D7);
  statusString = statusString + ",D8:" + digitalRead(D8);
  statusString = statusString + ",D9:" + digitalRead(D9);
  statusString = statusString + ",A0:" + analogRead(A0);
  statusString = statusString + "}";
  
  return statusString;
}

String setPinCurrentState(byte* payload)
{
  DeserializationError err = deserializeJson(doc, payload);

  if (err)
  {
    return "Deserialization Error: " + String((char *)err.c_str());
  } else {
    String newPayloadPin = doc["Pin"];
    String newPayloadState = doc["State"];
    if (newPayloadPin != NULL)
    {
      int pin = pinids[newPayloadPin.toInt()];
    
      if (doc["State"] == "HIGH")
      {
        digitalWrite(pin, HIGH);
      } else if (doc["State"] == "LOW") 
      {
        digitalWrite(pin, LOW);
      } else {
        return "Invalid State: ";
      }
    } else {
      return "Invalid Pin: " + newPayloadPin;
    }

    return "Set Pin State Success: " + newPayloadPin + " - " + newPayloadState;
  }
}

String setPinOption(byte* payload)
{
  DeserializationError err = deserializeJson(doc, payload);

  if (err)
  {
    return "Deserialization Error: " + String((char *)err.c_str());
  } else {
    String newPayloadPin = doc["Pin"];
    String newPayloadOption = doc["Option"];
    if (newPayloadPin != NULL)
    {
      int pin = pinids[newPayloadPin.toInt()];
      
      
      if (doc["Option"] == "INPUT")
      {
        pinMode(pin, INPUT);
      } else if (doc["Option"] == "INPUT_PULLUP") 
      {
        pinMode(pin, INPUT_PULLUP);
      } else if (doc["Option"] == "OUTPUT")
      {
        pinMode(pin, OUTPUT);
      } else {
        return "Invalid Option: ";
      }
    } else {
      return "Invalid Pin: " + newPayloadPin;
    }
      
    return "Set Pin Option Success: " + newPayloadPin + " - " + newPayloadOption;
  }
}

String setPinsCurrentState(byte* payload)
{
  DeserializationError err = deserializeJson(doc, payload);

  if (err)
  {
    return "Deserialization Error: " + String((char *)err.c_str());
  } else {
    String newPayloadPin = doc["Pin"];
    String newPayloadState = doc["State"];
    if (newPayloadPin != NULL)
    {
      int pin = pinids[newPayloadPin.toInt()];
    
      if (doc["State"] == "HIGH")
      {
        digitalWrite(pin, HIGH);
      } else if (doc["State"] == "LOW") 
      {
        digitalWrite(pin, LOW);
      } else {
        return "Invalid State: ";
      }
    } else {
      return "Invalid Pin: " + newPayloadPin;
    }

    return "Set Pin State Success: " + newPayloadPin + " - " + newPayloadState;
  }
}

String setPinsOption(byte* payload)
{
  DeserializationError err = deserializeJson(doc, payload);

  if (err)
  {
    return "Deserialization Error: " + String((char *)err.c_str());
  } else {
    String newPayloadPin = doc["Pin"];
    String newPayloadOption = doc["Option"];
    if (newPayloadPin != NULL)
    {
      int pin = pinids[newPayloadPin.toInt()];
      
      
      if (doc["Option"] == "INPUT")
      {
        pinMode(pin, INPUT);
      } else if (doc["Option"] == "INPUT_PULLUP") 
      {
        pinMode(pin, INPUT_PULLUP);
      } else if (doc["Option"] == "OUTPUT")
      {
        pinMode(pin, OUTPUT);
      } else {
        return "Invalid Option: ";
      }
    } else {
      return "Invalid Pin: " + newPayloadPin;
    }
      
    return "Set Pin Option Success: " + newPayloadPin + " - " + newPayloadOption;
  }
}
/************************** MQTT CALLBACK ***********************/

void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  String newTopic = topic;
  Serial.print(topic);
  Serial.print("] ");
  payload[length] = '\0';
  String newPayload = String((char *)payload);
  int intPayload = newPayload.toInt();
  Serial.println(newPayload);
  Serial.println();
  String stateMessage;
  
  newPayload.toCharArray(charPayload, newPayload.length() + 1); 
  
  if (newTopic == USER_MQTT_CLIENT_NAME"/getpinstatus") 
  {
    stateMessage = getDigitalState();
    stateMessage.toCharArray(charPayload, stateMessage.length() + 1);
    client.publish(USER_MQTT_CLIENT_NAME"/pinstatus", charPayload);
  }

  if (newTopic == USER_MQTT_CLIENT_NAME"/setpinstate")
  {
    stateMessage = setPinsCurrentState(payload);
    stateMessage.toCharArray(charPayload, stateMessage.length() + 1);
    client.publish(USER_MQTT_CLIENT_NAME"/lastAction", charPayload);

    stateMessage = getDigitalState();
    stateMessage.toCharArray(charPayload, stateMessage.length() + 1);
    client.publish(USER_MQTT_CLIENT_NAME"/pinstatus", charPayload);
  }

  if (newTopic == USER_MQTT_CLIENT_NAME"/setpinstate/pin")
  {
    stateMessage = setPinCurrentState(payload);
    stateMessage.toCharArray(charPayload, stateMessage.length() + 1);
    client.publish(USER_MQTT_CLIENT_NAME"/lastAction", charPayload);

    stateMessage = getDigitalState();
    stateMessage.toCharArray(charPayload, stateMessage.length() + 1);
    client.publish(USER_MQTT_CLIENT_NAME"/pinstatus", charPayload);
  }

  if (newTopic == USER_MQTT_CLIENT_NAME"/setpinoption")
  {
    stateMessage = setPinsOption(payload);
    stateMessage.toCharArray(charPayload, stateMessage.length() + 1);
    client.publish(USER_MQTT_CLIENT_NAME"/lastAction", charPayload);

    stateMessage = getDigitalState();
    stateMessage.toCharArray(charPayload, stateMessage.length() + 1);
    client.publish(USER_MQTT_CLIENT_NAME"/pinstatus", charPayload);
  }

  if (newTopic == USER_MQTT_CLIENT_NAME"/setpinoption/pin")
  {
    stateMessage = setPinOption(payload);
    stateMessage.toCharArray(charPayload, stateMessage.length() + 1);
    client.publish(USER_MQTT_CLIENT_NAME"/lastAction", charPayload);

    stateMessage = getDigitalState();
    stateMessage.toCharArray(charPayload, stateMessage.length() + 1);
    client.publish(USER_MQTT_CLIENT_NAME"/pinstatus", charPayload);
  }
  
}


void setup() 
{
  Serial.begin(115200);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.mode(WIFI_STA);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  ArduinoOTA.setHostname(USER_MQTT_CLIENT_NAME);
  ArduinoOTA.begin(); 
  timer.setInterval(10000);

  setPinIds();

}

int isInput(int pin)
{
//  uint8_t bit = digitalPinToBitMask(pin);
//  uint8_t port = digitalPinToPort(pin);
//  uint8_t *reg = portModeRegister(port);
  
//  if (*reg & bit) {
//      return false;
//  } else {
//      return true;
//  }
}

bool check_pins()
{
  bool foundChange = false;
  String stateMessage;
  
  for (int l=0; l<10; ++l)
  {
    if (pinstate[l] != digitalRead(pinids[l]))
    {
      foundChange = true;
      pinstate[l] = digitalRead(pinids[l]);

      stateMessage = "P" + String(l) + " Changed State To: " + digitalRead(pinids[l]);
      stateMessage.toCharArray(charPayload, stateMessage.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/actionalert", charPayload);
      
    }
  }

  // Check the analog pin
  //if ((analogRead(A0) >= (pinstate[10] + analogTriggerValue)) || (analogRead(A0) <= (pinstate[10] - analogTriggerValue)))
  //{
    /*foundChange = true;
    pinstate[10] = analogRead(A0);

    stateMessage = "A0 Changed Value To: " + analogRead(A0);
    stateMessage.toCharArray(charPayload, stateMessage.length() + 1);
    client.publish(USER_MQTT_CLIENT_NAME"/actionalert", charPayload);*/
    
  //}
  
  if (foundChange)
  {
    stateMessage = getDigitalState();
    stateMessage.toCharArray(charPayload, stateMessage.length() + 1);
   client.publish(USER_MQTT_CLIENT_NAME"/pinstatus", charPayload);
  }
}

void loop() 
{
  if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();
  ArduinoOTA.handle();

  
  
  if(boot == false)
  {
    if (check_pins())
    {
      
    }

    if (timer.isReady())
    {
      String stateMessage = "Alive";
      stateMessage.toCharArray(charPayload, stateMessage.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/state", charPayload);

      
      stateMessage = WiFi.localIP().toString();
      stateMessage.toCharArray(charPayload, stateMessage.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/ipaddress", charPayload);

      timer.reset();
    }
  }
}
