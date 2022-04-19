#include "DHT.h"
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <Wire.h>

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

#define DHTPIN 44          // what digital pin we're connected to
#define DHTTYPE DHT11     // DHT11
DHT dht(DHTPIN, DHTTYPE);

// #ETHERNET MQTT DATA
const char* username = "raxat"; // my AskSensors username
const char* pubTopic = "publish/raxat/almga4E03n9X013TRWNew96492gtXFoP"; // publish/username/apiKeyIn
const unsigned int writeInterval = 90; // write interval (in ms)config
const char* mqtt_server = "mqtt.asksensors.com";
unsigned int mqtt_port = 1883;
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

IPAddress ip(185,48,148,198); // TODO: Add the IP address
EthernetClient askClient;
PubSubClient client(askClient);

// 1: Sensors:
// OUTSIDE: 
const int motionSensor = 23;
const int photoSensor = A0;
const int doorSwitch = 3;
const int lightSwitch = 2;
Servo door;

//INSIDE:
const int humSensor = 22;
const int flameSensor = 47;
const int gasSensor = 7;
const int buzzer = 31;
const int sensorMin = 0;     // sensor minimum
const int sensorMax = 1024;  // sensor maximum

// 2: LEDS
const int switchPin = 42;
const int outsideLights = 46;
const int insideLights1 = 12;
const int insideLights2 = 13;


// Display:
LiquidCrystal_I2C lcd(0x27,16, 2  );

const byte COLOR_BLACK = 0b000;
const byte COLOR_RED = 0b100;
const byte COLOR_GREEN = 0b010;
const byte COLOR_BLUE = 0b001;
const byte COLOR_MAGENTA = 0b101;
const byte COLOR_CYAN = 0b011;
const byte COLOR_YELLOW = 0b110;
const byte COLOR_WHITE = 0b111;


const byte PIN_LED_R = 36;
const byte PIN_LED_G = 38;
const byte PIN_LED_B = 40;

void setup() {
  // Sensors:
  pinMode(motionSensor, INPUT);
  pinMode(photoSensor, INPUT);
  pinMode(humSensor, INPUT);
  pinMode(flameSensor, INPUT);
  pinMode(doorSwitch, INPUT);
  pinMode(lightSwitch, INPUT);

  // LEDS; 
  pinMode(outsideLights, OUTPUT);
  pinMode(insideLights1, OUTPUT);
  pinMode(insideLights2, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  pinMode(switchPin, INPUT);

  door.attach(9); 
  
  dht.begin();

  Serial.begin(9600);
  lcd.init();                      // initialize the lcd 
  lcd.init();
  // Print a message to the LCD.
  lcd.backlight();
  lcd.setCursor(0,0);
  printHello();  
  Serial.println("*****************************************************");
  Serial.println("********** Program Start : Arduino Ethernet publishes data to AskSensors over MQTT");
  Serial.print("********** connecting to Ethernet : ");
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  // give the Ethernet shield a second to initialize:
  delay(1000);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected())
    reconnect();
    client.loop();
    Serial.println("********** Publish MQTT data to ASKSENSORS");
    char mqtt_payload[30] = "";
    snprintf (mqtt_payload, 100, "m1=%d&m2=%d&m3=%d&m4=%d", getBinaryValue(gasSensor), getBinaryValue(flameSensor), (int)dht.readTemperature(), (int)dht.readHumidity());
    Serial.print("Publish message: ");
    Serial.println(mqtt_payload);
    Serial.print("Flame Sensor: ");

    int flameSensorReading = getBinaryValue(flameSensor);
    int gasSensorReading = getBinaryValue(gasSensor);
    Serial.println(flameSensorReading);
    Serial.print("Gas Sensor: ");
    Serial.println(gasSensorReading);

    if(flameSensorReading == 0 && gasSensorReading == 1) {
      digitalWrite(buzzer, LOW);
    } else {
      digitalWrite(buzzer, HIGH);
    }

    controlDoor();
    controlLights();
    

    float humi = dht.readHumidity();
    float tempC = dht.readTemperature();
    float tempF = dht.readTemperature(true);

    if (isnan(humi) || isnan(tempC) || isnan(tempF))
    {
      Serial.println("Failed to read from DHT sensor!");
    }

    showLights();
    readHumidity();
    
    Serial.print("Humidity: ");Serial.print(humi);Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(tempC);Serial.print(" *C ");
    Serial.print(tempF);Serial.println(" *F");
  
    client.publish(pubTopic, mqtt_payload);
    Serial.println("> MQTT data published");
    Serial.println("********** End ");
    Serial.println("*****************************************************");
    delay(writeInterval);// delay
  
}

void displayColor(byte color) {
  digitalWrite(PIN_LED_R, !bitRead(color, 2));
  digitalWrite(PIN_LED_G, !bitRead(color, 1));
  digitalWrite(PIN_LED_B, !bitRead(color, 0));
}

void showLights() {
   int photoVal = analogRead(photoSensor);
   Serial.print("Photo Sensor: ");
   Serial.println(photoVal);

   int motionVal = digitalRead(motionSensor);
   Serial.print("Motion Sensor: ");
   Serial.println(motionVal);
  
   if(photoVal >= 400 && motionVal == HIGH)
      digitalWrite(outsideLights, LOW);
   else
      digitalWrite(outsideLights, HIGH);
}

void readHumidity(){
//  int chk = DHT.read11(humSensor);
  Serial.print("Temperature = ");
//  Serial.println(DHT.temperature);
  Serial.print("Humidity = ");
//  Serial.println(DHT.humidity);
}

void printHello(){
  String hello = "Hello World!";
  for(int i = 0; i < hello.length(); i++){
    lcd.setCursor(i, 0);
    lcd.print(hello[i]);  
  }   
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  }

void reconnect() {
// Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("********** Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ethClient",username, "")) {
    Serial.println("-> MQTT client connected");
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println("-> try again in 5 seconds");
    // Wait 5 seconds before retrying
    delay(5000);
    } 
  }
}

int getBinaryValue(int sensor){
  if(digitalRead(sensor) == HIGH){
    return 1;  
  }  else {
    return 0;
  }
}

void controlDoor(){
  if(digitalRead(doorSwitch) == HIGH) {
    door.write(180);  
  } else {
    door.write(0);
  }
}

void controlLights(){
  if(digitalRead(lightSwitch) == HIGH) {
    digitalWrite(insideLights1, HIGH);
    digitalWrite(insideLights2, HIGH);  
  } else {
    digitalWrite(insideLights1, LOW);
    digitalWrite(insideLights2, LOW);  
  }  
}
