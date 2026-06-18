#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "time.h"

/* ---------------- WIFI DETAILS ---------------- */

const char* ssid = "WiFi Name";
const char* password = "WiFi Password";

String botToken = "YOUR_BOT_TOKEN";

/* First Person Chat ID */
String chatID1 = "FIRST_PERSON_CHAT_ID";

/* Second Person Chat ID */
String chatID2 = "SECOND_PERSON_CHAT_ID";

/* ---------------- PIN CONFIGURATION ---------------- */

const int ldrPin = 34;
const int buzzer = 25;
const int ledPin = 2;
const int laserPin = 26;

/* ---------------- SETTINGS ---------------- */

int threshold = 1800;
bool alertSent = false;

unsigned long lastCheckTime = 0;
bool safeMessageSent = false;

/* ---------------- TIME SERVER ---------------- */

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;
const int daylightOffset_sec = 0;

/* ---------------- FUNCTION TO SEND TELEGRAM MESSAGE ---------------- */

void sendTelegramMessage(String message)
{
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;

  String url = "https://api.telegram.org/bot" + botToken +
               "/sendMessage?chat_id=" + chatID1 +
               "&text=" + message;

  Serial.println("Sending message to Person 1...");

  https.begin(client, url);
  int httpCode = https.GET();

  if (httpCode > 0)
  {
    Serial.println("Message sent successfully");
  }
  else
  {
    Serial.println("Error sending message");
  }

  https.end();

  /* -------- SECOND PERSON MESSAGE -------- */

  String url2 = "https://api.telegram.org/bot" + botToken +
                "/sendMessage?chat_id=" + chatID2 +
                "&text=" + message;

  Serial.println("Sending message to Person 2...");

  https.begin(client, url2);
  int httpCode2 = https.GET();

  if (httpCode2 > 0)
  {
    Serial.println("Message sent to second user");
  }
  else
  {
    Serial.println("Error sending to second user");
  }

  https.end();
}

/* ---------------- GET CURRENT TIME ---------------- */

String getTime()
{
  struct tm timeinfo;

  if(!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return "Time Error";
  }

  char timeString[20];
  strftime(timeString, sizeof(timeString), "%H:%M:%S", &timeinfo);

  return String(timeString);
}

/* -------- SEND SAFE MESSAGE -------- */

void sendSafeMessage()
{
  String currentTime = getTime();

  String message = "✅ No breach detected at time " + currentTime;

  Serial.println("Sending Safe Status Message...");
  Serial.println(message);

  sendTelegramMessage(message);
}

void setup()
{
  Serial.begin(115200);

  pinMode(buzzer, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(laserPin, OUTPUT);

  digitalWrite(laserPin, HIGH);

  Serial.println("Connecting to WiFi...");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi Connected");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  Serial.println("Time synchronized");

  lastCheckTime = millis();
}

void loop()
{
  int ldrValue = analogRead(ldrPin);

  Serial.print("LDR Value: ");
  Serial.println(ldrValue);

  /* -------- BREACH DETECTION -------- */

  if (ldrValue > threshold && alertSent == false)
  {
    Serial.println("Laser Beam Interrupted!");
    Serial.println("Intrusion Detected");

    digitalWrite(buzzer, HIGH);
    digitalWrite(ledPin, HIGH);

    String currentTime = getTime();

    String message = "⚠ A breach Is detected at time " + currentTime;

    Serial.println("Sending Telegram Alert...");
    Serial.println(message);

    sendTelegramMessage(message);

    alertSent = true;
    safeMessageSent = false;

    delay(5000);
  }

  /* -------- RESET WHEN LASER RETURNS -------- */

  if (ldrValue < threshold)
  {
    alertSent = false;
    digitalWrite(buzzer, LOW);
    digitalWrite(ledPin, LOW);
  }

  /* -------- SAFE STATUS CHECK -------- */

  if (ldrValue < threshold)
  {
    if (millis() - lastCheckTime >= 5000 && safeMessageSent == false)
    {
      sendSafeMessage();
      safeMessageSent = true;
      lastCheckTime = millis();
    }
  }
  else
  {
    lastCheckTime = millis();
    safeMessageSent = false;
  }

  delay(500);
}