#include <Arduino.h>

#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>
#include <TFT_eSPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>

#include  "credentials.h"

#define SDA_PIN 21
#define SCL_PIN 22

#define BME280_ADDR  0x76
#define SEALEVELPRESSURE_HPA (1013.25)

TFT_eSPI tft = TFT_eSPI(); 
Adafruit_BME280 sensor;
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

unsigned long bot_lasttime;          // last time messages' scan has been done
bool Start = false;
const unsigned long BOT_MTBS = 1000; // mean time between scan messages
double temp, humidity, pressure;

void handleNewMessages(int numNewMessages)
{
  Serial.println("New Telegram messages received: ");
  Serial.println(String(numNewMessages));

  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    Serial.print("Message #");
    Serial.println(i);
    Serial.print("'");
    Serial.print(text);
    Serial.println("'");

    String from_name = bot.messages[i].from_name;
    if (from_name == "")
      from_name = "Guest";

    if(text == "/weather")
    {
      String response =     "Temperature: " + String(temp, 1) + "C\n";
      response = response + "Humidity:    " + String(humidity, 1) + "%\n";
      response = response + "Pressure:    " + String(pressure, 0) + "hPa\n\n";
      bot.sendMessage(chat_id, response);
    }

    if(text == "/hw")
    {
      bot.sendMessage(chat_id, "HW: ESP32 TTGO T-DISPLAY with OLED screen, BME280 sensor\n");
    }

    if(text == "/help")
    {
      bot.sendMessage(chat_id, "Usage:\n/hw - get hardware inventory\n/weather - report weather conditions\n/help - get this help");
    }
  }
}

void setup(void) {
  unsigned status;

  Serial.begin(115200);
  Serial.println("TTGO BME280 Telegram started");

  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.print("I2C bus started at ");
  Serial.print(SDA_PIN);
  Serial.print(", ");
  Serial.println(SCL_PIN);

  WiFi.mode(WIFI_STA);
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  Serial.println("Set Telegram root CA");
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  status = sensor.begin(BME280_ADDR, &Wire);
  if( !status )
  {
    Serial.println("Cannot find BME280 sensor. Aborted.");
    Serial.println(status);
    while (1) delay(10);
  }
  
  Serial.println("BME280 sensor initialized OK");

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLUE);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(0, 0, 4);
  delay(2000);
  tft.fillScreen(TFT_BLACK);
  Serial.println("Display initialized OK");

  Serial.print("Retrieving time: ");
  configTime(0, 0, "pool.ntp.org"); // get UTC time via NTP
  time_t now = time(nullptr);
  while (now < 24 * 3600)
  {
    Serial.print(".");
    delay(500);
    now = time(nullptr);
  }
  Serial.println(now);
  Serial.println("Time is retrieved OK");
}

void loop(void) {
  
  temp = sensor.readTemperature();
  humidity = sensor.readHumidity();
  pressure = sensor.readPressure();

  Serial.print( temp );
  Serial.print("C ");
  Serial.print( humidity );
  Serial.print("% ");
  Serial.print( pressure );
  Serial.println("hPa");
  
  tft.setCursor(0, 0);
  tft.print(temp);
  tft.print("C");
  tft.setCursor(0, 22);
  tft.print(humidity);
  tft.print("%");
  tft.setCursor(0, 44);
  tft.print(pressure);
  tft.print("hPa");
  tft.setCursor(0, 66);
  tft.print(WiFi.localIP());

  if (millis() - bot_lasttime > BOT_MTBS)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages)
    {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    bot_lasttime = millis();
  }
  delay(1000);  
}
