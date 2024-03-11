#include <ArduinoBLE.h>

BLEService service("19B10000-E8F2-537E-4F6C-D104768A1214"); // Bluetooth® Low Energy LED Service
BLECharacteristic characteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite, 50);
long time = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // BLE 라이브러리 초기화
  if (!BLE.begin()) {
    Serial.println("블루투스를 초기화할 수 없습니다!");
    while (1);
  }

  // 로컬 이름 설정
  BLE.setLocalName("CatWheel_arduino");
  BLE.setAdvertisedService(service);

  service.addCharacteristic(characteristic);
  BLE.addService(service);
  characteristic.setValue("Hello World");
  // 블루투스 광고 시작
  BLE.advertise();

  Serial.println("블루투스 가시성을 활성화했습니다!");
}
void loop() {
  BLEDevice central = BLE.central();
  if(time==0){
    time = millis();
  }
  if (central) {
    if(millis()>=time+5000){
    Serial.print("Connected to central: ");
    Serial.println(central.address());
    time =0;
    }
    if(central.connected()) {
      
      if (characteristic.written()) {
         uint8_t receivedData[20]; // 수신할 데이터를 저장할 버퍼
    int length = characteristic.readValue(receivedData, sizeof(receivedData)); // 데이터 수신

    String receivedString((char*)receivedData, length); // 수신된 바이트 배열을 문자열로 변환

        Serial.print("Received data: ");
        Serial.println(receivedString);
        if(receivedString=="All"){
   
      
        String dataToSend = "No.1 ID";
        characteristic.writeValue((const uint8_t*)dataToSend.c_str(), dataToSend.length());
        Serial.print("Sent data: ");
        Serial.println(dataToSend);
        }
      }
    }


  }else{
    if(millis()>=time+5000){
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
    time=0; }
}}