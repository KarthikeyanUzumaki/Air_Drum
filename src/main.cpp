#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

Adafruit_MPU6050 mpu;
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

// Unique IDs for Bluetooth
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

float lastAccelZ = 0;
unsigned long lastHitTime = 0;
const int debounceDelay = 150; 
const float jerkThreshold = 15.0; // Adjust this for sensitivity

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) { deviceConnected = true; }
    void onDisconnect(BLEServer* pServer) { 
        deviceConnected = false;
        pServer->getAdvertising()->start(); // Restart advertising
    }
};

void setup() {
    Serial.begin(115200);
    
    // Initialize MPU6050
    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        while (1) yield();
    }
    Wire.setClock(400000); // Fast I2C

    // Setup Bluetooth
    BLEDevice::init("PocketDrum-DIY");
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID,
                        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
                      );
    pCharacteristic->addDescriptor(new BLE2902());
    pService->start();
    pServer->getAdvertising()->start();
    Serial.println("Waiting for a client connection...");
}

void loop() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float currentAccelZ = a.acceleration.z;
    float jerk = abs(currentAccelZ - lastAccelZ);
    lastAccelZ = currentAccelZ;

    if (jerk > jerkThreshold && (millis() - lastHitTime) > debounceDelay) {
        if (deviceConnected) {
            pCharacteristic->setValue("HIT");
            pCharacteristic->notify();
            Serial.println("Drum Hit Detected!");
        }
        lastHitTime = millis();
    }
    delay(10); // 100Hz sampling rate
}