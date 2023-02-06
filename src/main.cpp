#include <Arduino.h>
#include <main.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

// #define RECEIVER_VARIANT

#ifndef RECEIVER_VARIANT
#define SENDER_VARIANT
#endif

#define MOTION_PIN 0
#define MOTION_PIN_LED 2
#define LED1_PIN 15
#define LED2_PIN 13

// receiver mac 5C:CF:7F:11:3A:B2
// sender mac 5C:CF:7F:0B:94:F9
#ifdef SENDER_VARIANT
u8 destionationAddress[] = {0x5c, 0xCf, 0x7F, 0x11, 0x3A, 0xB2};
#else
#endif
u8 new_data = 0;

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message
{
  unsigned int counter1;
  unsigned int counter2;
  int motion;
  uint8_t deviceMAC[6];

  String toString()
  {
    String result = "";
    result += "MAC: ";
    result += MAC_toString(deviceMAC);
    result += "\r\ncounter1: ";
    result += String(counter1);
    result += "\r\ncounter2: ";
    result += String(counter2);
    result += "\r\nmotion: ";
    result += String(motion);
    return result;
  }
} struct_message;

struct_message data_to_send;
struct_message incomingReadings;

unsigned int unsuccesfull_tx = 0;
unsigned int unsuccesfull_ack = 0;

void OnDataSent(unsigned char *mac_addr, unsigned char status)
{
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == 0 ? "Delivery Success" : "Delivery Fail");
  if (status != 0)
    unsuccesfull_ack++;
}

String MAC_toString(uint8_t *mac)
{
  String result = "";
  for (int i = 0; i < 6; ++i)
  {
    result += String(mac[i], 16);
    if (i < 5)
    {
      result += ':';
    }
  }
  return result;
}

void OnDataRecv(u8 *mac, u8 *incomingData, u8 len)
{
  new_data = len;
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
}

void setup()
{
  Serial.begin(115200);
  Serial.print("Starting as");
#ifdef SENDER_VARIANT
  Serial.println(" sender");
#else
  Serial.println(" receiver");
#endif
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
  if (esp_now_add_peer(destionationAddress, esp_now_role::ESP_NOW_ROLE_SLAVE, 0, NULL, 0) != 0)
  {
    Serial.println("Failed to add peer");
    return;
  }
  pinMode(MOTION_PIN, INPUT_PULLUP); // GPIO0 btn
  data_to_send.counter1 = unsuccesfull_ack;
  data_to_send.counter2 = unsuccesfull_tx;
  WiFi.macAddress(data_to_send.deviceMAC);
  Serial.print("Sender mac: ");
  Serial.println(MAC_toString(data_to_send.deviceMAC));
#else
  pinMode(MOTION_PIN_LED, OUTPUT);
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
#endif
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
}

int cnt = 0;
void loop()
{
#ifdef SENDER_VARIANT
  Serial.print("loop");
  Serial.println(cnt++);

  u8 success = data_to_send.counter1 == unsuccesfull_ack ? 0 : 1;

  data_to_send.motion = digitalRead(MOTION_PIN);
  data_to_send.counter1 = unsuccesfull_ack;
  data_to_send.counter2 = unsuccesfull_tx;
  int result = esp_now_send(destionationAddress, (u8 *)&data_to_send, sizeof(data_to_send));
  Serial.print("Sending data: ");
  Serial.println(data_to_send.toString());

  if (result == 0)
  {
    Serial.println("Sent with success");
  }
  else
  {
    unsuccesfull_tx++;
    success++;
    Serial.println("Error sending the data");
  }
  if (success != 0)
  {
    digitalWrite(LED1_PIN, HIGH);
    delay(100);
    digitalWrite(LED1_PIN, LOW);
    delay(900);
  }
  else
    delay(1000);
#else

  if (new_data > 0)
  {
    digitalWrite(LED1_PIN, HIGH);
    Serial.print("Bytes received: ");
    Serial.println(new_data);
    Serial.println(incomingReadings.toString());
    digitalWrite(MOTION_PIN_LED, incomingReadings.motion > 0 ? LOW : HIGH);
    new_data = 0;
    delay(100);
    digitalWrite(LED1_PIN, LOW);
  }
#endif
}