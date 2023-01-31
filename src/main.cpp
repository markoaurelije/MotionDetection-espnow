#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

// #define RECEIVER_VARIANT

#ifndef RECEIVER_VARIANT
#define SENDER_VARIANT
#endif

#define MOTION_PIN 0
#define MOTION_PIN_LED 2
#define RX_LED_PIN 15

// Variable to store if sending data was successful
String success;

// receiver mac 5C:CF:7F:11:3A:B2
// sender mac 5C:CF:7F:0B:94:F9
u8 broadcastAddress[] = {0x5c, 0xCf, 0x7F, 0x11, 0x3A, 0xB2};

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message
{
  float temp;
  float hum;
  int motionDetected;
} struct_message;

struct_message data_to_send;
struct_message incomingReadings;

void OnDataSent(unsigned char *mac_addr, unsigned char status)
{
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == 0 ? "Delivery Success" : "Delivery Fail");
  if (status == 0)
  {
    success = "Delivery Success :)";
  }
  else
  {
    success = "Delivery Fail :(";
  }
}

void OnDataRecv(u8 *mac, u8 *incomingData, u8 len)
{
  digitalWrite(RX_LED_PIN, HIGH);
  Serial.print("Bytes received: ");
  Serial.println(len);
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));

  Serial.print("temperature");
  Serial.println(incomingReadings.temp);
  Serial.print("humidity");
  Serial.println(incomingReadings.hum);
  Serial.print("Motion");
  Serial.println(incomingReadings.motionDetected);
  digitalWrite(MOTION_PIN_LED, incomingReadings.motionDetected > 0 ? LOW : HIGH);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting");
  WiFi.mode(WIFI_STA);
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  WiFi.disconnect();

  // Init ESP-NOW
  if (esp_now_init() != 0)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

#ifdef SENDER_VARIANT
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);

  // Add peer
  if (esp_now_add_peer(broadcastAddress, esp_now_role::ESP_NOW_ROLE_SLAVE, 0, NULL, 0) != 0)
  {
    Serial.println("Failed to add peer");
    return;
  }
  pinMode(MOTION_PIN, INPUT_PULLUP); // GPIO0 btn
  data_to_send.temp = 25.5;
  data_to_send.hum = 50.5;
#else
  pinMode(MOTION_PIN_LED, OUTPUT);
  pinMode(RX_LED_PIN, OUTPUT);
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
#endif
}

int cnt = 0;
void loop()
{
#ifdef SENDER_VARIANT
  data_to_send.motionDetected = digitalRead(MOTION_PIN);
  int result = esp_now_send(broadcastAddress, (u8 *)&data_to_send, sizeof(data_to_send));

  data_to_send.temp += 0.1;

  if (result == 0)
  {
    Serial.println("Sent with success");
  }
  else
  {
    Serial.println("Error sending the data");
  }
#else
  digitalWrite(RX_LED_PIN, LOW);
#endif

  Serial.print("loop");
  Serial.println(cnt++);
  delay(1000);
}