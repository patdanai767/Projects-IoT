//เพิ่ม Servo 3 ตัว, เพิ่ม loadcell 1 ตัว
//1.) ขวด/กระป๋อง มีน้ำ D2 -> D0
//2.) กระป๋อง ไม่มีน้ำ D2 -> D1
//3.) ขวด มีน้ำ D2
//ตรวจว่ามีวัตถุ -> ช่างน้ำหนัก -> แยกขวด/กระป๋อง

//D2,D0,D1,D5,D6,A0,VIN,GND
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h>
#include <HX711_ADC.h>

Servo myservoD2;  //ประกาศตัวแปรแทน Servo ปัดลงเฉยๆ
Servo myservoD0;  //ประกาศตัวแปรแทน Servo เปิดช่อง Bottles
Servo myservoD1;  //ประกาศตัวแปรแทน Servo เปิดช่อง Cans
long duration;
int distance, can = 0, bottle = 0, weightCan = 0, weightBottle = 0;
const char* ssid = "PATR-NOTEBOOK 6371";   //อย่าลืมแก้ไข SSID ของ WIFI ที่จะให้ NodeMCU ไปเชื่อมต่อ
const char* password = "Hj9]8389";  //อย่าลืมแก้ไข PASSWORD ของ WIFI
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_Client = "client001_peeranat";                             //อย่าลืมแก้ไข ClientID
const char* mqtt_username = "Imrg51JeZZEX2QHKTZVXpr6eycTFhApNtlg9mEyZWfw";  //อย่าลืมแก้ไข Token
const char* mqtt_password = "";                                             //อย่าลืมแก้ไข Secret
const int LOADCELL_DOUT_PIN = D5;                                           // ของ LoadCell
const int LOADCELL_SCK_PIN = D6;                                            // ของ LoadCell
float loadcell;
int weightB;
int weightC;

HX711_ADC scale(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
WiFiClient espClient;
PubSubClient client(espClient);

int wb = 0;
int wc = 0;
long lastMsg = 0;
char msg[200];
String DataString;
const int LED = 13;
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_Client, mqtt_username, mqtt_password)) {  //เชื่อมต่อกับ MQTTBROKER
      Serial.println("connected");
      client.subscribe("@msg/operator");
    } else {
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
  String message;
  for (int i = 0; i < length; i++) {
    message = message + char(payload[i]);
  }
}

void setup() {
  myservoD2.attach(4);   // กำหนดขา D2 ควบคุม Servo
  myservoD0.attach(16);  // กำหนดขา D0 ควบคุม Servo
  myservoD1.attach(5);   // กำหนดขา D1 ควบคุม Servo
  myservoD2.write(180);  // ให้ Servo หมุนไปที่ 90 องศา
  myservoD0.write(174);
  myservoD1.write(170);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, 1);

  scale.begin();
  scale.start(1000);
  scale.setCalFactor(-378.5186567164179);

  Serial.begin(115200);
  WiFi.begin(ssid, password);  //เชื่อมต่อกับ WIFI
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());            //เชื่อมต่อกับ WIFI สำเร็จ แสดง IP
  client.setServer(mqtt_server, mqtt_port);  //กำหนด MQTT BROKER, PORT ที่ใช้
  client.setCallback(callback);              //ตั้งค่าฟังก์ชันที่จะทำงานเมื่อมีข้อมูลเข้ามาผ่านการ Subscribe
  client.subscribe("@msg/operator");
}

void waitupdate(int eiei) {
  for (int i = 0; i < eiei; i++) {
    scale.update();
    loadcell = scale.getData();
    Serial.println(loadcell);
    delay(1);
  };
}

void calibra() {

  loadcell = scale.getData();
  Serial.println("kuy.");
  scale.tareNoDelay();
  waitupdate(1000);
}

void loop() {
  if (!client.connected()) {
    Serial.println("MQTT connection...");
    reconnect();
  }
  client.loop();
  int sensorValue = analogRead(A0);  // อ่านค่าที่ได้จาก Sensor ตรวจจับโลหะ
  scale.update();
  loadcell = scale.getData();
  Serial.println(loadcell);

  if (loadcell > 4) {
    digitalWrite(LED, 0);
    waitupdate(1000);


    if (sensorValue > 600 && loadcell >= 30)  //metal have water
    {

      Serial.print("Have a can of water\n");
      calibra();
      myservoD2.write(0);
      myservoD1.write(0);
      delay(1000);
      myservoD2.write(180);
      delay(1000);
      myservoD1.write(170);
      wc++;
    } else if (sensorValue < 600 && loadcell >= 60)  //bottle have water
    {

      Serial.print("Have a bottle of water\n");
      if (loadcell > 100) {
        calibra();
      } else {
        calibra();
      }
      myservoD2.write(0);
      myservoD1.write(0);
      delay(1000);
      myservoD2.write(180);
      delay(1000);
      myservoD1.write(170);
      wb++;
    } else if (sensorValue < 600 && loadcell <= 60 && loadcell > 10)  //bottle no water
    //ตรวจสอบว่า Sensor ทั้ง 2 ตัววัดค่าได้ตรงตามที่กำหนดหรือไม่(ไม่เป็นโลหะ)
    {
      weightBottle += loadcell;
      weightB = loadcell;
      calibra();
      Serial.println("Not Metal No water\n");
      Serial.print("Loadcell : ");
      Serial.println(loadcell);
      myservoD2.write(0);
      myservoD0.write(0);
      delay(1000);
      myservoD2.write(180);
      delay(1000);
      myservoD0.write(174);
      Serial.print("All weight");
      Serial.println(weightBottle);

      bottle++;
    } else if (sensorValue > 600 && loadcell <= 30 && loadcell > 5)  //metal no water
    //ตรวจสอบว่า Sensor ทั้ง 2 ตัววัดค่าได้ตรงตามที่กำหนดหรือไม่(เป็นโลหะ)
    {
      weightCan += loadcell;
      weightC = loadcell;
      calibra();
      Serial.println("Metal No water\n");
      Serial.print("Loadcell : ");
      Serial.println(loadcell);
      myservoD2.write(0);
      delay(500);
      myservoD2.write(180);
      Serial.print("All weight : ");
      Serial.println(weightCan);
      can++;
    }
  } else if (loadcell <= -5) {
    calibra();
    waitupdate(2000);
  }


  delay(50);

  long now = millis();
  if (now - lastMsg > 5000) {  //จับเวลาส่งข้อมูลทุก ๆ 5 วินาที
    lastMsg = now;
    DataString = "{\"Bottle\":" + (String)bottle + ",\"Can\":" + (String)can + ",\"Weight_B\":" + (String)weightB + ",\"Weight_C\":" + (String)weightC + ",\"WeightBottles\":" + (String)weightBottle + ",\"WeightCans\":" + (String)weightCan + ",\"WaterBottle\":" + (String)wb + ",\"WaterCan\":" + (String)wc + "}";
    // Example of data : {"data":{"temperature":25 , "humidity": 60}}
    weightB = 0;
    weightC = 0;
    DataString.toCharArray(msg, 200);
    client.publish("peeranat/out1", msg);  //ส่งข้อมูลไปยัง Real Time Database(Shadow)
    delay(1);
  }

  delay(1);
}