#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Ganti true jika berhotspot tanpa password
boolean modeHotspot = false;
const char* hotspot = "HOTSPOT-SKANEBA";

// HOTSPOT ITC
const char* ssid = "HOTSPOT-SKANEBA";
const char* password = "itbisa123";

// WiFi credentials
// ASSEMBLY TE - CNC
// const char* ssid = "ASSEMBLY ONLY";
// const char* password = "onlyassemblytebos";

// INSTRUKTUR TE - 2.4G
// const char* ssid = "INTRUKTUR-TAV-2.4G";
// const char* password = "skanebabisa1";

//HOSTSPOT-SKANEBA-TU
// const char* ssid = "HOTSPOT-SKANEBA-TU";
// const char* password = "skanebabisa";

// MQTT Broker Configuration
const char* mqtt_server = "172.16.80.123";  // Ganti dengan alamat IP broker MQTT Anda
// const char* mqtt_server = "10.16.0.102";  // Ganti dengan alamat IP broker MQTT Anda
const int mqtt_port = 1883;               // Port MQTT default
const char* mqtt_user = "ben";            // Username MQTT Anda
const char* mqtt_password = "1234";       // Password MQTT Anda

#define LED_PIN D0  // D0
#define BUZ_PIN D1  // D1

// RFID
#define SDA_PIN 2  // D4
#define RST_PIN 0  // D3

char IDTAG[20];
char chipID[25];                       // Store ESP8266 Chip ID
char nodevice[20] = "2309MAS001";      // Change to your desired Node Device (max 20 characters)
char key[50] = "1234567890987654321";  // Change to your desired Key (max 20 characters)

// Membuat array untuk memetakan pesan ke kode bunyi buzzer
const char* buzzerCodes[] = {
  "404", "_...",
  "405", "_....",
  "406", "_.....",
  "407", "_.....",
  "500", "_.....",
  "501", "_._...",
  "502", "_._.....",
  "505", "_.....",
  "510", "_.....",
  "515", "_.....",
  "545", "_.....",
  "555", "_.....",
  "IDTT", "._..",
  "HLTM", "._.",
  "TBPS", "._._..",
  "SAPP", "...",
  "PLAW", ".._",
  "PPBH", "..",
  "PPPP", "...",
  "SMPM", "...",
  "MMMM", "...",
  "BMPM", "..",
  "PKBD", "_.",
  "TASK", "_._._.",
  "BMPE", ".."
};

String receivedMessage = "";

MFRC522 mfrc522(SDA_PIN, RST_PIN);

WiFiClient espClient;
PubSubClient client(espClient);

// Variabel untuk memantau waktu terakhir pembacaan kartu RFID
unsigned long lastRFIDReadTime = 0;
const unsigned long RFID_READ_INTERVAL = 600000;  // 10 menit dalam milidetik (10 * 60 * 1000)

// boolean indexRestart = false;

unsigned long previousLEDmqtt = 0;
const long intervalLEDmqtt = 100;
const long intervalLEDmqtt2 = 300;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZ_PIN, OUTPUT);

  if (modeHotspot == true) {
    const char* ssid = hotspot;
  }

  Serial.begin(115200);
  if (modeHotspot == true) {
    WiFi.begin(ssid);
  } else {
    WiFi.begin(ssid, password);
  }

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
    Serial.println("Menyambungkan ke WiFi...");
  }

  digitalWrite(LED_PIN, HIGH);
  Serial.println("Tersambung ke WiFi");

  // Setup MQTT client
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  while (!client.connected()) {
    if (client.connect("NodeMCUClient", mqtt_user, mqtt_password)) {
      Serial.println("Tersambung ke MQTT Broker");
    } else {
      Serial.println("Koneksi gagal. Mengulangi koneksi...");
      for (int q = 0; q < 5; q++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
      }

      delay(1000);
    }
  }

  Serial.println();
  Serial.println("Tersambung ke Wi-Fi");
  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC Address: ");
  Serial.println(WiFi.macAddress());

  // Get ESP8266 Chip ID
  int num = ESP.getChipId();
  itoa(num, chipID, 10);
  Serial.println("Chip ID: ");
  Serial.println(chipID);

  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("Tempelkan kartu RFID");
  buzz(3);

  // Subscribe to a topic

  // String topic = "responServer_";
  // topic += nodevice;
  // client.subscribe(topic.c_str(), 0);
  client.subscribe("responServer", 0);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Menerima pesan pada topic: ");
  Serial.println(topic);
  Serial.print("Pesan: ");
  for (int i = 0; i < length; i++) {
    // Serial.print((char)payload[i]);
    receivedMessage += (char)payload[i];
  }
  Serial.println(receivedMessage);
  Serial.println();

  // prosen respon data
  identifyAndProcessJsonResponse(receivedMessage, nodevice);
}

