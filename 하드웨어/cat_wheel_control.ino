//사용할 라이브러리
#include "Arduino.h"  //IDE ver.2.0.4
#include <EDB.h>         //EDB ver.1.0.4
#include <SD.h>          //SD ver.1.2.4
#include <Stepper.h>     //stepper ver.1.1.3
#include <SPI.h>
#include <ArduinoBLE.h> //Bluetooth ver.1.1.3(메모리 문제로 낮춤)


//연결된 센서 핀번호
//로터리 인코더 핀
#define R_ENCODER_PIN 13
#define R_ENCODER_PIN2 12
//wheel IR pin
#define W_IR_PIN 2
#define BW_LINE 2  //2cm 간격의 띠값
//Food IR
#define F_IR_PIN 3
//stepper_Motor pin
#define STEPPER_STEPS 2048
#define PIE 3.14

//DB data pin
#define SD_PIN 4         //우리 이더넷 쉴드는 SD는 4핀
#define ETHERNET_PIN 10 //이더넷은 10핀을 사용(SPI 방식을 이용하여 둘이 같이 킬 수 없다.)
#define TABLE_SIZE 8192  //DB 최대 사이즈

//stepper
Stepper eatmotor(STEPPER_STEPS, 9, 7, 8, 6);
#define STEPPER_SPEED 10
int target_reward=0;
int IsMotorSpin = 0;
int HowMuchSpinMotor =0;

//bool
bool foodIR_empty = false;     //먹이가 비었는지 확인용도
bool foodON = false;           //먹이를 주어야한다고 체크
bool startTime_check = false;  //런타임 값 지속 변경 방지용도(휠 회전을 시작하였습니다.)
bool on_IR = false;            //적외선 감지 시작

//milis()
long time = 0;            //먹이 딜레이
int time_delay = 500;    //time에서 0.5초마다 작동시키기 위한 중간수
long time2 = 0;           //IR 시간체크
long runTime = 0;         //달린 시간
int IR_value = 0;         //현재 적외선 센서값
int IR_reValue = 0;       //이전 적외선 센서값
int count_IR = 0;         //적외선 변동 횟수

//bluetooth
BLEService service("19B10000-E8F2-537E-4F6C-D104768A1214"); // Bluetooth® Low Energy LED Service
BLECharacteristic characteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite, 50);

//Encoder
int P;  // 퍼스 체크
float encoder_Redius;
int oldA = 0;
int oldB = 0;

//DB
char* db_name = "/db/edb_cat.db";            //데이터베이스 저장 위치
char* profile_name = "/db/profile.db";  //프로파일 저장 위치

File dbFile;
File dbFile_profile;

//런닝 데이터 형식
struct LogEvent {
  int id;               //식별 id
  int Run_Time;         // 달린 시간 (초단위)
  int Rotations_Count;  //몇 바퀴 돌았는지 (거리<distance(m)>*100/둘레<wheel_circumference>)
  float Distance;       //달린 거리
  float Kcal;           //칼로리
  int Reward;           //먹이 제공 횟수
} logEvent;

//기본 프로필 데이터 형식
struct Profile_LogEvent {
  int id;
  float Wheel_Circumference;  //휠 둘레
  int Cat_Age;                //고양이 나이
  float Cat_Weight;           //고양이 무게
  char Cat_gender;            //고양이 성별
  int Cat_rewardSet;          //목표 보상

} profile_LogEvent;

void writer (unsigned long address, byte data) {
    dbFile.seek(address);
    dbFile.write(data);
    dbFile.flush();
}

