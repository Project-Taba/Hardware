#include <BLEDevice.h> // BLE 통신을 위한 기본 헤더 파일 포함
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define FORCE_SENSOR_PIN 36 // 압력 센서를 연결할 핀 번호 정의

BLEServer *pServer = NULL; // BLE 서버 객체 초기화
BLECharacteristic *pTxCharacteristic = NULL; // 데이터 전송용 캐릭터리스틱 객체 초기화
bool deviceConnected = false; // 장치 연결 상태 표시
bool oldDeviceConnected = false; // 이전 연결 상태 표시

// 고유 식별자(UUID, 128bit) 정의
#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b" // 서비스의 UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // 데이터 수신을 위한 캐릭터리스틱 UUID
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // 데이터 전송을 위한 캐릭터리스틱 UUID

// BLE 서버 연결 및 해제 이벤트를 처리하기 위한 콜백 클래스 정의
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true; // 장치가 연결되면 연결 상태를 true로 설정
  };
  
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false; // 장치 연결이 해제되면 연결 상태를 false로 설정
  }
};

// 데이터 수신 시 호출되는 콜백 함수를 정의하는 클래스
class MyCallbacks: public BLECharacteristicCallbacks {  
  void onWrite(BLECharacteristic *pCharacteristic) {
    // 클라이언트로부터 데이터를 수신 받았을 때 실행
    std::string rxValue = pCharacteristic->getValue(); // 수신된 데이터를 가져옴
    if (rxValue.length() > 0) {
      // 수신된 데이터가 있을 경우 시리얼 모니터에 출력
      Serial.println("*********");
      Serial.print("수신된 값: ");
      for (int i = 0; i < rxValue.length(); i++) {
        Serial.print(rxValue[i]);
      }
      Serial.println();
      Serial.println("*********");
    }
  }
};

void setup() {
  Serial.begin(115200); // 시리얼 통신 시작
  pinMode(FORCE_SENSOR_PIN, INPUT); // 압력 센서 핀을 입력 모드로 설정

  BLEDevice::init("ESP32 Force Sensor"); // BLE 디바이스 초기화 및 이름 설정

  pServer = BLEDevice::createServer(); // BLE 서버 객체 생성
  pServer->setCallbacks(new MyServerCallbacks()); // 서버 연결/해제 이벤트 처리를 위한 콜백 설정

  BLEService *pService = pServer->createService(SERVICE_UUID); // BLE 서비스 생성

  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      ); // 데이터 전송을 위한 캐릭터리스틱 생성 및 속성 설정
  pTxCharacteristic->addDescriptor(new BLE2902()); // 캐릭터리스틱에 알림(Notify) 속성 추가

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_RX,
                        BLECharacteristic::PROPERTY_WRITE
                      ); // 데이터 수신을 위한 캐릭터리스틱 생성 및 속성 설정
  pRxCharacteristic->setCallbacks(new MyCallbacks()); // 데이터 수신 시 실행될 콜백 함수 설정

  pService->start(); // 서비스 시작
  pServer->getAdvertising()->start(); // BLE 광고 시작
  Serial.println("클라이언트 연결을 기다리는 중..."); // 클라이언트 연결 대기 메시지 출력
}

void loop() {
  if (deviceConnected) {
    if (!oldDeviceConnected) {
      // 장치가 새로 연결됐을 때 처리
      oldDeviceConnected = deviceConnected;
    }
    // 압력 센서 값 읽기 및 BLE 캐릭터리스틱 업데이트
    int analogReading = analogRead(FORCE_SENSOR_PIN);
    String forceValue = String(analogReading); // 압력 값

    String forceValue2 = "test"; // 추가 데이터 (예시)

    // 압력 값과 추가 데이터를 하나의 문자열로 결합
    String combinedValue = forceValue + "," + forceValue2;
  
    pTxCharacteristic->setValue((uint8_t*)combinedValue.c_str(), combinedValue.length());
    pTxCharacteristic->notify(); // 변경된 값을 클라이언트에 알림
    delay(10); // 너무 빈번한 업데이트 방지를 위한 지연
  } else if (oldDeviceConnected) {
    // 장치 연결이 해제됐을 때 처리, 광고 재시작
    pServer->startAdvertising();
    Serial.println("광고 시작");
    oldDeviceConnected = deviceConnected;
    delay(500); // 연결 해제 후 짧은 지연
  }
  delay(1000); // 연결 해제 후 짧은 지연
}