void loop() {
  client.loop();

  // Ambil waktu sekarang
  unsigned long currentMillisLEDmqtt = millis();

  // Periksa apakah sudah waktunya untuk mematikan atau menyalakan LED
  if (!client.connected()) {
    if (currentMillisLEDmqtt - previousLEDmqtt >= intervalLEDmqtt2) {
      // Simpan waktu terakhir kali LED dinyalakan atau dimatikan
      previousLEDmqtt = currentMillisLEDmqtt;

      // Balikkan status LED
      if (digitalRead(LED_PIN) == LOW) {
        digitalWrite(LED_PIN, HIGH);
      } else {
        digitalWrite(LED_PIN, LOW);
      }
    }
  } else {
    if (currentMillisLEDmqtt - previousLEDmqtt >= intervalLEDmqtt) {
      // Simpan waktu terakhir kali LED dinyalakan atau dimatikan
      previousLEDmqtt = currentMillisLEDmqtt;

      // Balikkan status LED
      if (digitalRead(LED_PIN) == LOW) {
        digitalWrite(LED_PIN, HIGH);
      } else {
        digitalWrite(LED_PIN, LOW);
      }
    }
  }

  int berhasilBaca = bacaTag();

  if (berhasilBaca) {
    // Kemudian, segera buat koneksi MQTT lagi
    reconnect();

    lastRFIDReadTime = millis();    // Perbarui waktu terakhir pembacaan kartu RFID
    static char hasilTAG[20] = "";  // Store previous tag ID

    if (IDTAG) {
      buzz(1);
    } else {
      buzz(0);
    }

    if (strcmp(hasilTAG, IDTAG) != 0) {
      strcpy(hasilTAG, IDTAG);
      Serial.println();
      Serial.println("=======================");
      Serial.println("ID Kartu: " + String(IDTAG));

      // Send data to server and receive JSON response
      String jsonResponse = sendCardIdToServer(IDTAG);

      Serial.println("Selesai kirim untuk ID: " + String(IDTAG));
      Serial.println("=======================");
      Serial.println("");
    } else {
      Serial.println("...");
    }

    delay(900);
    strcpy(hasilTAG, "");
  } else {
    strcpy(IDTAG, "");
  }

  buzz(0);
  receivedMessage = "";

  // Periksa apakah sudah waktunya untuk restart
  unsigned long currentTime = millis();
  if (currentTime - lastRFIDReadTime > RFID_READ_INTERVAL) {
    Serial.println("Tidak ada aktifitas pembacaan kartu RFID selama 10 menit. Melakukan restart...");
    // ESP.restart();  // Melakukan restart NodeMCU
  }
}

int bacaTag() {
  if (!mfrc522.PICC_IsNewCardPresent())
    return 0;

  if (!mfrc522.PICC_ReadCardSerial())
    return 0;

  memset(IDTAG, 0, sizeof(IDTAG));
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    snprintf(IDTAG, sizeof(IDTAG), "%s%02X", IDTAG, mfrc522.uid.uidByte[i]);
  }

  return 1;
}

String sendCardIdToServer(String cardId) {
  String jsonResponse = "";
  // Send RFID card data, Chip ID, Node Device, and Key to the server
  String request = "{";
  request += "\"nokartu\":\"" + String(cardId) + "\",";
  request += "\"idchip\":\"" + String(chipID) + "\",";
  request += "\"nodevice\":\"" + String(nodevice) + "\",";
  request += "\"key\":\"" + String(key) + "\",";
  request += "\"ipa\":\"" + WiFi.localIP().toString() + "\"";
  request += "}";

  if (client.connect("NodeMCUClient", mqtt_user, mqtt_password)) {
    // String mqttTopic = "dariMCU_" + String(nodevice);
    String mqttTopic = "dariMCU";
    Serial.println("Tersambung ke MQTT Broker");
    Serial.println("Kirim ke topik: " + mqttTopic + ": " + request);
    client.publish(mqttTopic.c_str(), request.c_str(), 0);
    // client.disconnect();
  } else {
    Serial.println("Koneksi ke MQTT Broker gagal");
    for (int q = 0; q <= 3; q++) {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
    }

    // index restart MCU
    // indexRestart = true;
  }

  return jsonResponse;
}

void reconnect() {
  // Loop sampai terhubung ke broker MQTT
  while (!client.connected()) {
    Serial.println("Menyambungkan ke MQTT Broker...");
    // Coba terhubung ke broker MQTT
    if (client.connect("NodeMCUClient", mqtt_user, mqtt_password)) {
      Serial.println("Tersambung ke MQTT Broker");

      // Langganan topik yang Anda butuhkan di sini jika diperlukan
      // String topic = "responServer_";
      // topic += nodevice;
      // client.subscribe(topic.c_str(), 0);
      client.subscribe("responServer", 0);
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      Serial.println(" mencoba konek lagi dalam 5 detik");
      // Tunggu sebelum mencoba lagi
      for (int q = 0; q <= 5; q++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
      }

      delay(4000);
    }
  }
}

