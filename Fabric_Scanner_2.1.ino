/*
Fabric Scanner 1x1 meters 
Board: Arduino Mega

9th December, 2021
*/

#include <Stepper.h>
#include <LiquidCrystal.h>
#include <EasyBuzzer.h>

//315MHZ RF Receiver
const int REC_D0 = 47, REC_D1 = 48, REC_D2 = 49, REC_D3 = 50, REC_VT = 19;

volatile byte State_Remote = 0;
int TestButtonCounter = 0; 

#define A 1
#define B 2
#define C 3
#define D 4

char BT_data = 0;
//String Standby;
int BT_Status;

int State_BT = 0; 

#define State_Idle 0
#define State_Scan 1
#define State_TestLED 2
#define State_Scan1 3
int State_Scanner = 0;

//LEDs
const int LED_1 = 22, LED_2 = 23, LED_3 = 24, LED_4 = 25, LED_5 = 26, LED_6 = 27, LED_7 = 28, LED_8 = 29, TOP_PANEL = 30, BACK_PANEL = 31;

// Stepper Motor 28BYJ-48
const int MOT_1 = 32, MOT_2 = 33, MOT_3 = 34, MOT_4 = 35;
const int StepsPerRev = 2048;
  Stepper StepMotor = Stepper(StepsPerRev, MOT_1, MOT_3, MOT_2, MOT_4);
  
const int LIMIT_SWITCH = 46;
long MOTOR_HOMING = -1;

const int Steps_rotationA = -1767;       //Roate Polarizer to 22.5 degrees
const int Steps_rotationB = -3534;       //Rotate Polarizer to 45 degrees
const int Steps_rotationC = -7068;       //Rotate Polarizer to 90 degrees
const int Steps_rotationD = 3534;       //Rotate Polarizer to -45 degrees
const int Steps_rotationE = -10602;       //Rotate Polarizer to 135 degrees
const int Steps_HomePositioning = -100;  //Position the motor Correctly

int State_Slave;

//Buzzer
const int BUZZER = 36;

//Camera Trigger and Focus
const int CAM_FOCUS = 38, CAM_TRIG = 37;

