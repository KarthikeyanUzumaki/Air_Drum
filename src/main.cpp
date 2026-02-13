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

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

float yaw = 0;
unsigned long lastTime = 0;
unsigned long lastHitTime = 0;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) { deviceConnected = true; }
    void onDisconnect(BLEServer* pServer) { 
        deviceConnected = false;
        pServer->getAdvertising()->start(); 
    }
};

void setup() {
    Serial.begin(115200);
    if (!mpu.begin()) { while (1) yield(); }
    Wire.setClock(400000); 

    BLEDevice::init("PocketDrum-DIY");
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_NOTIFY);
    pCharacteristic->addDescriptor(new BLE2902());
    pService->start();
    pServer->getAdvertising()->start();
}

void loop() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    unsigned long currentTime = millis();
    float dt = (currentTime - lastTime) / 1000.0;
    lastTime = currentTime;

   
    if (abs(g.gyro.z) > 0.15) { 
        yaw += g.gyro.z * dt * (180.0 / PI);
    } else {
        yaw *= 0.98; 
    }

    
    if (digitalRead(0) == LOW) { yaw = 0; delay(300); }

    float totalAccel = sqrt(sq(a.acceleration.x) + sq(a.acceleration.y) + sq(a.acceleration.z));

    if (totalAccel > 28.0 && (millis() - lastHitTime) > 150) {
        String hitType = "SN";
        if (yaw < -35) hitType = "HH";     
        else if (yaw > 0) hitType = "KK";  
      
        if (deviceConnected) {
            pCharacteristic->setValue(hitType.c_str());
            pCharacteristic->notify();
        }
        Serial.print("Zone: "); Serial.print(hitType); Serial.print(" | Yaw: "); Serial.println(yaw);
        lastHitTime = millis();
    }
}