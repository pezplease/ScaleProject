#include <Arduino.h>
#include <TFT_eSPI.h>
#include "soc/rtc.h"
#include "HX711.h"
#include "Pushbutton.h"
#include <Wire.h>
#include "NimBLEDevice.h"


TFT_eSPI tft = TFT_eSPI();
HX711 scale;
int reading;
int lastReading;
#define CALIBRATION_FACTOR -755.495
const int LOADCELL_DOUT_PIN = 37;
const int LOADCELL_SCK_PIN = 25;
//-755.494
//const int calibration_factor = -34375/45.5;


NimBLEServer* pServer = nullptr;
NimBLECharacteristic *pCharacteristic = nullptr;
bool deviceConnected = false;
#define SERVICE_UUID        "a67f5009-f645-404a-b4a3-24dd1ae7ea88"
#define CHARACTERISTIC_UUID "012b6003-3363-401b-8bbf-6797b59edb0a"

int buttonState = 0;
#define BUTTON_PIN_R 0
Pushbutton buttonR(BUTTON_PIN_R);


void displayWeight (int weight){
  tft.fillScreen(TFT_BLACK);
  //char buffer[8];
  //snprintf(buffer,5, "%llu", weight);
  //tft.drawString("Weight is:",5,5,3);
  String weightString = String(weight);
  tft.drawString(weightString,65,140,4);
  //"%.1f"

}

class ServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
        Serial.print("Client address: ");
        Serial.println(connInfo.getAddress().toString().c_str());
        /** We can use the connection handle here to ask for different connection parameters.
         *  Args: connection handle, min connection interval, max connection interval
         *  latency, supervision timeout.
         *  Units; Min/Max Intervals: 1.25 millisecond increments.
         *  Latency: number of intervals allowed to skip.
         *  Timeout: 10 millisecond increments, try for 5x interval time for best results.
         */
        pServer->updateConnParams(connInfo.getConnHandle(), 24, 48, 0, 60);
    };
    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
        Serial.println("Client disconnected - start advertising");
        NimBLEDevice::startAdvertising();
    };
    void onMTUChange(uint16_t MTU, NimBLEConnInfo& connInfo) {
        Serial.printf("MTU updated: %u for connection ID: %u\n", MTU, connInfo.getConnHandle());
    };
    /*
// m********************* Security handled here **********************
****** Note: these are the same return values as defaults *******
    uint32_t onPassKeyDisplay() {
        Serial.println("Server Passkey Display");
        // This should return a random 6 digit number for security
         // or make your own static passkey as done here.
         
        return 123456;
    };

    void onConfirmPIN(const NimBLEConnInfo& connInfo, uint32_t pass_key) {
        Serial.print("The passkey YES/NO number: ");Serial.println(pass_key);
        // Inject false if passkeys don't match. 
        NimBLEDevice::injectConfirmPIN(connInfo, true);
    };

    void onAuthenticationComplete(const NimBLEConnInfo& connInfo) {
        //Check that encryption was successful, if not we disconnect the client 
        if(!connInfo.isEncrypted()) {
            NimBLEDevice::getServer()->disconnect(connInfo.getConnHandle());
            Serial.println("Encrypt connection failed - disconnecting client");
            return;
        }
        Serial.println("Starting BLE work!");
    };*/
};

class MyServerCallbacks: public NimBLEServerCallbacks{
  void onConnect(NimBLEServer* pServer){
    deviceConnected = true;
    Serial.println("Client Connected");
  }
  void onDisconnect(NimBLEServer* pServer){
    deviceConnected = false;
    Serial.println("Client Disconnected");
  }
};



void setup() {

  Serial.begin(115200);

  rtc_cpu_freq_config_t config;

  rtc_clk_cpu_freq_get_config(&config);
  rtc_clk_cpu_freq_to_config(RTC_CPU_FREQ_80M, &config);
  rtc_clk_cpu_freq_set_config_fast(&config);

  pinMode(BUTTON_PIN_R, INPUT);

  tft.init();
  tft.setRotation(4);
  tft.fillScreen(TFT_BLACK);
  //tft.drawString("Tat's Display!", 5, 5,2);

  Serial.println("Initializing the scale");
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(CALIBRATION_FACTOR);
  scale.tare();




  //create bluetooth server and service for app to connect to
  NimBLEDevice::init("Scale");

  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  NimBLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      NIMBLE_PROPERTY::READ|
      NIMBLE_PROPERTY::NOTIFY);

  pService->start();
  //pCharacteristic->setValue(reading);

  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

}

void loop() {
  
  
  if (buttonR.getSingleDebouncedPress()){
    Serial.println("Tare");
    scale.tare();
  }
  if (scale.wait_ready_timeout(200)){
    reading = round(scale.get_units());
    if (reading != lastReading){
      Serial.println(reading);
      displayWeight(reading);

      if (deviceConnected){
        pCharacteristic->setValue(String(reading).c_str());
        pCharacteristic->notify();
      }

    }
    Serial.println(reading);
     lastReading = reading;
  }
  else{
    Serial.println("HX711 not found");

  }
};










/*
  tft.fillScreen(TFT_BLACK);
  //tft.drawString("scale.get_units(5)", 5, 50,4);
  unsigned long long number = scale.get_units(5);
  char buffer[9];
  sprintf(buffer, "%llu", number);
  //tft.drawString("Weight is:",5,5,3);
  tft.drawCentreString(buffer,70,140,4);
  scale.power_down();             // put the ADC in sleep mode
  delay(50);
  scale.power_up();
  */