//16X2 LCD
const int LCD_D4 = 39, LCD_D5 = 40, LCD_D6 = 41, LCD_D7 = 42, LCD_EN = 43, LCD_RS = 45;
LiquidCrystal LCD(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

//Raspberry Pi trigger for AEB(Auto-exposure Bracketing) script
const int R_PI = 52;       //AEB(GPIO12 on Raspberry Pi)
const int R_PI1 = 53;      //USB replugging (GPIO24 on Raspberry Pi)

//Setting delays for Scan sequence
const int DELAY_LEDILLUMINATE = 1000;          //delay (milli second) between Panel On and Shutter On
const int DELAY_SHUTTER = 2000;                //delay (milli second) for Camera Shutter (2000, 5000, 10000, 2000)
const int DELAY_SHUTTER1 = 500;                //Additional deay for Fuji GFX100
const int DELAY_INPICS = 1000;                 //delay (milli second) between pictures (500,1000)

//Setting delays for LCD 
unsigned long CurrentMillis = 0;
unsigned long PreviousMillis = 0;
unsigned long CurrentMillis1 = 0;
unsigned long PreviousMillis1 = 0;
unsigned long CurrentMillis2;
unsigned long PreviousMillis2 = 0;
const unsigned long LCDInterval = 5000;    //To cycle instructions regarding remote buttons on LCD when in idle mode
const unsigned long LightsInterval = 60000;  //Lights would turn off after this interval in LED test mode
unsigned long lgUpdateTime;
unsigned long wait_millis = 2000;     //2 second wait for slave
unsigned long current_millis;

byte returnedValue;

//PWM pins
//const int PWM1 = 5, PWM2 = 6, PWM3 = 7, PWM4 = 8, PWM5 = 9, PWM6 = 10, PWM7 = 11, PWM8 = 12;

//Using PWM duty cycle 51,102,153,204(20%,40%,60%,80%) for 0',45',90' and 135'
int Slave_motState1[8] = {51, 51, 51, 51, 51, 51, 51, 51};  
int Slave_motState2[8] = {51, 204, 153, 102, 51, 204, 153, 102};       
int Slave_motState3[8] = {51, 204, 153, 102, 51, 204, 153, 102}; 
int Slave_motState4[8] = {204, 153, 102, 51, 204, 153, 102, 51}; 
int SL_MotState[8];

int PWMpin[8] = {3,5,6,7,8,9,10,11};

int LED[8] = {22,23,24,25,26,27,28,29}; 

int flag_noResponse = 0, flag_allRotated = 0;

void setup() {
  
  //Configure pin to trigger code on raspberyy pi
  pinMode (R_PI, INPUT_PULLUP);
  pinMode (R_PI1, INPUT_PULLUP);
  
  Serial.begin(9600);
  
  //Configuring Pins for RF Receiver as INPUT
  pinMode (REC_D0, INPUT); pinMode (REC_D1, INPUT); pinMode (REC_D2, INPUT);
  pinMode (REC_D3, INPUT); pinMode (REC_VT, INPUT);

  //Configuring Pins for the LED PANELS and COBS as Output
  pinMode (LED_1, OUTPUT); pinMode (LED_2, OUTPUT); pinMode (LED_3, OUTPUT); pinMode (LED_4, OUTPUT); pinMode (LED_5, OUTPUT);
  pinMode (LED_6, OUTPUT); pinMode (LED_7, OUTPUT); pinMode (LED_8, OUTPUT); pinMode (TOP_PANEL, OUTPUT); pinMode (BACK_PANEL, OUTPUT);

  //Set LED PANELS and COBS Off
  digitalWrite (LED_1, HIGH); digitalWrite (LED_2, HIGH); digitalWrite (LED_3, HIGH); digitalWrite (LED_4, HIGH); digitalWrite (LED_5, HIGH); 
  digitalWrite (LED_6, HIGH); digitalWrite (LED_7, HIGH); digitalWrite (LED_8, HIGH); digitalWrite (TOP_PANEL, HIGH); digitalWrite (BACK_PANEL, HIGH);

  //Configuring PWM pins as an Output
  pinMode (PWMpin[0], OUTPUT); pinMode (PWMpin[1], OUTPUT); pinMode (PWMpin[2], OUTPUT); pinMode (PWMpin[3], OUTPUT);
  pinMode (PWMpin[4], OUTPUT); pinMode (PWMpin[5], OUTPUT); pinMode (PWMpin[6], OUTPUT); pinMode (PWMpin[7], OUTPUT);

  analogWrite(PWMpin[0], 51); analogWrite(PWMpin[1], 51); analogWrite(PWMpin[2], 51); analogWrite(PWMpin[3], 51);
  analogWrite(PWMpin[4], 51); analogWrite(PWMpin[5], 51); analogWrite(PWMpin[6], 51); analogWrite(PWMpin[7], 51);

  pinMode (CAM_FOCUS, INPUT_PULLUP);
  pinMode (CAM_TRIG, INPUT_PULLUP);

  EasyBuzzer.setPin(BUZZER);
  pinMode (BUZZER, LOW);

  //Configure Limit Switch as Input
  pinMode (LIMIT_SWITCH,INPUT_PULLUP);

  //Setting up LCD
  LCD.begin(16,2); //Initializing LCD
  LCD.setCursor(0,1);
  LCD.print("Fabric Scanner");
  delay(2000);

  StepMotor.setSpeed(15);  //maximum speed of Stepper Motor
  
  LCD.setCursor(0,1);
  LCD.print("  Motor Homing");
  
  while (digitalRead(LIMIT_SWITCH) == HIGH){
  StepMotor.step(MOTOR_HOMING);
  MOTOR_HOMING++;
  delay(5);
  }

  MOTOR_HOMING  = 1;

   while (digitalRead(LIMIT_SWITCH) == LOW){
    StepMotor.step(MOTOR_HOMING);
    MOTOR_HOMING--;
    delay(5);
   }

   StepMotor.step(Steps_HomePositioning);

    LCD.setCursor(0,1);
    LCD.print("Homing Completed!");
    delay(2000);

  //attachInterrupt (digitalPinToInterrupt(REC_VT), Interrupt, RISING); //attaching interrupt to VT of the RF Receiver
  lgUpdateTime = millis(); //for sending data to the app

}

void loop()
{
  LCD.clear();

  serialEvent();
  
  ScannerState();
  
  if(State_Scanner == State_Idle)
    Idle();
    else if(State_Scanner == State_Scan)
      Scan();
      else if(State_Scanner == State_TestLED)
        TestLED();
        else if(State_Scanner == State_Scan1)
          Scan1(); 
}

void serialEvent() {
   while (Serial.available()) {
   BT_data = Serial.read();
   //Serial.print(BT_data);
   //Serial.print("\n");
   //{Serial.println(BT_Status);}
   }
   if (BT_data == '1'){
   State_BT = A;
   }
   else if (BT_data == '2'){
   State_BT = B;
   //{BT_Status = "LED TEST";}   
   }
   else if (BT_data == '3'){
   State_BT = C;
   //{BT_Status = " Standby";}
   //Serial.println("s");   
   }
   else if(BT_data == '4'){
   State_BT = D;
   }
   else
   State_BT = 0;
}

void Interrupt()
{
  if(digitalRead(REC_D0))             
    State_Remote = A;
  else if (digitalRead(REC_D1))
    State_Remote = B;
  else if (digitalRead(REC_D2))
    State_Remote = C;
  else if (digitalRead(REC_D3))
    State_Remote = D;
  else
    State_Remote = 0; 
}

void ScannerState()
{
  if(State_Remote == A || State_BT == A)
  {
    State_Scanner = State_Scan;
    State_Remote = 0;
    State_BT = 0;
  }
  else if(State_Remote == B || State_BT == B)
  {
    State_Scanner = State_TestLED;
    State_Remote = 0;
    State_BT = 0;
  }
  else if(State_Remote == D || State_BT == D)
  {
    State_Scanner = State_Scan1;
    State_Remote = 0;
    State_BT = 0;
  }
  else if(State_Remote == C || State_BT == C)
  {
    State_Scanner = State_Idle;
    State_Remote = 0;
    State_BT = 0;
  }
}

int CheckRemote()
{
  return State_Remote;      //Return the latest button press detected on receiver
}

void Idle()
{
  while(State_Scanner == State_Idle)                //keep it in the loop 
  {
    
    /*if (millis() - lgUpdateTime > 1000) {  //Send data to the app with an intevel of 1s (Testing Bluetooth)
    lgUpdateTime = millis();
    Serial.println("s");
  }*/

    serialEvent();
    LCD.setCursor(0,0);
    LCD.print("Quixel Megascan");                  

    CurrentMillis1 = millis();                            //Keep track of time for switching messages on LCD without using delay(wait) or interrupt
    if(CurrentMillis1-PreviousMillis1 > LCDInterval)
      PreviousMillis1 = CurrentMillis1;

    if((CurrentMillis1-PreviousMillis1) < (LCDInterval*2/5))     //Display message on second line of LCD for 0-0.4 of LCDInterval1
    {
      LCD.setCursor(0,1);
      LCD.print(" Fabric Scanner ");
    }
    else if((CurrentMillis1-PreviousMillis1) < (LCDInterval*3/5))    //Display instruction regarding A button on second line of LCD for 0.4-0.6 of LCDInterval1
    {
      LCD.setCursor(0,1);
      LCD.print("A = Start Scan   ");
    }
    else if((CurrentMillis1-PreviousMillis1) < (LCDInterval*4/5))    //Display instruction regarding B button on second line of LCD for 0.6-0.8 of LCDInterval1
    {
      LCD.setCursor(0,1);
      LCD.print("B = Test LEDs ");
    }
    ScannerState();                              //Check and update scanner state
  }
}

void TestLED()
{ 
  TestButtonCounter = 1;                          //Start test button counter from 1
  LCD.setCursor(0,0);
  LCD.print("   TEST LED  ");                    //Update display
  while (State_Scanner == State_TestLED)        //Remain in loop until change in scanner state
  {
    //Serial.println("l");  //Testing Bluetooth
    serialEvent();
    LCD.setCursor(0,1);
    if(CurrentMillis - PreviousMillis > LightsInterval) 
    PreviousMillis = millis();
    delay(500);
    switch(TestButtonCounter)                     //Cycle test button counter from 1 to 8 LEDs and BackPanel
    {
      case 1: digitalWrite(TOP_PANEL, HIGH); digitalWrite(LED_1, LOW); digitalWrite(LED_2, HIGH); digitalWrite(LED_3, HIGH); digitalWrite(LED_4, HIGH); digitalWrite(LED_5, HIGH); digitalWrite(LED_6, HIGH); digitalWrite(LED_7, HIGH); digitalWrite(LED_8, HIGH); digitalWrite(BACK_PANEL, HIGH);  LCD.print("  LED 1 is On   "); 
      CurrentMillis = millis(); if (CurrentMillis - PreviousMillis > LightsInterval) TestButtonCounter ++;  break;
      case 2: digitalWrite(TOP_PANEL, HIGH); digitalWrite(LED_1, HIGH); digitalWrite(LED_2, LOW); digitalWrite(LED_3, HIGH); digitalWrite(LED_4, HIGH); digitalWrite(LED_5, HIGH); digitalWrite(LED_6, HIGH); digitalWrite(LED_7, HIGH); digitalWrite(LED_8, HIGH); digitalWrite(BACK_PANEL, HIGH);  LCD.print("  LED 2 is On   "); 
      CurrentMillis = millis(); if (CurrentMillis - PreviousMillis > LightsInterval) TestButtonCounter ++;  break;
      case 3: digitalWrite(TOP_PANEL, HIGH); digitalWrite(LED_1, HIGH); digitalWrite(LED_2, HIGH); digitalWrite(LED_3, LOW); digitalWrite(LED_4, HIGH); digitalWrite(LED_5, HIGH); digitalWrite(LED_6, HIGH); digitalWrite(LED_7, HIGH); digitalWrite(LED_8, HIGH); digitalWrite(BACK_PANEL, HIGH);  LCD.print("  LED 3 is On   "); 
      CurrentMillis = millis(); if (CurrentMillis - PreviousMillis > LightsInterval) TestButtonCounter ++;   break;
      case 4: digitalWrite(TOP_PANEL, HIGH); digitalWrite(LED_1, HIGH); digitalWrite(LED_2, HIGH); digitalWrite(LED_3, HIGH); digitalWrite(LED_4, LOW); digitalWrite(LED_5, HIGH); digitalWrite(LED_6, HIGH); digitalWrite(LED_7, HIGH); digitalWrite(LED_8, HIGH); digitalWrite(BACK_PANEL, HIGH);  LCD.print("  LED 4 is On   "); 
      CurrentMillis = millis(); if (CurrentMillis - PreviousMillis > LightsInterval) TestButtonCounter ++;   break;
      case 5: digitalWrite(TOP_PANEL, HIGH); digitalWrite(LED_1, HIGH); digitalWrite(LED_2, HIGH); digitalWrite(LED_3, HIGH); digitalWrite(LED_4, HIGH); digitalWrite(LED_5, LOW); digitalWrite(LED_6, HIGH); digitalWrite(LED_7, HIGH); digitalWrite(LED_8, HIGH); digitalWrite(BACK_PANEL, HIGH);  LCD.print("  LED 5 is On   "); 
      CurrentMillis = millis(); if (CurrentMillis - PreviousMillis > LightsInterval) TestButtonCounter ++;   break;
      case 6: digitalWrite(TOP_PANEL, HIGH); digitalWrite(LED_1, HIGH); digitalWrite(LED_2, HIGH); digitalWrite(LED_3, HIGH); digitalWrite(LED_4, HIGH); digitalWrite(LED_5, HIGH); digitalWrite(LED_6, LOW); digitalWrite(LED_7, HIGH); digitalWrite(LED_8, HIGH); digitalWrite(BACK_PANEL, HIGH);  LCD.print("  LED 6 is On   "); 
      CurrentMillis = millis(); if (CurrentMillis - PreviousMillis > LightsInterval) TestButtonCounter ++;   break;
      case 7: digitalWrite(TOP_PANEL, HIGH); digitalWrite(LED_1, HIGH); digitalWrite(LED_2, HIGH); digitalWrite(LED_3, HIGH); digitalWrite(LED_4, HIGH); digitalWrite(LED_5, HIGH); digitalWrite(LED_6, HIGH); digitalWrite(LED_7, LOW); digitalWrite(LED_8, HIGH); digitalWrite(BACK_PANEL, HIGH);  LCD.print("  LED 7 is On   "); 
      CurrentMillis = millis(); if (CurrentMillis - PreviousMillis > LightsInterval) TestButtonCounter ++;   break;
      case 8: digitalWrite(TOP_PANEL, HIGH); digitalWrite(LED_1, HIGH); digitalWrite(LED_2, HIGH); digitalWrite(LED_3, HIGH); digitalWrite(LED_4, HIGH); digitalWrite(LED_5, HIGH); digitalWrite(LED_6, HIGH); digitalWrite(LED_7, HIGH); digitalWrite(LED_8, LOW); digitalWrite(BACK_PANEL, HIGH);  LCD.print("  LED 8 is On   "); 
      CurrentMillis = millis(); if (CurrentMillis - PreviousMillis > LightsInterval) TestButtonCounter ++;   break;
      case 9: digitalWrite(TOP_PANEL, HIGH); digitalWrite(LED_1, HIGH); digitalWrite(LED_2, HIGH); digitalWrite(LED_3, HIGH); digitalWrite(LED_4, HIGH); digitalWrite(LED_5, HIGH); digitalWrite(LED_6, HIGH); digitalWrite(LED_7, HIGH); digitalWrite(LED_8, HIGH); digitalWrite(BACK_PANEL, LOW);  LCD.print("Back Panel is On   "); 
      CurrentMillis = millis(); if (CurrentMillis - PreviousMillis > LightsInterval) TestButtonCounter ++;   break;
      default: digitalWrite(TOP_PANEL, HIGH); digitalWrite(LED_1, HIGH); digitalWrite(LED_2, HIGH); digitalWrite(LED_3, HIGH); digitalWrite(LED_4, HIGH); digitalWrite(LED_5, HIGH); digitalWrite(LED_6, HIGH); digitalWrite(LED_7, HIGH); digitalWrite(LED_8, HIGH); digitalWrite(BACK_PANEL, HIGH);  LCD.print(" All LED's Off   "); break;
    }

    if(CheckRemote() == B || State_BT == B)                      //Increment counter when B button is pressed on RF remote
    {
      PreviousMillis = millis();
      State_Remote = 0;
      State_BT = 0;
      TestButtonCounter++;
      if(TestButtonCounter > 10)
        TestButtonCounter = 1;
    }

    if(CheckRemote() == C || State_BT == C)                      //Turn all panels OFF and exit test panels state when C button is pressed on RF remote
    {
      digitalWrite(TOP_PANEL, HIGH); digitalWrite(LED_1, HIGH); digitalWrite(LED_2, HIGH); digitalWrite(LED_3, HIGH); digitalWrite(LED_4, HIGH); digitalWrite(LED_5, HIGH); digitalWrite(LED_6, HIGH); digitalWrite(LED_7, HIGH); digitalWrite(LED_8, HIGH); digitalWrite(BACK_PANEL, HIGH);
      ScannerState();
    }
  }
}

void Scan1()
{

  LCD.setCursor (1,0);
  LCD.print ("Homing SL CPL");    

  for(int x=0; x<=7; x++)                    
  {
    LCD.setCursor(0,1);
    LCD.print ("      LED"); LCD.print(x+1); LCD.print("      ");
    analogWrite(PWMpin[x], Slave_motState1[x]);                       //Rotating SL CPL to homing position
    Serial.print("Sending LED");
    Serial.print(x);
    Serial.print(": ");
    Serial.println(Slave_motState1[x]);
    delay(100);
    if (Cancel())
    return;
  }

  LCD.setCursor (0,1);
  LCD.print ("     ......     ");
  
  delay(25000);

  CameraWakeUp();     // Half Press to wake up the camera

  LCD.setCursor (1,0);
  LCD.print (" Initalizing...  ");
  serialEvent();

  LCD.setCursor (1,0);
  LCD.print ("   Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print("   Back Panel  "); 
  digitalWrite(BACK_PANEL, LOW);
  pinMode (R_PI, OUTPUT);
  digitalWrite (R_PI, LOW);        //Send LOW Signal to Raspberry Pi
  delay(1000);
  pinMode (R_PI, INPUT_PULLUP);       //Send HIGH Signal to Raspberry Pi
  delay(90000);
  digitalWrite(BACK_PANEL, HIGH);  //Turn Off Back Panel
  delay(DELAY_INPICS);
  if(Cancel())
  return;

  delay(500);
  
  LCD.setCursor (1,0);
  LCD.print ("Rotating SL CPL");    


  for(int x=0; x<=7; x++)                    
  {
    LCD.setCursor(0,1);
    LCD.print ("      LED"); LCD.print(x+1); LCD.print("      ");
    analogWrite(PWMpin[x], Slave_motState2[x]);                       //Rotating SL CPL to homing position
    Serial.print("Sending LED");
    Serial.print(x);
    Serial.print(": ");
    Serial.println(Slave_motState2[x]);
    delay(100);
  if (Cancel())
  return;
  }

  LCD.setCursor (0,1);
  LCD.print ("    ......     ");
  
  delay(30000);

  LCD.setCursor(0,1);
  LCD.print ("   Complete    ");

  Trigger();  // To ensure Camera triggers through 2.5mm cable

  delay(2000); // added because it was missing one shot

  for (int x =1; x<=8; x ++)
  {
  LCD.setCursor (1,0);
  LCD.print ("  Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("     LED "); LCD.print (x); LCD.print ("     ");
  digitalWrite (LED[x-1], LOW);             //Turn on Side Light
  Serial.println(LED[x-1]);
  delay(DELAY_LEDILLUMINATE);
  Trigger();
  delay(DELAY_SHUTTER1);
  digitalWrite (LED[x-1], HIGH);            //Turn off Side Light
  delay(DELAY_INPICS);
  if(Cancel())                           //Check for Cancel Button
  return;
  }

  USBReset();       // Disabling and enabling USB via Raspberry PI
  
  LCD.setCursor (1,0);
  LCD.print(" Rotating CPL  ");
  LCD.setCursor (0,1);
  LCD.print("   90 Degrees  ");           //Rotate Camera Polarizer to 90 degrees
  StepMotor.step(Steps_rotationC);
  if(Cancel())                          //Check for Cancel Button
  return;

  for (int x =1; x<=8; x ++)
  {
  LCD.setCursor (1,0);
  LCD.print ("  Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("     LED "); LCD.print (x); LCD.print ("     ");
  digitalWrite (LED[x-1], LOW);             //Turn on Side Light
  Serial.println(LED[x-1]);
  delay(DELAY_LEDILLUMINATE);
  Trigger();
  delay(DELAY_SHUTTER1);
  digitalWrite (LED[x-1], HIGH);            //Turn off Side Light
  delay(DELAY_INPICS);
  if(Cancel())                           //Check for Cancel Button
  return;
  }

  USBReset();       // Disabling and enabling USB via Raspberry PI

  LCD.setCursor (1,0);
  LCD.print ("Rotating SL CPL");  

    for(int x=0; x<=7; x++)                    
  {
    LCD.setCursor(0,1);
    LCD.print ("      LED"); LCD.print(x+1); LCD.print("      ");
    analogWrite(PWMpin[x], Slave_motState4[x]);                       //Rotating SL CPL to State 4 
    Serial.print("Sending LED");
    Serial.print(x);
    Serial.print(": ");
    Serial.println(Slave_motState4[x]);
    delay(100);
    if (Cancel())
    return;
  }

  LCD.setCursor (0,1);
  LCD.print ("    ......     ");
  
  delay(20000);

  LCD.setCursor(0,1);
  LCD.print ("   Complete    ");

  LCD.setCursor (1,0);
  LCD.print("  Rotating CPL  ");
  LCD.setCursor (0,1);
  LCD.print("  135 Degrees  ");
  StepMotor.step(Steps_rotationB);
  if(Cancel())                          //Check for Cancel Button
  return;

  for (int x =1; x<=8; x ++)
  {
  LCD.setCursor (1,0);
  LCD.print ("  Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("     LED "); LCD.print (x); LCD.print ("     ");
  digitalWrite (LED[x-1], LOW);             //Turn on Side Light
  Serial.println(LED[x-1]);
  delay(DELAY_LEDILLUMINATE);
  Trigger();
  delay(DELAY_SHUTTER1);
  digitalWrite (LED[x-1], HIGH);            //Turn off Side Light
  delay(DELAY_INPICS);
  if(Cancel())                           //Check for Cancel Button
  return;
  }

  USBReset();       // Disabling and enabling USB via Raspberry PI
  
  LCD.setCursor (1,0);
  LCD.print (" Homing SL CPL");

  for(int x=0; x<=7; x++)                    
  {
    LCD.setCursor(0,1);
    LCD.print ("      LED"); LCD.print(x+1); LCD.print("      ");
    analogWrite(PWMpin[x], Slave_motState1[x]);                       //Rotating SL CPL to homing position
    Serial.print("Sending LED");
    Serial.print(x);
    Serial.print(": ");
    Serial.println(Slave_motState1[x]);
    delay(100);
  }

  LCD.setCursor (0,1);
  LCD.print ("    ......     ");

  if (Cancel())
  return;
  
  delay(10000);

  LCD.setCursor(0,1);
  LCD.print ("   Complete    ");

  LCD.setCursor(0,1);
  LCD.print ("               ");

  LCD.setCursor(1,0);
  LCD.print(" Homing Cam CPL");
    while (digitalRead(LIMIT_SWITCH) == HIGH){
    StepMotor.step(MOTOR_HOMING);
    MOTOR_HOMING++;
    delay(5);
  }

  MOTOR_HOMING  = 1;
    while (digitalRead(LIMIT_SWITCH) == LOW){
    StepMotor.step(MOTOR_HOMING);
    MOTOR_HOMING--;
    delay(5);
   }

  StepMotor.step(Steps_HomePositioning);

    LCD.setCursor(0,1);
    LCD.print("Homing Completed");
    delay(2000);

  EasyBuzzer.beep(1000, 2);
  delay(2000);
  EasyBuzzer.stopBeep();

    State_Scanner = State_Idle;
}

int Cancel1()
{
  LCD.setCursor(1,0);
  LCD.print("Cancelling Scan... ");    //Display message on LCD

  LCD.setCursor (1,0);
  LCD.print (" Homing SL CPL");

  for(int x=0; x<=7; x++)                    
  {
    LCD.setCursor(0,1);
    LCD.print ("      LED"); LCD.print(x+1); LCD.print("      ");
    analogWrite(PWMpin[x], Slave_motState1[x]);                       //Rotating SL CPL to homing position
    Serial.print("Sending LED");
    Serial.print(x);
    Serial.print(": ");
    Serial.println(Slave_motState1[x]);
    delay(1000);
  }

  LCD.setCursor (0,1);
  LCD.print ("    ......     ");
  
  delay(30000);

  LCD.setCursor(0,1);
  LCD.print ("   Complete    ");

      LCD.setCursor(1,0);
  LCD.print("Cancelling Scan... ");    //Display message on LCD
    
    LCD.setCursor(1,0);
    LCD.print("Homing CAM CPL ");
  
    while (digitalRead(LIMIT_SWITCH) == HIGH){
    StepMotor.step(MOTOR_HOMING);
    MOTOR_HOMING++;
    delay(5);
  }

  MOTOR_HOMING  = 1;

    while (digitalRead(LIMIT_SWITCH) == LOW){
    StepMotor.step(MOTOR_HOMING);
    MOTOR_HOMING--;
    delay(5);
   }

   StepMotor.step(Steps_HomePositioning);

    LCD.setCursor(0,1);
    LCD.print("Homing Completed");
    delay(2000);

  EasyBuzzer.beep(1000, 2);
  delay(2000);
  EasyBuzzer.stopBeep();
  
  State_Scanner = State_Idle;
}

int Cancel()
{
  serialEvent();
  if(CheckRemote() == C || State_BT == C)        //Check if C button was pressed on remote
  {
    LCD.setCursor(1,0);
    LCD.print(" Cancelling Scan");    //Display message on LCD
    digitalWrite(TOP_PANEL, HIGH); digitalWrite(LED_1, HIGH); digitalWrite(LED_2, HIGH); digitalWrite(LED_3, HIGH); digitalWrite(LED_4, HIGH); digitalWrite(LED_5, HIGH); digitalWrite(LED_6, HIGH); digitalWrite(LED_7, HIGH); digitalWrite(LED_8, HIGH); digitalWrite(BACK_PANEL, HIGH);

  LCD.setCursor (1,0);
  LCD.print (" Homing SL CPL");

  for(int x=0; x<=7; x++)                    
  {
    LCD.setCursor(0,1);
    LCD.print ("      LED"); LCD.print(x+1); LCD.print("      ");
    analogWrite(PWMpin[x], Slave_motState1[x]);                       //Rotating SL CPL to homing position
    Serial.print("Sending LED");
    Serial.print(x);
    Serial.print(": ");
    Serial.println(Slave_motState1[x]);
    delay(1000);
  }

  LCD.setCursor (0,1);
  LCD.print ("    ......     ");
  
  delay(30000);

  LCD.setCursor(0,1);
  LCD.print ("   Complete    ");

  LCD.setCursor(1,0);
  LCD.print("Cancelling Scan... ");    //Display message on LCD
    
    LCD.setCursor(0,1);
    LCD.print(" Homing Cam CPL ");
  
    while (digitalRead(LIMIT_SWITCH) == HIGH){
    StepMotor.step(MOTOR_HOMING);
    MOTOR_HOMING++;
    delay(5);
  }

  MOTOR_HOMING  = 1;

    while (digitalRead(LIMIT_SWITCH) == LOW){
    StepMotor.step(MOTOR_HOMING);
    MOTOR_HOMING--;
    delay(5);
   }

   StepMotor.step(Steps_HomePositioning);

    LCD.setCursor(0,1);
    LCD.print("Homing Completed");
    delay(2000);

  EasyBuzzer.beep(1000, 2);
  delay(2000);
  EasyBuzzer.stopBeep();
  
  ScannerState();
  return 1;
  }
  else {
  return 0;
  }
}

void CameraWakeUp() {

  pinMode (CAM_FOCUS, OUTPUT);
  digitalWrite (CAM_FOCUS, LOW);   // This will wake up the Camera from sleep mode before Scan sequence is initiated
  delay(500);
  pinMode (CAM_TRIG, OUTPUT);
  pinMode (CAM_FOCUS, INPUT_PULLUP);
  delay(500);
}

void Trigger() {
  
  pinMode (CAM_FOCUS, OUTPUT);
  pinMode (CAM_TRIG, OUTPUT);
  digitalWrite (CAM_FOCUS, LOW);        //Turn on Camera Focus
  digitalWrite (CAM_TRIG, LOW);         //Turn on Camera Trigger
  delay(DELAY_SHUTTER);
  pinMode (CAM_FOCUS, INPUT_PULLUP);
  pinMode (CAM_TRIG, INPUT_PULLUP);
}

void USBReset() {
  
  LCD.setCursor(0,1);
  LCD.print(" Replugging Cam ");
  pinMode (R_PI1, OUTPUT);
  digitalWrite (R_PI1, LOW);        //Send LOW Signal to Raspberry Pi
  delay(1000);
  pinMode (R_PI1, INPUT_PULLUP);       //Send HIGH Signal to Raspberry Pi
  delay(3000);
}

void Scan() {

  CameraWakeUp();     // Half Press to wake up the camera

  LCD.setCursor (1,0);
  LCD.print (" Initalizing...  ");
  serialEvent();

  LCD.setCursor (1,0);
  LCD.print ("   Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print("   Back Panel  "); 
  digitalWrite(BACK_PANEL, LOW);
  pinMode (R_PI, OUTPUT);
  digitalWrite (R_PI, LOW);        //Send LOW Signal to Raspberry Pi
  delay(1000);
  pinMode (R_PI, INPUT_PULLUP);       //Send HIGH Signal to Raspberry Pi
  delay(90000);
  digitalWrite(BACK_PANEL, HIGH);  //Turn Off Back Panel
  delay(DELAY_INPICS);
  if(Cancel())
  return;

  Trigger();  // To ensure Camera triggers through 2.5mm cable

  delay(2000); // added because it was missing one shot
  
  /*LCD.setCursor (1,0);
  LCD.print ("   Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("    Top Panel  ");
  digitalWrite (TOP_PANEL, LOW);        //Turn on Top Panel
  delay(DELAY_LEDILLUMINATE);
  Trigger();
  delay(DELAY_SHUTTER1);
  digitalWrite (TOP_PANEL, HIGH);       //Turn off Top Panel
  delay(DELAY_INPICS);
  if(Cancel())                          //Check for Cancel Button
  return;

  LCD.setCursor (1,0);
  LCD.print("  Rotating CPL  ");
  LCD.setCursor (0,1);
  LCD.print("  22.5 Degrees  ");
  StepMotor.step(Steps_rotationB);
  if(Cancel())                          //Check for Cancel Button
  return;*/
  
  LCD.setCursor (1,0);
  LCD.print ("  Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("     LED 1      ");  
  digitalWrite (LED_1, LOW);             //Turn on LED 1
  delay(DELAY_LEDILLUMINATE);
  Trigger();
  delay(DELAY_SHUTTER1);
  digitalWrite (LED_1, HIGH);            //Turn off LED 1
  delay(DELAY_INPICS);
  if(Cancel())                          //Check for Cancel Button
  return;
  
  LCD.setCursor (1,0);
  LCD.print ("  Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("     LED 3       "); 
  digitalWrite (LED_3, LOW);            //Turn on LED 3
  delay(DELAY_LEDILLUMINATE);
  Trigger();
  delay(DELAY_SHUTTER1);
  digitalWrite (LED_3, HIGH);          //Turn off LED 3
  delay(DELAY_INPICS);
  if(Cancel())                          //Check for Cancel Button
  return;

  LCD.setCursor (1,0);
  LCD.print ("  Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("     LED 5       "); 
  digitalWrite (LED_5, LOW);            //Turn on LED 5
  delay(DELAY_LEDILLUMINATE);
  Trigger();
  delay(DELAY_SHUTTER1);
  digitalWrite (LED_5, HIGH);          //Turn off LED 5
  delay(DELAY_INPICS);
  if(Cancel())                          //Check for Cancel Button
  return;

  LCD.setCursor (1,0);
  LCD.print ("  Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("     LED 7       "); 
  digitalWrite (LED_7, LOW);            //Turn on LED 7
  delay(DELAY_LEDILLUMINATE);
  Trigger();
  delay(DELAY_SHUTTER1);
  digitalWrite (LED_7, HIGH);          //Turn off LED 7
  delay(DELAY_INPICS);
  if(Cancel())                          //Check for Cancel Button
  return;

  LCD.setCursor (1,0);
  LCD.print("  Rotating CPL  ");
  LCD.setCursor (0,1);
  LCD.print("  45 Degrees  ");
  StepMotor.step(Steps_rotationB);
  if(Cancel())                          //Check for Cancel Button
  return;
  
  LCD.setCursor (1,0);
  LCD.print ("  Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("     LED 2      ");  
  digitalWrite (LED_2, LOW);             //Turn on LED 2
  delay(DELAY_LEDILLUMINATE);
  Trigger();
  delay(DELAY_SHUTTER1);
  digitalWrite (LED_2, HIGH);           //Turn off LED 2
  delay(DELAY_INPICS);
  if(Cancel())                          //Check for Cancel Button
  return;
  
  LCD.setCursor (1,0);
  LCD.print ("  Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("     LED 4       "); 
  digitalWrite (LED_4, LOW);            //Turn on LED 4
  delay(DELAY_LEDILLUMINATE);
  Trigger();
  delay(DELAY_SHUTTER1);
  digitalWrite (LED_4, HIGH);          //Turn off LED 4
  delay(DELAY_INPICS);
  if(Cancel())                          //Check for Cancel Button
  return;

  LCD.setCursor (1,0);
  LCD.print ("  Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("     LED 6       "); 
  digitalWrite (LED_6, LOW);            //Turn on LED 6
  delay(DELAY_LEDILLUMINATE);
  Trigger();
  delay(DELAY_SHUTTER1);
  digitalWrite (LED_6, HIGH);          //Turn off LED 6
  delay(DELAY_INPICS);
  if(Cancel())                          //Check for Cancel Button
  return;

  LCD.setCursor (1,0);
  LCD.print ("  Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("     LED 8       "); 
  digitalWrite (LED_8, LOW);            //Turn on LED 8
  delay(DELAY_LEDILLUMINATE);
  Trigger();
  delay(DELAY_SHUTTER1);
  digitalWrite (LED_8, HIGH);          //Turn off LED 8
  delay(DELAY_INPICS);
  if(Cancel())                          //Check for Cancel Button
  return;  

  LCD.setCursor (1,0);
  LCD.print("  Rotating CPL  ");
  LCD.setCursor (0,1);
  LCD.print("  90 Degrees  ");
  StepMotor.step(Steps_rotationB);
  if(Cancel())                          //Check for Cancel Button
  return;

  USBReset();       // Disabling and enabling USB via Raspberry PI
  
  LCD.setCursor (1,0);
  LCD.print ("  Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("     LED 1      ");  
  digitalWrite (LED_1, LOW);             //Turn on LED 1
  delay(DELAY_LEDILLUMINATE);
  Trigger();
  delay(DELAY_SHUTTER1);
  digitalWrite (LED_1, HIGH);           //Turn off LED 1
  delay(DELAY_INPICS);
  if(Cancel())                          //Check for Cancel Button
  return;
  
  LCD.setCursor (1,0);
  LCD.print ("  Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("     LED 3       "); 
  digitalWrite (LED_3, LOW);            //Turn on LED 3
  delay(DELAY_LEDILLUMINATE);
  Trigger();
  delay(DELAY_SHUTTER1);
  digitalWrite (LED_3, HIGH);          //Turn off LED 3
  delay(DELAY_INPICS);
  if(Cancel())                          //Check for Cancel Button
  return;

  LCD.setCursor (1,0);
  LCD.print ("  Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("     LED 5       "); 
  digitalWrite (LED_5, LOW);            //Turn on LED 5
  delay(DELAY_LEDILLUMINATE);
  Trigger();
  delay(DELAY_SHUTTER1);
  digitalWrite (LED_5, HIGH);          //Turn off LED 5
  delay(DELAY_INPICS);
  if(Cancel())                          //Check for Cancel Button
  return;

  LCD.setCursor (1,0);
  LCD.print ("  Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("     LED 7       "); 
  digitalWrite (LED_7, LOW);            //Turn on LED 7
  delay(DELAY_LEDILLUMINATE);
  Trigger();
  delay(DELAY_SHUTTER1);
  digitalWrite (LED_7, HIGH);          //Turn off LED 7
  delay(DELAY_INPICS);
  if(Cancel())                          //Check for Cancel Button
  return;

  LCD.setCursor (1,0);
  LCD.print("  Rotating CPL  ");
  LCD.setCursor (0,1);
  LCD.print(" 135 Degrees ");
  StepMotor.step(Steps_rotationB);
  if(Cancel())                          //Check for Cancel Button
  return;

  LCD.setCursor (1,0);
  LCD.print ("  Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("     LED 2      ");  
  digitalWrite (LED_2, LOW);             //Turn on LED 2
  delay(DELAY_LEDILLUMINATE);
  Trigger();
  delay(DELAY_SHUTTER1);
  digitalWrite (LED_2, HIGH);           //Turn off LED 2
  delay(DELAY_INPICS);
  if(Cancel())                          //Check for Cancel Button
  return;
  
  LCD.setCursor (1,0);
  LCD.print ("  Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("     LED 4       "); 
  digitalWrite (LED_4, LOW);            //Turn on LED 4
  delay(DELAY_LEDILLUMINATE);
  Trigger();
  delay(DELAY_SHUTTER1); 
  digitalWrite (LED_4, HIGH);          //Turn off LED 4
  delay(DELAY_INPICS);
  if(Cancel())                         //Check for Cancel Button
  return;

  LCD.setCursor (1,0);
  LCD.print ("  Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("     LED 6       "); 
  digitalWrite (LED_6, LOW);            //Turn on LED 6
  delay(DELAY_LEDILLUMINATE);
  Trigger();
  delay(DELAY_SHUTTER1);
  digitalWrite (LED_6, HIGH);          //Turn off LED 6
  delay(DELAY_INPICS);
  if(Cancel())                         //Check for Cancel Button
  return; 

  LCD.setCursor (1,0);
  LCD.print ("  Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("     LED 8       "); 
  digitalWrite (LED_8, LOW);            //Turn on LED 8
  delay(DELAY_LEDILLUMINATE);
  Trigger();
  delay(DELAY_SHUTTER1);
  digitalWrite (LED_8, HIGH);          //Turn off LED 8
  delay(DELAY_INPICS);
  if(Cancel())                         //Check for Cancel Button
  return;

  USBReset();       // Disabling and enabling USB via Raspberry PI

    LCD.setCursor(0,1);
    LCD.print("  Motor Homing");
  
    while (digitalRead(LIMIT_SWITCH) == HIGH){
    StepMotor.step(MOTOR_HOMING);
    MOTOR_HOMING++;
    delay(5);
  }

  MOTOR_HOMING  = 1;

    while (digitalRead(LIMIT_SWITCH) == LOW){
    StepMotor.step(MOTOR_HOMING);
    MOTOR_HOMING--;
    delay(5);
   }

  StepMotor.step(Steps_HomePositioning);

    LCD.setCursor(0,1);
    LCD.print("Homing Completed");
    delay(2000);

  EasyBuzzer.beep(1000, 2);
  delay(2000);
  EasyBuzzer.stopBeep();

    State_Scanner = State_Idle;
}