byte reader (unsigned long address) {
    dbFile.seek(address);
    byte b = dbFile.read();
    return b;
}
// Create an EDB object with the appropriate write and read handlers
EDB edb(&writer, &reader);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);  //start serial monitor
 while (!Serial) {
    ; // Serial 포트 연결 대기
  }


  pinMode(R_ENCODER_PIN, INPUT);
  pinMode(W_IR_PIN, INPUT);
  pinMode(F_IR_PIN, INPUT);
  eatmotor.setSpeed(STEPPER_SPEED);

  //EDB
  randomSeed(analogRead(0));

  if (!SD.begin(4)) {
    Serial.println("No SD-card.");
    while(1);
  }
  // Check dir for db files
  if (!SD.exists("/db")) {
    Serial.println("Dir for Db files does not exist, creating...");
    SD.mkdir("/db");
  }

  if (SD.exists(db_name)) {
    
    dbFile = SD.open(db_name, FILE_WRITE);

    // Sometimes it wont open at first attempt, espessialy after cold start
    // Let's try one more time
    if (!dbFile) {
      dbFile = SD.open(db_name, FILE_WRITE);
    }

    if (dbFile) {
      Serial.print("Openning current table... ");
      EDB_Status result = edb.open(0);
      if (result == EDB_OK) {
        Serial.println("DONE");
      } else {
        Serial.println("ERROR");
        Serial.println("Did not find database in the file " + String(db_name));
        Serial.print("Creating new table... ");
        edb.create(0, TABLE_SIZE, (unsigned int)sizeof(logEvent));
        Serial.println("DONE");
      }
    } else {
      Serial.println("Could not open file " + String(db_name));
    }
  } else {
    Serial.print("Creating table... ");
    // create table at with starting address 0
    dbFile = SD.open(db_name, FILE_WRITE);
    edb.create(0, TABLE_SIZE, (unsigned int)sizeof(logEvent));
    Serial.println("DONE");
  }
  //checked files
  if (SD.exists(profile_name)) {  //프로파일 체크(초기)
    dbFile_profile = SD.open(profile_name, FILE_WRITE);

    // Sometimes it wont open at first attempt, espessialy after cold start
    // Let's try one more time
    if (!dbFile_profile) {
      dbFile_profile = SD.open(profile_name, FILE_WRITE);
    }

    if (dbFile_profile) {
      Serial.print("Openning current table... ");
      EDB_Status result = edb.open(0);
      if (result == EDB_OK) {
        Serial.println("DONE");
      } else {
        Serial.println("ERROR");
        Serial.println("Did not find database in the file " + String(profile_name));
        Serial.print("Creating new table... ");
        edb.create(0, TABLE_SIZE, (unsigned int)sizeof(profile_LogEvent));
        Serial.println("DONE");
      }
    } else {
      Serial.println("Could not open file " + String(profile_name));
    }
  } else {
    Serial.print("Creating table... ");
    // create table at with starting address 0
    dbFile = SD.open(profile_name, FILE_WRITE);
    edb.create(0, TABLE_SIZE, (unsigned int)sizeof(profile_LogEvent));
    Serial.println("DONE");
  }
  while (!Serial)
  ;
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
  characteristic.setValue("Wellcom!!");
  // 블루투스 광고 시작
  BLE.advertise();

  Serial.println("블루투스 가시성을 활성화했습니다!");

  //IR 초기화
  IR_reValue = !digitalRead(W_IR_PIN);
  if(countRecords()==0){
  profile_LogEvent.id=1; //int
  profile_LogEvent.Wheel_Circumference=20*PIE;  //휠 둘레 float
  profile_LogEvent.Cat_Age = 12;                //고양이 나이 int
  profile_LogEvent.Cat_Weight=4;           //고양이 무게 float
  profile_LogEvent.Cat_gender='F';            //고양이 성별 char
  profile_LogEvent.Cat_rewardSet=500;       //int
  createRecords_Profile();
  dbFile_profile.close();
  }else{selectAll_profile();}
}
//loop start
void loop() {

  if (!dbFile){
dbFile = SD.open(db_name, FILE_WRITE);
if (dbFile) {
      Serial.print("Openning current table... ");
      EDB_Status result = edb.open(0);
  }}
  if (!dbFile_profile){
    dbFile = SD.open(profile_name, FILE_WRITE);
    if (dbFile_profile) {
      Serial.print("Openning current table... ");
      EDB_Status result = edb.open(0);
  }
  }

  // put your main code here, to run repeatedly:
  BLEDevice central = BLE.central();
  run_W_IR();
  //get cat-wheel sensor data

  //network
 if(central.connected()){
   if(characteristic.written()){
     String result =read_data();
     if(result == "Data"){
        
     }
     if(result =="Prof" ){
       selectAll_profile();
     }
     send_data("test data");
   }
 }

//먹이 활성화 조건 확인
if(count_IR!=0){
if((BW_LINE*count_IR)%profile_LogEvent.Cat_rewardSet == 0){
    if(IR_reValue!=IR_value){
    target_reward++;
    Serial.print("t reward: ");
    Serial.println(target_reward);
    foodON = true;
}}
if(target_reward <= 0){
  foodON = false;
}
    //먹이 제공
    if (foodON == true) {
      check_food();
      if (foodIR_empty == false) {
        //reward_food();
        EatMotorSpin();
      }
    }
  
}}

