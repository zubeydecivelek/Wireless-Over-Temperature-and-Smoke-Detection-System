#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>


// WiFi ayarları
const char* ssid = "Bakkalzade";
const char* password = "123456789";

// Telegram Bot Token
#define BOTtoken "7042774380:AAFfik-5hF_sWeN665j4YfsurvJSmjr53oA"

// WiFi ve Telegram nesneleri
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Pin tanımlamaları
const int mq4Pin = 32;
const int lm35Pin = 35;
const int ledPin = 25;
const int buzzerPin = 33;
const int buttonPin = 26;     // Butonun bağlı olduğu GPIO pin numarası
const int buttonPinDec = 27;     // Butonun bağlı olduğu GPIO pin numarası


// Zaman kontrolü
unsigned long lastTime = 0;
unsigned long timerDelay = 5000; // 5 saniye

// Alarm durum değişkenleri
bool alarmState = false;
bool stopped = false;
unsigned long alarmTriggeredTime = 0;
unsigned long alarmMessageInterval = 10000; // 5 saniye
float temperature = 24;
int smokeValue = 0;

// Chat Id
String user_chat_id = "980015360";

void setup() {
  Serial.begin(115200);

  // Pin modları
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(mq4Pin, INPUT);
  pinMode(lm35Pin, INPUT);
  // Buton pinini giriş olarak ayarla ve dahili pull-up direncini etkinleştir
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(buttonPinDec, INPUT_PULLUP);

  
  // WiFi bağlantısı
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print('.');
  }
  Serial.println(WiFi.localIP());
  digitalWrite(ledPin, HIGH);
  // digitalWrite(buzzerPin, HIGH);
  Serial.println("buzzer çalıştı");
  
client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Telegram SSL sertifikası

}

void loop() {
  // Duman ve sıcaklık sensörlerinden okuma
  smokeValue = analogRead(mq4Pin);
  smokeValue = smokeValue - 1300 <= 0 ? 0 : smokeValue - 1300;
  int tempValue = analogRead(lm35Pin);
  float voltage = tempValue * (3.3 / 4095.0);
  float sicaklik = voltage * 100.0;

  
  // Butonun durumunu oku
  int buttonState = digitalRead(buttonPin);
  int buttonStateDec = digitalRead(buttonPinDec);

  // Eğer butona basılmışsa
  if ((buttonState == LOW) && (temperature<100)) {
    temperature += 0.001856; 
  }
  if ((buttonStateDec == LOW) && (temperature>0)){
    temperature -= 0.001798; 
  }

  Serial.println(smokeValue);

  // Alarm kontrolü
  if ((smokeValue > 200 || temperature > 50) && !stopped) {
    if (!alarmState) {
      alarmState = true;
      alarmTriggeredTime = millis();
      sendTelegramAlert();
    } else if (millis() - alarmTriggeredTime >= alarmMessageInterval) {
      sendTelegramAlert();
      alarmTriggeredTime = millis();
    }
    digitalWrite(ledPin, HIGH);
    digitalWrite(buzzerPin, HIGH);
  } else {
    alarmState = false;
    digitalWrite(ledPin, LOW);
    digitalWrite(buzzerPin, LOW);
  }

  // Telegram mesaj kontrolü
  if (millis() > lastTime + timerDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTime = millis();
  }

}

void sendTelegramAlert() {
  String message = "ALARM! Smoke or fire is detected!";
  bot.sendMessage(user_chat_id, message, "");
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    user_chat_id = chat_id;
    String text = bot.messages[i].text;

    if (text == "/status") {
      String message = "Current Status:\n";
      message += "Smoke Value: " + String(smokeValue) + "\n";
      message += "Temperature: " + String(temperature)  + "°C";
      bot.sendMessage(chat_id, message, "");
    } else if (text == "/stop") {
      alarmState = false;
      stopped = true;
      digitalWrite(ledPin, LOW);
      digitalWrite(buzzerPin, LOW);
      bot.sendMessage(chat_id, "Alarm stopped.", "");
    }else if (text == "/reactivate"){
      stopped = false;
      bot.sendMessage(chat_id, "Reactivated.", "");
    }
    else if (text == "/start"){
      bot.sendMessage(chat_id, "Welcome to our next generation House Protector.\nPossible commands:\n-/status\n-/stop\n-/reactivate", "");
    }
    else if (text == ""){
    } 
    else {
      bot.sendMessage(chat_id, "Unknown command: "+ text +". Possible commands:\n-/status\n-/stop\n-/reactivate", "");
    }
  }
}


