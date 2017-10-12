#include <math.h>
//Range values
const int FSRMAX = 800;
const int FSRMIN = 150;
const int FSRSTART = 5;
const int FLEXMAX = 30;

//FSR pressure sensitive resistors
int fsrPin1 = A0;
int fsrPin2 = A1;
//Flex sensor
int flexPin = A2;
//Ultrasonic distance sensor
int TrigPin = 2; 
int EchoPin = 3; 
int buzzerPin = 6;

//Digital shifters
int clockPin = 13;
int latchPin = 12;
int dataPin = 5; //data for force and time

int pointclockPin = 11;
int pointlatchPin =10;
int pointDataPin = 7;


int fsrVal1 = 0;
int fsrVal2 = 0;
int flexVal = 0;
float distance; 

unsigned long time1=0;
unsigned long time2=0;
unsigned long now = 0;
float timeAll;

int handShake = 0; //0: no hand, 1: hand appears;
int forceN = 0; //0-5 for the sequence
int timeN = 0;
int tooClose = 0;
int tooFast = 0;
int wrongForce = 0;
int wrongTime = 0;
float totalPoint = 5; 

int range[7] = {1,3,7,15,31,63,127};
int numList[6] = {119,65,59,107,77,110};


void setup() {
  Serial.begin(9600);
  pinMode (fsrPin1, INPUT);
  pinMode (fsrPin2, INPUT);
  pinMode (flexPin, INPUT);
  
  pinMode (TrigPin, OUTPUT); 
  pinMode (EchoPin, INPUT); 

  pinMode (dataPin, OUTPUT);
  pinMode (pointDataPin, OUTPUT);
  pinMode (latchPin, OUTPUT);
  pinMode (clockPin, OUTPUT);
  pinMode (pointlatchPin, OUTPUT);
  pinMode (pointclockPin, OUTPUT);

  pinMode (buzzerPin, OUTPUT);
}

void loop() {
  readFSR();
  handShake = 0; //default as no hand
  tooClose = 0; 
  tooFast = 0;
  wrongForce =0;
  wrongTime = 0;
  
  totalPoint = 5;
  timeN = 0; forceN = 0;
  if((fsrVal1>=FSRMIN and fsrVal2 == 0) or (fsrVal1 ==0 and fsrVal2 >=FSRMIN)){
    Serial.println("Connection problem, please check the wires.");
  }
  while(fsrVal1 >=FSRSTART and fsrVal2 >= FSRSTART){
    if(handShake == 0){
      //A new hand shake begins, we just entered this while loop for the first round
      Serial.println("Handshake starts!");
      time1 = micros();
      pointClear();
    }
    handShake = 1;
    ifForceOK();
    readFSR();
    ifDistanceOK();
    ifShakeFreqOK();
    now = micros();
    timeN = int((now-time1)/1000000)+1;
    //Output the force, time data to LEDs via shift registors
    ledDisplay();
    Serial.print("led ranges: ");
    Serial.print(forceN);
    Serial.print(", ");
    Serial.println(timeN);
    
  }
  
  
  //handshake ends
  ledClear();
  //if there was a handshake
  if(handShake == 1){
    
    time2 = micros();
    timeAll = int((time2-time1)/1000);  //millisecond
    Serial.print("Time duration: ");
    Serial.print(timeAll);
    Serial.println(" ms");
    
    if(tooClose==1){totalPoint -=1;} //if the person ever standed too close
    if(tooFast ==1){totalPoint -=1;}
    if(wrongForce ==1){totalPoint -= 0.4;}
    int rulesViolated = tooClose + tooFast + wrongForce + wrongTime;
    timePointCalc(timeAll); //Reduce the points according to time duration

    if(totalPoint<0) totalPoint = 0;
    if(rulesViolated > 2) totalPoint = 0;
    Serial.print("Total point: ");
    Serial.println(totalPoint);
    pointDisplay(round(totalPoint));
  }
  
  delay(1000);
}


//-------------------------------------
//--------       Functions       ------
//-------------------------------------


void readFSR(){
  fsrVal1 = analogRead(fsrPin1);
  fsrVal2 = analogRead(fsrPin2);
}