void buzz(int loop) {
  if (loop == 0) {
    digitalWrite(BUZ_PIN, LOW);
  } else if (loop == 1) {
    digitalWrite(BUZ_PIN, HIGH);
    delay(100);
    digitalWrite(BUZ_PIN, LOW);
  } else {
    for (int t = 0; t < loop; t++) {
      digitalWrite(BUZ_PIN, HIGH);
      delay(100);
      digitalWrite(BUZ_PIN, LOW);
      delay(100);
    }
  }
}

void buzz_er(String _kode) {
  int delayTime = 0;

  for (int i = 0; i < _kode.length(); i++) {
    char karakter = _kode.charAt(i);

    if (karakter == '_') {
      // Buzzer berbunyi selama 1 detik
      // tone(BUZ_PIN, 1000);  // Frekuensi bunyi buzzer (1 kHz)
      digitalWrite(BUZ_PIN, HIGH);
      delay(1000);  // Bunyi selama 1 detik
      // noTone(BUZ_PIN);  // Matikan buzzer
      digitalWrite(BUZ_PIN, LOW);
    } else if (karakter == '.') {
      // Buzzer berbunyi selama 100 mili detik
      // tone(BUZ_PIN, 1000);  // Frekuensi bunyi buzzer (1 kHz)
      digitalWrite(BUZ_PIN, HIGH);
      delay(100);  // Bunyi selama 100 mili detik
      // noTone(BUZ_PIN);      // Matikan buzzer
      digitalWrite(BUZ_PIN, LOW);
    } else if (karakter == ' ') {
      // Tunda 100 mili detik
      delay(100);
    }

    // Tunda sebelum karakter selanjutnya (jika ada)
    if (i < _kode.length() - 1) {
      delay(100);  // Tunda 100 mili detik sebelum karakter berikutnya
    }
  }
}

void buzzBasedOnMessage(const char* message) {
  for (int i = 0; i < sizeof(buzzerCodes) / sizeof(buzzerCodes[0]); i += 2) {
    if (strcmp(message, buzzerCodes[i]) == 0) {
      buzz_er(buzzerCodes[i + 1]);
      Serial.println(buzzerCodes[i + 1]);
      break;  // Keluar dari loop setelah menemukan kode yang cocok
    }
  }
}

void identifyAndProcessJsonResponse(String jsonResponse, char* _nodevice) {
  // Parse and process JSON response
  // Menghapus karakter newline dan carriage return
  jsonResponse.replace("\n", "");
  jsonResponse.replace("\r", "");
  // Menghapus karakter backslash yang mengganggu
  jsonResponse.replace("\\", "");

  // Menghapus karakter ganda ("") dari awal dan akhir JSON
  jsonResponse = jsonResponse.substring(1, jsonResponse.length() - 1);

  // Serial.print("jsonResponse: ");
  // Serial.println(jsonResponse);

  // StaticJsonDocument<1024> jsonDoc;
  DynamicJsonDocument jsonDoc(1024);

  DeserializationError error = deserializeJson(jsonDoc, jsonResponse);

  if (error) {
    Serial.print("gagal to parse JSON: ");
    Serial.println(error.c_str());
  } else {
    // Mengakses elemen-elemen JSON yang benar
    const char* json_id = jsonDoc["respon"][0]["id"];
    const char* json_nodevice = jsonDoc["respon"][0]["nodevice"];
    const char* json_message = jsonDoc["respon"][0]["message"];
    const char* json_info = jsonDoc["respon"][0]["info"];
    const char* json_nokartu = jsonDoc["respon"][0]["nokartu"];

    if (json_nodevice) {
      // Print the values
      // Serial.println("Element JSON:");
      Serial.print("- id: ");
      Serial.println(json_id);
      Serial.print("- nodevice asal: ");
      Serial.println(_nodevice);
      Serial.print("- nodevice json: ");
      Serial.println(json_nodevice);

      if (strcmp(_nodevice, json_nodevice) == 0) {
        Serial.print("- pesan: ");
        Serial.println(json_message);
        Serial.print("- info: ");
        Serial.println(json_info);
        Serial.print("- nokartu: ");
        Serial.println(json_nokartu);

        Serial.print("ID & Nomor Device Sesuai! ");
        Serial.println();

        // Setelah selesai memproses respon, putuskan koneksi MQTT
        client.disconnect();
        Serial.println("Koneksi MQTT diputus!");

        buzzBasedOnMessage(json_message);
      } else {
        Serial.println("ID & Nomor Device Tidak Sesuai...!");
        Serial.println("Tidak ada Respon...!");

        // Setelah selesai memproses respon, putuskan koneksi MQTT
        client.disconnect();
        Serial.println("Koneksi MQTT diputus!");

        buzzBasedOnMessage("501");
      }
    } else {
      // Elemen "nodevice" tidak ada dalam JSON
      Serial.println("Elemen \"nodevice\" tidak ada dalam JSON.");
      Serial.println("Tidak ada Respon...!");

      // Setelah selesai memproses respon, putuskan koneksi MQTT
      client.disconnect();
      Serial.println("Koneksi MQTT diputus!");

      buzzBasedOnMessage("502");
    }
  }
}