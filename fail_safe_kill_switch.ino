#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// NRF24L01 Setup 
#define CE_PIN   9
#define CSN_PIN 10
RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

//  Drone Control 
const int motorPins[4] = {3, 5, 6, 9}; // Change based on wiring
const unsigned long SIGNAL_TIMEOUT = 500;  // 500 ms no signal => fail
unsigned long lastSignalTime = 0;
bool failsafeTriggered = false;

//  Data Structure 
struct ControlData {
  int throttle;
  int yaw;
  int pitch;
  int roll;
  bool killSwitch;
};

ControlData receivedData;

// Setup 
void setup() {
  Serial.begin(9600);
  
  // Initialize NRF24L01
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);  // More reliable at distance
  radio.setChannel(108);
  radio.openReadingPipe(0, address);
  radio.startListening();
  
  // Initialize motors
  for (int i = 0; i < 4; i++) {
    pinMode(motorPins[i], OUTPUT);
    analogWrite(motorPins[i], 0); // Start off
  }

  Serial.println("Drone Receiver Ready.");
}

//  Loop 
void loop() {
  if (radio.available()) {
    if (radio.read(&receivedData, sizeof(ControlData))) {
      lastSignalTime = millis();  // Reset signal timer
      
      if (receivedData.killSwitch) {
        triggerFailsafe("Kill switch activated");
      } else if (!failsafeTriggered) {
        updateMotors(receivedData.throttle);
      }
    } else {
      triggerFailsafe("Corrupted data");
    }
  }

  // Check for signal timeout
  if (millis() - lastSignalTime > SIGNAL_TIMEOUT && !failsafeTriggered) {
    triggerFailsafe("Signal lost");
  }
}

//  Motor Control 
void updateMotors(int throttle) {
  throttle = constrain(throttle, 0, 255);
  for (int i = 0; i < 4; i++) {
    analogWrite(motorPins[i], throttle);
  }
}

//  Failsafe Logic 
void triggerFailsafe(String reason) {
  failsafeTriggered = true;
  for (int i = 0; i < 4; i++) {
    analogWrite(motorPins[i], 0); // Immediately stop motors
  }
  Serial.print("FAILSAFE TRIGGERED: ");
  Serial.println(reason);
}