//start "Rotary function"
void R_Encoder_Sensor() {

  int change = checkEncoder();

  //sensor Input data 'P'
  if (change != 0) {

    if (change == 1) {
      P = P + change;
    } else {
      P = P + change * (-1);
    }
  }
}

int checkEncoder(void) {
  int result = 0;
  int newA = digitalRead(R_ENCODER_PIN);
  int newB = digitalRead(R_ENCODER_PIN2);

  if (newA != oldA || newB != oldB) {
    if (oldA == LOW && newA == HIGH) {
      result = -(oldB * 2 - 1);
    }
  }
  oldA = newA;
  oldB = newB;
  return result;
}
//end "Rotary function"

//start "wheel IR function"
void run_W_IR() {
  IR_value = !digitalRead(W_IR_PIN);

  if (IR_value != IR_reValue) {
    IR_reValue = IR_value;
    if (on_IR == false) on_IR = true;
    
      count_IR++;
      Serial.print("counif: ");
      Serial.println(count_IR);
      if (startTime_check == false) {
        runTime = millis();
        startTime_check = true;
      }
      if (time2 != 0) { time2 = 0; }
    

  } else {
    if (on_IR) {
      if (time2 == 0) time2 = millis();
      if (time2 + 5000 <= millis()) {
        logEvent.id = countRecords() + 1;

        logEvent.Run_Time = (millis() - runTime) / 1000;
        logEvent.Distance = distance_cal(1);
        logEvent.Rotations_Count = distance_cal(2) / profile_LogEvent.Wheel_Circumference;
        logEvent.Kcal = calculateExerciseCalories();
        logEvent.Reward = 1;
        Serial.print("Run_Time : ");
        Serial.println(logEvent.Run_Time);
        Serial.print("Kcal : ");
        Serial.println(logEvent.Kcal);
        Serial.print("Distance : ");
        Serial.println(logEvent.Distance);
        Serial.print("Rotations_Count : ");
        Serial.println(logEvent.Rotations_Count);
        Serial.print("reward : ");
        Serial.println(logEvent.Reward);
        createRecords();


        startTime_check = false;
        runTime = 0;
        count_IR = 0;
        on_IR = false;
        
        dbFile.close();
      }
    }
  }
  
}
//end "wheel IR function"

//start "Food IR function"
void check_food() {

  if (time + time_delay > millis()) {
    if (digitalRead(F_IR_PIN) == 0) {

      if (time + 3000 > millis()) {
        Serial.print("EMPTY");
        time_delay = 500;
        //휴대폰에 전송
        send_data("Reward Empty");
        foodIR_empty = true;
      } else {
        time_delay += 500;
      }
    } else {
      time_delay = 500;
      time = 0;
      foodIR_empty = false;
    }
  }
}
//end "Food IR function"

//거리계산
float distance_cal(int type) {
  if (type == 1)
    return count_IR * BW_LINE / 100;  //M 단위
  if (type == 2)
    return count_IR * BW_LINE;  //cm 단위
}

//start "calorie function"
float calculateExerciseCalories() {
  float exercise_met;

  if (runTime / 1000 <= 1) {
    exercise_met = 1.0;
  } else {
    exercise_met = 2.0 * distance_cal(1) / runTime / 1000 + 1.0;
  }
  // 활동계수 계산
  float speed = distance_cal(1) / runTime;
  float activity_factor;
  if (speed <= 2.22) {
    activity_factor = 1.2;
  } else if (speed <= 3.33) {
    activity_factor = 1.4;
  } else if (speed <= 4.44) {
    activity_factor = 1.6;
  } else {
    activity_factor = 1.8;
  }
  float calories = exercise_met * profile_LogEvent.Cat_Weight * runTime / 1000 / 60;
  return calories * activity_factor;
}

