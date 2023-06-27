#include <DHT.h>              // header file for humidity and temperature sensor
#include <ESP8266WiFi.h>      //header file for wifi module
#include <WiFiClient.h>
#include <ThingSpeak.h>       //header file for thingspeak
#include <SoftwareSerial.h>

#define DHTPIN 3              // pin to which DHT11 data pin is connected
#define DHTTYPE DHT11         // DHT11 sensor model
DHT dht(DHTPIN, DHTTYPE);     // creating a DHT object

SoftwareSerial espSerial(6,7); // RX, TX pins for ESP8266

// WiFi settings
const char* ssid = "WIT-WIFI";
const char* password = "$wit$123";
String PORT = "80";

// ThingSpeak settings
const char* server = "api.thingspeak.com";
const char* apiKey = "RA4M8OZ4B03633UC";
const unsigned long channelID = 2161727;

int moisturePin = A0;         // analog input pin A0 for soil moisture sensor

const int flowSensorPin = 5 ;  // pin for the flow sensor
volatile int flowSensorInterruptCount = 0; // Initialize the flow sensor interrupt count

int mqPin = A0;               // Analog input pin for MQ sensor
int ledPin = 12;              // On-board LED pin for mq sensor

// pins for ultrasonic sensors
const int trigPin = 9;
const int echoPin = 10;
// variables for duration and distance measurements using ultrasonic sensor
long duration;
int waterLevel;

WiFiClient client;             // Initialize the client and Wi-Fi

void setup()
{
  Serial.begin(9600);          // Initialize serial communication at 9600 baud
  espSerial.begin(9600);

  WiFi.begin(ssid, password);  // Connect to Wi-Fi
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");

  sendCommand("AT+RST", "Ready");
  sendCommand("AT+CWMODE=1", "OK");
  connectToWiFi();

  ThingSpeak.begin(client);    // Initialize ThingSpeak

  dht.begin();                 //initialize DHT sensor

  pinMode(flowSensorPin, INPUT); // Set the flow sensor pin as input
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), flowSensorInterrupt, RISING); // Set the interrupt for the flow sensor pin
  
  pinMode(ledPin, OUTPUT);     // Set LED pin as output of mq sensor

  // setup pins for ultrasonic sensors
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

void loop()
{
  delay(2000);                // Wait for 2 seconds before reading data

  int moistureValue = analogRead(moisturePin); // Read soil  moisture sensor value
  // Print soil moisture value to serial monitor
  Serial.print("Soil Moisture: ");
  Serial.print(moistureValue);
  
  // reading temperature and humidity using DHT sensor
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  // Print temperature and humidity to the serial monitor
  Serial.print("\nTemperature: ");
  Serial.print(temperature);
  Serial.print(" °C \n Humidity: ");
  Serial.print(humidity);
  Serial.println("%");
  
  float flowRate = (flowSensorInterruptCount / 7.5); // Calculate the flow rate (7.5 is the number of pulses per liter for the flow sensor)
  // print flow rate to serial monitor 
  Serial.print("Flow rate: ");
  Serial.print(flowRate);
  Serial.println(" L/min");
  flowSensorInterruptCount = 0;     // Reset the flow sensor interrupt count
  
  int mqValue = analogRead(mqPin); // Read analog value from MQ sensor
  // Print mq sensor value to serial monitor
  Serial.print("MQ Sensor value: ");
  Serial.println(mqValue); 
  if (mqValue > 200 && mqValue <1000)
  { 
    // If mq sensor value is above threshold
    digitalWrite(ledPin, HIGH);    // Turn on LED
    Serial.print("Warning : There may be a fire");  
  }
  else
  { 
    digitalWrite(ledPin, LOW);     // Turn off LED
    Serial.print("There is no fire");
  }    
  
  // send a 10 microsecond pulse to trigger pin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = duration * 0.034/2;        // measure duration of echo pulse using ultrasonic sensors
  // print distance measured by ultrasonic sensor to serial monitor
  Serial.print("Water Level: ");
  Serial.print(waterLevel);
  Serial.println(" cm");

  // Update ThingSpeak
  ThingSpeak.writeField(channelID, 1, moistureValue, apiKey);
  ThingSpeak.writeField(channelID, 2, temperature, apiKey);
  ThingSpeak.writeField(channelID, 3, humidity, apiKey);
  ThingSpeak.writeField(channelID, 4, flowRate, apiKey);
  ThingSpeak.writeField(channelID, 5, mqValue, apiKey);
  ThingSpeak.writeField(channelID, 6, waterLevel, apiKey);

  Serial.println(); // Print new line  

  delay(10000);                        // Delay for a short time before reading data again
}

void flowSensorInterrupt() 
{
  flowSensorInterruptCount++;         // Increment the flow sensor interrupt count
}

void sendCommand(String command, String expectedResponse) 
{
  espSerial.println(command);
  delay(1000);
  while (espSerial.available()) 
  {
    String response = espSerial.readStringUntil('\n');
    if (response.indexOf(expectedResponse) != -1) 
    {
      break;
    }
  }
}

void connectToWiFi() 
{
  sendCommand("AT+CWJAP=\"" + String(ssid) + "\",\"" + String(password) + "\"", "OK");
  Serial.println("Connected to Wi-Fi!");
}