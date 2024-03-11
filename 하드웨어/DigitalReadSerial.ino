#include <Stepper.h>

const int STEPS = 2048;              //  모터 한바퀴 (360도) 스텝수 [변경불가]
Stepper eatmotor(STEPS, 11,9,10,8);  //  모터 핀


int IRdetect = 3;              //  IR 거리측정센서 핀번호 : 3
int eatDetectpin = 2;          //  먹이통감지센서 핀번호 : 2
int eatNoDetectCount = 0;      //  먹이통 적외선 센서가 연속으로 빈 먹이통을 감지할 때 쓰이는 변수. 센서 정확도 향상에 쓰임.
int previouslySenserVel = 0;   //  IR 상태변화 확인
int tick = 0;                  //  IR 거리측정센서 틱횟수 (1틱에 2센티미터)
int eatamount = 1;             //  먹이 나오는 양 조절 (1은 모터 1바퀴 회전)
int eatCycle = 20;             //  몇 틱사이클마다 먹이를 줄건지 설정(1틱은 2센티미터)
int IsMotorSpin = 0;          //  현재 모터가 스핀중인가 아닌가를 저장하는 bool
int HowMuchSpinMotor= 0;       //  모터가 현재 얼마만큼 회전했는지를 저장하는 전역변수(한바퀴 = 1024)


void setup() {
  eatmotor.setSpeed(10);       //  모터 스피드 
  Serial.begin(9600);          //  컴퓨터 시리얼포트 

  pinMode(IRdetect, INPUT);    // IR 거리센서 핀 인풋모드
  pinMode(eatDetectpin, INPUT);   // 먹이통 측정센서 감지핀 입력모드

}

void loop() {
  int IRstate = !digitalRead(IRdetect);     //  내가 가지고 있는 적외선 센서가 탐지값이 0 이고 안탐지된게 1이라 역으로 저장함.
  //Serial.println(IRstate);                 //  적외선 센서 값 확인용 출력 

  if (previouslySenserVel != IRstate){      // 2센티미터 단위 바코드가 흰색에서 검은색, 검은색에서 흰색으로 바뀔 때 마다 1틱씩 추가 
    tick = tick + 1;
    previouslySenserVel = IRstate;
  }
  EatMotorSpin(); //---💕💕💕--- 사이클마다 모터를 돌리는 함수 

  if(digitalRead(eatDetectpin) == 0){       // 먹이통 빈거 감지하는 코드 (정확도 향상을 위해 연속으로 10회가 감지되어야지 먹이통이 비었다는 신호가 가도록 만듦)
    eatNoDetectCount =+ 1;                  // 먹이통이 빈 게 감지되면 빈거 감지되었다는 함수 1회업
    if (eatNoDetectCount > 10){             // 11회 연속으로 먹이통 빈 게 감지되면 실행
      Serial.print("EMPTY");
      //--------------먹이통이 비었습니다-------
      eatNoDetectCount = 0;                 // 연속감지 변수 초기화
    }
  }
  else if(digitalRead(eatDetectpin) == 1){
    eatNoDetectCount = 0;                   // 연속으로 먹이통의 빈 값이 들어오지 않으면 먹이통 빈거 센서 연속감지 값 초기화
  }

  //-------------------확인용 출력--------------
  
 
  Serial.print(tick*2);
  Serial.println("cm");
 
}

void EatMotorSpin(void){                      // 먹이 주는 모터 돌리는 함수

  if (tick != 0){                             // 초기값 건너뛰기
    if (IsMotorSpin == 1){                    // 만일 이미 모터가 돌아가는 중이라면
      Serial.println("----E A T----");        // 여기부터 먹이를 줄 때 실행되는 코드
      eatmotor.step(STEPS/1024);              // 2스텝씩 모터를 돌리는 코드
      HowMuchSpinMotor ++;                    // 지금까지 몇 스텝을 돌렸나 저장
      if(HowMuchSpinMotor == 256){            // 모터 1/4 바퀴 다 돌렸을때 초기화
        IsMotorSpin = 0;
        HowMuchSpinMotor = 0;
      }
    }
    if (tick % eatCycle == 0){                // 모터를 돌려야 할 때를 판단하고 저장
      IsMotorSpin = 1;
      tick = tick + 1;                        // 스텝모터 계속돌아가지 않게 틱 1회 추가하는 코드
    }
  }
}