float BMR_cal() {
  float weight = profile_LogEvent.Cat_Weight;
  char gender = profile_LogEvent.Cat_gender;
  int age = profile_LogEvent.Cat_Age;

  float BMR;
  if (gender == 'm') {
    BMR = (0.68 * weight) - (0.034 * age) + 31;
  } else if (gender == 'f') {
    BMR = (0.608 * weight) - (0.027 * age) + 46;
  }
  return BMR;
}
//end "caloie function"

//start "step moter"
void EatMotorSpin(void) {  // 먹이 주는 모터 돌리는 함수


    if (IsMotorSpin == 1) {             // 만일 이미 모터가 돌아가는 중이라면
        // 여기부터 먹이를 줄 때 실행되는 코드
      eatmotor.step(STEPPER_STEPS / 1024);      // 2스텝씩 모터를 돌리는 코드
      HowMuchSpinMotor++;               // 지금까지 몇 스텝을 돌렸나 저장
      if (HowMuchSpinMotor == 256) {    // 모터 1/4 바퀴 다 돌렸을때 초기화
        IsMotorSpin = 0;
        HowMuchSpinMotor = 0;
        target_reward--;
      }
      if(IR_reValue!=IR_value){
        Serial.println("----E A T----");
      }
    }
  }
//end "step moter"
//start "EDB function"

void deleteOneRecord(int recno)  //레코더 recno를 삭제(1개)
{
  Serial.print("Deleting recno: ");
  Serial.println(recno);
  edb.deleteRec(recno);
}



int countRecords()  // DB에 몇개의 레코드가 있는지 확인
{
  Serial.print("Record Count: ");
  Serial.println(edb.count());
  return edb.count();
}

void createRecords()  //레코드 생성
{
  Serial.print("Creating Records... ");

  EDB_Status result = edb.appendRec(EDB_REC logEvent);

  Serial.println("DONE");
}
void createRecords_Profile()  //레코드 생성 프로파일 레코드 append예정
{
  Serial.print("Creating Records... ");

  EDB_Status result = edb.appendRec(EDB_REC profile_LogEvent);
  //profile_LogEvent.
  Serial.println("DONE");
}
void sendAlldata() {
  for (int recno = 1; recno <= edb.count(); recno++)  
  {
    EDB_Status result = edb.readRec(recno, EDB_REC logEvent);
    if(result == EDB_OK){
    send_data(String(logEvent.id));
    send_data(String(logEvent.Run_Time));
    send_data(String(logEvent.Rotations_Count));
    send_data(String(logEvent.Distance));
    send_data(String(logEvent.Kcal));
    send_data(String(logEvent.Reward));
    }else{
      Serial.println("EDB data send fail");
    }
    //블루투스로 데이터 전송
  }
}
void selectAll_profile()  //프로파일 로딩
{
  EDB_Status result = edb.readRec(countRecords()-1, EDB_REC profile_LogEvent);

}
void updateOneRecord(int recno) {

  EDB_Status result = edb.updateRec(recno, EDB_REC logEvent);

  Serial.println("DONE");
}
//end "EDB Function"

//start "BLE Function"
void send_data(String dataToSend){
        characteristic.writeValue((const uint8_t*)dataToSend.c_str(), dataToSend.length());
        Serial.print("Sent data: ");
        Serial.println(dataToSend);
}
String read_data(){
  uint8_t receivedData[20]; // 수신할 데이터를 저장할 버퍼
  int length = characteristic.readValue(receivedData, sizeof(receivedData)); // 데이터 수신
  String receivedString((char*)receivedData, length); // 수신된 바이트 배열을 문자열로 변환
  Serial.print("Received data: ");
  Serial.println(receivedString);
  return receivedString;
}
//end "BLE Function"

