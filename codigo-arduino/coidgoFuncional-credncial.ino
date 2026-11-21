
/*
  CREDENCIAL INTELIGENTE ITTLA
  Firmware: ESP32 + RC522 + pagina

  LECTOR RC522 - Esp32:
    SDA / SS -> GPIO 5
    SCK      -> GPIO 18
    MOSI     -> GPIO 23
    MISO     -> GPIO 19
    RST      -> GPIO 22
    GND      -> GND
    3.3V     -> 3V3

  BLE:
    Nombre: ITTLA-ESP32

  Servicio:
    7c3a0001-8e8f-4d5a-9b3c-1a2b3c4d5e6f

  Caracteristica:
    7c3a0002-8e8f-4d5a-9b3c-1a2b3c4d5e6f

  Dependencias:
    - MFRC522 by GithubCommunity
    - wifi incluida en ESP32 Arduino Core
*/
#include <SPI.h>
#include <MFRC522.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
// CONFIGURACION RC522
#define RC522_SS_PIN   5
#define RC522_RST_PIN 22

#define SPI_SCK_PIN   18
#define SPI_MISO_PIN  19
#define SPI_MOSI_PIN  23

MFRC522 rfid(RC522_SS_PIN, RC522_RST_PIN);

bool readerReady = false;


// CONFIGURACION 
// NO MODIFICAR: COINCIDE CON LA PWA

#define DEVICE_NAME "ITTLA-ESP32"

#define SERVICE_UUID \
  "7c3a0001-8e8f-4d5a-9b3c-1a2b3c4d5e6f"

#define CHARACTERISTIC_UUID \
  "7c3a0002-8e8f-4d5a-9b3c-1a2b3c4d5e6f"
BLEServer* bleServer = nullptr;
BLECharacteristic* uidCharacteristic = nullptr;
bool deviceConnected = false;
bool restartAdvertising = false;
// EVENTOS BLE

class ServerCallbacks : public BLEServerCallbacks {

  void onConnect(BLEServer* server) override {
    deviceConnected = true;
    restartAdvertising = false;

    Serial.println("[BLE] Android conectado.");
    Serial.println("[BLE] Esperando tarjeta...");
  }

  void onDisconnect(BLEServer* server) override {
    deviceConnected = false;
    restartAdvertising = true;

    Serial.println("[BLE] Android desconectado.");
  }
};

// ==========================================
// CONVERTIR UID A HEXADECIMAL
// ==========================================

String uidToHex(byte* buffer, byte bufferSize) {

  String result;
  result.reserve(bufferSize * 2);

  for (byte i = 0; i < bufferSize; i++) {

    if (buffer[i] < 0x10) {
      result += '0';
    }

    result += String(buffer[i], HEX);
  }

  result.toUpperCase();

  return result;
}

// ==========================================
// INICIAR LECTOR RC522
// ==========================================

void setupRC522() {

  Serial.println("[SPI] Iniciando bus SPI...");

  SPI.begin(
    SPI_SCK_PIN,
    SPI_MISO_PIN,
    SPI_MOSI_PIN,
    RC522_SS_PIN
  );

  Serial.println("[RC522] Inicializando lector...");

  rfid.PCD_Init();

  delay(100);

  byte version = rfid.PCD_ReadRegister(
    MFRC522::VersionReg
  );

  if (version == 0x00 || version == 0xFF) {

    readerReady = false;

    Serial.println("[ERROR] RC522 no detectado.");
    Serial.println("[ERROR] Revisa alimentacion y conexiones.");

    return;
  }

  readerReady = true;

  Serial.println("[OK] RC522 detectado.");

  Serial.print("[RC522] Version: 0x");
  Serial.println(version, HEX);

  Serial.println("[RC522] Lector preparado.");
}

// ==========================================
// INICIAR BLUETOOTH LOW ENERGY
// ==========================================

void setupBLE() {

  Serial.println("[BLE] Inicializando Bluetooth...");

  BLEDevice::init(DEVICE_NAME);

  bleServer = BLEDevice::createServer();

  bleServer->setCallbacks(new ServerCallbacks());

  BLEService* service = bleServer->createService(
    SERVICE_UUID
  );

  uidCharacteristic = service->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );

  uidCharacteristic->setValue("");

  uidCharacteristic->addDescriptor(
    new BLE2902()
  );

  service->start();

  BLEAdvertising* advertising =
    BLEDevice::getAdvertising();

  advertising->addServiceUUID(SERVICE_UUID);

  advertising->setScanResponse(true);

  BLEDevice::startAdvertising();

  Serial.println("[Esp32] Servidor iniciado.");

  Serial.print("[Esp32] Nombre: ");
  Serial.println(DEVICE_NAME);

  Serial.println("[Esp32] Publicidad activa.");
}

// ==========================================
// ENVIAR UID A LA PWA
// ==========================================

void sendUID(String uid) {

  Serial.println("--------------------------------");

  Serial.print("[RC522] UID detectado: ");
  Serial.println(uid);

  if (deviceConnected && uidCharacteristic != nullptr) {

    uidCharacteristic->setValue(uid.c_str());

    uidCharacteristic->notify();

    Serial.print("[Esp32] UID enviado a Android: ");
    Serial.println(uid);

  } else {

    Serial.println("[Esp32] Sin conexion Android.");
    Serial.println("[Esp32] UID no enviado.");
  }

  Serial.println("--------------------------------");
}

// ==========================================
// CONFIGURACION INICIAL
// ==========================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("======================================");
  Serial.println(" CREDENCIAL INTELIGENTE ");
  Serial.println(" ESP32 + RC522 + wifi");
  Serial.println("======================================");

  setupRC522();

  setupBLE();

  Serial.println();
  Serial.println("[SISTEMA] Inicializacion terminada.");

  if (readerReady) {
    Serial.println("[SISTEMA] Acerca una tarjeta.");
  } else {
    Serial.println("[SISTEMA] lector pendiente.");
  }
}

// ==========================================
// CICLO PRINCIPAL
// ==========================================

void loop() {

  // Recuperar publicidad despues de desconexion BLE

  if (restartAdvertising) {

    restartAdvertising = false;

    delay(300);

    BLEDevice::startAdvertising();

    Serial.println("[Esp32] Publicidad reiniciada.");
  }

  // No intentar leer si el modulo no fue detectado

  if (!readerReady) {
    delay(500);
    return;
  }

  // Detectar tarjeta nueva

  if (!rfid.PICC_IsNewCardPresent()) {
    delay(30);
    return;
  }

  // Leer UID

  if (!rfid.PICC_ReadCardSerial()) {
    delay(30);
    return;
  }

  String uid = uidToHex(
    rfid.uid.uidByte,
    rfid.uid.size
  );

  // Enviar UID a Android

  sendUID(uid);

  // Finalizar comunicacion con la tarjeta

  rfid.PICC_HaltA();

  rfid.PCD_StopCrypto1();

  delay(100);
}