int ifDistanceOK(){
  //0: too close, 1: OK
  digitalWrite(TrigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(TrigPin, HIGH); 
  delayMicroseconds(10);
  digitalWrite(TrigPin, LOW); 
  distance = pulseIn(EchoPin, HIGH)/ 58.00;
  if (distance < 20){
    tooClose = 1;
    Serial.println("Warning: You're standing too close!");
    digitalWrite(buzzerPin, HIGH);
    delay(100);
    digitalWrite(buzzerPin, LOW);
    delay(100);
    digitalWrite(buzzerPin, HIGH);
    delay(100);
    digitalWrite(buzzerPin, LOW);
 
    return 0;
  }
  else{
    return 1;
  }
}

int ifForceOK(){
  //0: weak, 1: ok, 2: strong, 3: wtf is that
  //Serial.print("Force: ");
  //Serial.print(fsrVal1);
  //Serial.print(", ");
  //Serial.println(fsrVal2);
  if (fsrVal1 <= FSRMAX and fsrVal1 >= FSRMIN and fsrVal2 <= FSRMAX and fsrVal2 >= FSRMIN){
      //good force
      Serial.println("Ok, keep it!");
      if(fsrVal1 >=660 or fsrVal2 >=660){
        forceN=3;
      }
      else if(fsrVal1 >=500 or fsrVal2 >=500){
        forceN=2;
      }
      else{
        forceN=1;
      }
      return 1;
   }
    //If too strong
    else if (fsrVal1 >= FSRMAX or fsrVal2 >= FSRMAX){
      wrongForce = 1;
      Serial.println("Auuuch!");
      if(fsrVal1 >= 900 or fsrVal2 >=900){
        forceN = 6;
        totalPoint = totalPoint - 0.1;
      }
      else if(fsrVal1 >=860 or fsrVal2 >=860){
        forceN = 5;
        totalPoint = totalPoint - 0.001;
      }
      else{
        forceN = 4;
        totalPoint = totalPoint - 0.0005;
      }
      return 2;
    }
    //If too weak
    else if (fsrVal1 <= FSRMIN or fsrVal2 <= FSRMIN){
      wrongForce = 1;
      Serial.println("Harder plz");
      forceN = 0;
      totalPoint = totalPoint - 0.001;
      return 0;
    }
    else{
      wrongForce = 1;
      Serial.println("Is that a hand?");
      forceN=0;
      totalPoint = totalPoint - 0.01;
      return 3;
    }
}

void timePointCalc(float t){ //T: ms
  if(t<1500){
    wrongTime = 1;
    totalPoint -=2;
  }
  else if(t>3000 and t<=4000){
    totalPoint -=1;
  }
  else if(t>4000){
    wrongTime = 1;
    totalPoint -=2;
  }
}

void pointDisplay(int tpoint){
  digitalWrite(pointlatchPin, LOW);
  shiftOut(pointDataPin, pointclockPin, MSBFIRST, numList[tpoint]);
  digitalWrite(pointlatchPin, HIGH);
}

void ledDisplay(){
  //starts to transmitting the message to shift registers
  if(forceN > 6) forceN = 6;
  if(timeN >6) timeN = 6;
  digitalWrite(latchPin, LOW); 
  shiftOut(dataPin, clockPin, MSBFIRST, range[forceN]);
  shiftOut(dataPin, clockPin, MSBFIRST, range[timeN]);
  digitalWrite(latchPin, HIGH); //let shift registers end listening for information
  
}

void ledClear(){
  digitalWrite(latchPin, LOW); 
  shiftOut(dataPin, clockPin, MSBFIRST, 0);
  shiftOut(dataPin, clockPin, MSBFIRST, 0);
  digitalWrite(latchPin, HIGH);
}
void pointClear(){
  digitalWrite(pointlatchPin, LOW);
  shiftOut(pointDataPin, pointclockPin, MSBFIRST, 0);
  digitalWrite(pointlatchPin, HIGH);
}

void ifShakeFreqOK(){
  int change = 0;
  flexVal = analogRead(flexPin);
  delay(50);
  change = analogRead(flexPin) - flexVal;
  Serial.println(change);
  if(change > FLEXMAX or change < -FLEXMAX){
    tooFast = 1;
    totalPoint -= 0.5;
    digitalWrite(buzzerPin, HIGH);
    delay(1000);
    digitalWrite(buzzerPin, LOW);
  }
}


