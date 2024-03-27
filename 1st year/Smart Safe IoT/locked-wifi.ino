#include <Wire.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Keypad_I2C.h>
#include <Keypad.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define I2CADDR 0x20
#define relay_pin 16
#define led D8
int i = 0;

WiFiClient espClient; 
PubSubClient client(espClient);

const char* ssid = "Paipai"; 
const char* password = "////////"; 
const char* mqtt_server = "broker.hivemq.com"; 
const int mqtt_port = 1883;
const char* mqtt_Client = "MQTT_CLIENT_NA"; 
const char* mqtt_username = "MQTT_TOKEN_NA"; 
const char* mqtt_password = "MQTT_SECRET_NA"; 

long lastMsg = 0;
int value = 0;
char msg[100];
char lock_status_on[5] = "ON";
char lock_status_off[5] = "OFF";
String DataString;

String keyinput = "";
byte keyindex = 0;
byte faildelay = 0;

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
  if (client.connect(mqtt_Client, mqtt_username, mqtt_password)) {
    Serial.println("connected");
    client.subscribe("patrdanai/status");
  }
  else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println("try again in 5 seconds");
    delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.print(length);
  String message;
  for (int j = 0; j < length; j++) {
    message = message + char(payload[j]);
    Serial.print(payload[j]);
  }
  Serial.println(message);
  if(String(topic) == "patrdanai/status") {
    if (message == "true"){
      digitalWrite(relay_pin,HIGH);
      int status = 1;
      DataString = "{\"status_relay\":"+(String)status+"}"; 
      DataString.toCharArray(msg, 100);
      client.publish("patrdanai/status_relay", msg);
      //client.publish("@shadow/data/update", "{\"data\" : {\"led\" : \"on\"}}");
      Serial.println("Locked : OPEN"); 
      // delay(5000);
      // digitalWrite(relay_pin,LOW);
      // Serial.println("Locked : CLOSE"); 
    }
    else if (message == "false") {
      
      i = 0;
      digitalWrite(relay_pin,LOW);
      int status = 0;
      DataString = "{\"status_relay\":"+(String)status+"}"; 
      DataString.toCharArray(msg, 100);
      client.publish("patrdanai/status_relay", msg);
      //client.publish("@shadow/data/update", "{\"data\" : {\"led\" : \"off\"}}");
      Serial.println("Locked : CLOSE"); }
    } 
}

const byte ROWS = 4;
const byte COLS = 3;
//int led = D8;
int status;

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[ROWS] = {0, 1, 2, 3};
byte colPins[COLS] = {4, 5, 6};

String password_lock = "1234";

// unsigned long time2;
Keypad_I2C keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS, I2CADDR);

void setup() {
  pinMode(led,OUTPUT);
  Wire.begin( );
  keypad.begin( );
  Serial.begin(115200);
  lcd.begin();
  lcd.display();
  lcd.backlight();
  lcd.clear();
  pinMode(relay_pin, OUTPUT);
  digitalWrite(relay_pin, LOW);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password); //เชื่อมต่อกับ WIFI
  WiFi.setHostname("SmartSafe IOT");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); 
  client.setServer(mqtt_server, mqtt_port); 
  client.setCallback(callback); 
  client.publish("patrdanai/status","KUYYYYYY");
  client.subscribe("patrdanai/status");
  // time2 = millis();
}
void loop(){
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  long now = millis();
  lcd.setCursor(2, 0);
  lcd.print(">>> LOCK <<<");
  lcd.setCursor(0, 1);
  lcd.print("PASSWORD : ");
  // digitalWrite(led,LOW);
  for (int d = 0; d < 4; d++) {
    if (d >= keyindex) {
      break;
    }
    lcd.setCursor(d + 11, 1);
    lcd.print("*");
  }
  char key = keypad.getKey();
  if (key != NO_KEY) {
    // time2 = millis();
    lcd.backlight();
    lcd.display();
    keyinput += key;
    keyindex++;
    Serial.println(keyinput);
    if (keyindex > 3) {
      
        DataString = "{\"keyinput\":"+(String)keyinput+",\"status_relay\":"+(String)status+"}"; 
        DataString.toCharArray(msg, 100);
        Serial.println("============");
        Serial.println(msg);
        Serial.println("============");
        client.publish("patrdanai/keys", msg);
        delay(1);

      if (keyinput == password_lock) {
      
        
        faildelay = 0;
        lcd.clear();
        lcd.setCursor(1, 0);
        lcd.print(" >> UNLOCK << ");
        Serial.println("Unlock");
        digitalWrite(relay_pin, HIGH);

        status = 1;
        DataString = "{\"status_relay\":"+(String)status+"}"; 
        DataString.toCharArray(msg, 100);
        client.publish("patrdanai/status_relay", msg);
        Serial.print("Status : ");
        Serial.println(status);
        keyinput = "";
        keyindex = 0;
        // digitalWrite(led,HIGH);
        delay(5000);
        lcd.clear();
        Serial.println("lock");
        digitalWrite(relay_pin, LOW);
        status = 0;
        DataString = "{\"status_relay\":"+(String)status+"}"; 
        DataString.toCharArray(msg, 100);
        client.publish("patrdanai/status_relay", msg);
        Serial.print("Status : ");
        Serial.println(status);
        // Serial.print(led);
      }
      else {
        Serial.println("ISUS");
        Serial.println(keyinput);
        keyinput = "";
        keyindex = 0;
        lcd.clear();
        if (faildelay >= 2) {
          faildelay = 1000;
          lcd.setCursor(1, 0);
          lcd.print(" >> LOCKED << ");
          // lcd.setCursor(0, 1);
          // lcd.print("TIME COUNT : ");
          Serial.println("Password fail");
          Serial.println("Fail delay " + String(faildelay) + " S");
          for (i = faildelay; i >= 0; i--) {
            if (!client.connected()) {
                reconnect();
                }
                client.loop();
                callback;
            // lcd.setCursor(13, 1);
            // lcd.print("   ");
            // lcd.setCursor(13, 1);
            // lcd.print(i);
            // Serial.println(i);

            delay(1000);
          }
          lcd.clear();
          // time2 = millis();
          faildelay -= 100;
          Serial.println("Delay End");
        }
        else {
          faildelay++;
        }
      }
    }
  }
}