/*
Fabric Scanner 1x1 meters 
Board: Arduino Mega

9th December, 2021
*/

#include <Stepper.h>
#include <LiquidCrystal.h>
#include <EasyBuzzer.h>


//Indication LEDs
const int PWR_LED = 14, BUSY_LED = 15, TEST_LED =16;

const unsigned long LEDInterval = 50;       //For throubbing of status LED when in idle mode

//315MHZ RF Receiver
const int REC_D0 = 17, REC_D1 = 18, REC_D2 = 19, REC_D3 = 20, REC_VT = 21;

volatile byte State_Remote = 0;
int TestButtonCounter = 0; 

#define A 1
#define B 2
#define C 3
#define D 4

#define State_Idle 0
#define State_Scan 1
#define State_TestLED 2
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
const int Steps_rotationB = -3534;       //Rotate Polarizer to 67.5 degrees
const int Steps_rotationC = -1767;       //Rotate Polarizer to 90 degrees
const int Steps_rotationD = -1767;       //Rotate Plarizer to 112.5 degrees
const int Steps_rotationE = -3534;       //Rotate Polarizer to 157.5 degree
//const int Steps_rotationF = -1767;     //Rotate Polarizer to 180 degree
const int Steps_HomePositioning = -100;    //Position the motor Correctly

//Buzzer
const int BUZZER = 36;

//Camera Trigger and Focus
const int CAM_FOCUS = 37, CAM_TRIG = 38;

//16X2 LCD
const int LCD_D4 = 39, LCD_D5 = 40, LCD_D6 = 41, LCD_D7 = 42, LCD_EN = 43, /*LCD_RW = 44,*/ LCD_RS = 45;
LiquidCrystal LCD(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

//Setting delays for Scan sequence
const int DELAY_LEDILLUMINATE = 1000;          //delay (milli second) between Panel On and Shutter On
const int DELAY_SHUTTER = 5000;                //delay (milli second) for Camera Shutter (2000, 5000)
const int DELAY_SHUTTER1 = 500;                //Additional deay for Fuji GFX100
const int DELAY_INPICS = 1000;                 //delay (milli second) between pictures (500,1000)

//Setting delays for LCD 
unsigned long CurrentMillis = 0;
unsigned long PreviousMillis = 0;
unsigned long CurrentMillis1 = 0;
unsigned long PreviousMillis1 = 0;
const unsigned long LCDInterval = 5000;    //To cycle instructions regarding remote buttons on LCD when in idle mode


void setup() {

  //Configuring Pins for Indication LED as Output
  pinMode (PWR_LED, OUTPUT); pinMode (BUSY_LED, OUTPUT); pinMode (TEST_LED, OUTPUT);

  //Setting state of Indication LED at Startup
  digitalWrite (PWR_LED, HIGH); digitalWrite (BUSY_LED, LOW); digitalWrite (TEST_LED, LOW);
  
  //Configuring Pins for RF Receiver as INPUT
  pinMode (REC_D0, INPUT); pinMode (REC_D1, INPUT); pinMode (REC_D2, INPUT);
  pinMode (REC_D3, INPUT); pinMode (REC_VT, INPUT);

  //Configuring Pins for the LED PANELS and COBS as Output
  pinMode (LED_1, OUTPUT); pinMode (LED_2, OUTPUT); pinMode (LED_3, OUTPUT); pinMode (LED_4, OUTPUT); pinMode (LED_5, OUTPUT);
  pinMode (LED_6, OUTPUT); pinMode (LED_7, OUTPUT); pinMode (LED_8, OUTPUT); pinMode (TOP_PANEL, OUTPUT); pinMode (BACK_PANEL, OUTPUT);

  //Set LED PANELS and COBS Off
  digitalWrite (LED_1, HIGH); digitalWrite (LED_2, HIGH); digitalWrite (LED_3, HIGH); digitalWrite (LED_4, HIGH); digitalWrite (LED_5, HIGH); 
  digitalWrite (LED_6, HIGH); digitalWrite (LED_7, HIGH); digitalWrite (LED_8, HIGH); digitalWrite (TOP_PANEL, HIGH); digitalWrite (BACK_PANEL, HIGH);

  //Configuring Pins for Camera Focus and Trigger as Output
  pinMode (CAM_FOCUS, OUTPUT);
  pinMode (CAM_TRIG, OUTPUT);

  //Set Camera Focus and Trigger Low
  digitalWrite (CAM_FOCUS, HIGH);
  digitalWrite (CAM_TRIG, HIGH);

  //Configuring Pin for Buzzer as Output
  pinMode (CAM_FOCUS, OUTPUT);

  EasyBuzzer.setPin(BUZZER);

  //Configure Limit Switch as Input
  pinMode (LIMIT_SWITCH,INPUT_PULLUP);

  //digitalWrite (LIMIT_SWITCH, HIGH);

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
  Serial.print("CW");
  }

  MOTOR_HOMING  = 1;

   while (digitalRead(LIMIT_SWITCH) == LOW){
    StepMotor.step(MOTOR_HOMING);
    MOTOR_HOMING--;
    Serial.print("CCW");
    delay(5);
   }

   StepMotor.step(Steps_HomePositioning);

    LCD.setCursor(0,1);
    LCD.print("Homing Completed!");
    delay(2000);

  attachInterrupt (digitalPinToInterrupt(REC_VT), Interrupt, RISING); //attaching interrupt to VT of the RF Receiver
  
}

void loop() {

  LCD.clear();

if(State_Scanner == State_Idle)
    Idle();
  else if(State_Scanner == State_Scan)
    Scan();
  else if(State_Scanner == State_TestLED)
    TestLED();
}

void Interrupt()
{
  if(digitalRead(REC_D0))             
    State_Remote = A;
  else if (digitalRead(REC_D1))
    State_Remote = B;
  else if (digitalRead(REC_D2))
    State_Remote = C;
  /*else if (digitalRead(RemoteD3))
    State_Remote = D;*/
  else
    State_Remote = 0; 
}

void ScannerState()
{
  if(State_Remote == A)
  {
    State_Scanner = State_Scan;
    State_Remote = 0;
  }
  else if(State_Remote == B)
  {
    State_Scanner = State_TestLED;
    State_Remote = 0;
  }
  else if(State_Remote == C)
  {
    State_Scanner = State_Idle;
    State_Remote = 0;
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
    
    CurrentMillis = millis();                     //Keep track of time to blink LED without using delay(wait) or interrupt
    if(CurrentMillis-PreviousMillis > 2000)
      PreviousMillis = CurrentMillis;
      
    /*if(CurrentMillis-PreviousMillis > LEDInterval)
      digitalWrite(BUSY_LED, LOW);
    else
      digitalWrite(BUSY_LED, HIGH);*/
  }
}

void TestLED()
{
  TestButtonCounter = 1;                          //Start test button counter from 1
  LCD.setCursor(0,0);
  LCD.print("   TEST LED  ");                    //Update display
  while (State_Scanner == State_TestLED)        //Remain in loop until change in scanner state
  {
    LCD.setCursor(0,1);
    switch(TestButtonCounter)                     //Cycle test button counter from 1 to 6 for six panels
    {
      case 1: digitalWrite(TOP_PANEL, LOW); digitalWrite(LED_1, HIGH); digitalWrite(LED_2, HIGH); digitalWrite(LED_3, HIGH); digitalWrite(LED_4, HIGH); digitalWrite(LED_5, HIGH); digitalWrite(LED_6, HIGH); digitalWrite(LED_7, HIGH); digitalWrite(LED_8, HIGH);  LCD.print(" Top Panel is On  "); break;
      case 2: digitalWrite(TOP_PANEL, HIGH); digitalWrite(LED_1, LOW); digitalWrite(LED_2, HIGH); digitalWrite(LED_3, HIGH); digitalWrite(LED_4, HIGH); digitalWrite(LED_5, HIGH); digitalWrite(LED_6, HIGH); digitalWrite(LED_7, HIGH); digitalWrite(LED_8, HIGH);  LCD.print("  LED 1 is On   "); break;
      case 3: digitalWrite(TOP_PANEL, HIGH); digitalWrite(LED_1, HIGH); digitalWrite(LED_2, LOW); digitalWrite(LED_3, HIGH); digitalWrite(LED_4, HIGH); digitalWrite(LED_5, HIGH); digitalWrite(LED_6, HIGH); digitalWrite(LED_7, HIGH); digitalWrite(LED_8, HIGH);  LCD.print("  LED 2 is On   "); break;
      case 4: digitalWrite(TOP_PANEL, HIGH); digitalWrite(LED_1, HIGH); digitalWrite(LED_2, HIGH); digitalWrite(LED_3, LOW); digitalWrite(LED_4, HIGH); digitalWrite(LED_5, HIGH); digitalWrite(LED_6, HIGH); digitalWrite(LED_7, HIGH); digitalWrite(LED_8, HIGH);  LCD.print("  LED 3 is On   "); break;
      case 5: digitalWrite(TOP_PANEL, HIGH); digitalWrite(LED_1, HIGH); digitalWrite(LED_2, HIGH); digitalWrite(LED_3, HIGH); digitalWrite(LED_4, LOW); digitalWrite(LED_5, HIGH); digitalWrite(LED_6, HIGH); digitalWrite(LED_7, HIGH); digitalWrite(LED_8, HIGH);  LCD.print("  LED 4 is On   "); break;
      case 6: digitalWrite(TOP_PANEL, HIGH); digitalWrite(LED_1, HIGH); digitalWrite(LED_2, HIGH); digitalWrite(LED_3, HIGH); digitalWrite(LED_4, HIGH); digitalWrite(LED_5, LOW); digitalWrite(LED_6, HIGH); digitalWrite(LED_7, HIGH); digitalWrite(LED_8, HIGH);  LCD.print("  LED 5 is On   "); break;
      case 7: digitalWrite(TOP_PANEL, HIGH); digitalWrite(LED_1, HIGH); digitalWrite(LED_2, HIGH); digitalWrite(LED_3, HIGH); digitalWrite(LED_4, HIGH); digitalWrite(LED_5, HIGH); digitalWrite(LED_6, LOW); digitalWrite(LED_7, HIGH); digitalWrite(LED_8, HIGH);  LCD.print("  LED 6 is On   "); break;
      case 8: digitalWrite(TOP_PANEL, HIGH); digitalWrite(LED_1, HIGH); digitalWrite(LED_2, HIGH); digitalWrite(LED_3, HIGH); digitalWrite(LED_4, HIGH); digitalWrite(LED_5, HIGH); digitalWrite(LED_6, HIGH); digitalWrite(LED_7, LOW); digitalWrite(LED_8, HIGH);  LCD.print("  LED 7 is On   "); break;
      case 9: digitalWrite(TOP_PANEL, HIGH); digitalWrite(LED_1, HIGH); digitalWrite(LED_2, HIGH); digitalWrite(LED_3, HIGH); digitalWrite(LED_4, HIGH); digitalWrite(LED_5, HIGH); digitalWrite(LED_6, HIGH); digitalWrite(LED_7, HIGH); digitalWrite(LED_8, LOW);  LCD.print("  LED 8 is On   "); break;
      default: digitalWrite(TOP_PANEL, HIGH); digitalWrite(LED_1, HIGH); digitalWrite(LED_2, HIGH); digitalWrite(LED_3, HIGH); digitalWrite(LED_4, HIGH); digitalWrite(LED_5, HIGH); digitalWrite(LED_6, HIGH); digitalWrite(LED_7, HIGH); digitalWrite(LED_8, HIGH);  LCD.print(" All LED's Off   "); break;
    }
    if(CheckRemote() == B)                      //Increment counter when B button is pressed on RF remote
    {
      State_Remote = 0;
      TestButtonCounter++;
      if(TestButtonCounter > 10)
        TestButtonCounter = 1;
    }

    if(CheckRemote() == C)                      //Turn all panels OFF and exit test panels state when C button is pressed on RF remote
    {
      digitalWrite(TOP_PANEL, HIGH); digitalWrite(LED_1, HIGH); digitalWrite(LED_2, HIGH); digitalWrite(LED_3, HIGH); digitalWrite(LED_4, HIGH); digitalWrite(LED_5, HIGH); digitalWrite(LED_6, HIGH); digitalWrite(LED_7, HIGH); digitalWrite(LED_8, HIGH);
      ScannerState();
    }
  }
}

int Cancel()
{
  if(CheckRemote() == C)        //Check if C button was pressed on remote
  {
    LCD.setCursor(1,0);
    LCD.print(" Cancelling Scan... ");    //Display message on LCD
    digitalWrite(TOP_PANEL, HIGH); digitalWrite(LED_1, HIGH); digitalWrite(LED_2, HIGH); digitalWrite(LED_3, HIGH); digitalWrite(LED_4, HIGH); digitalWrite(LED_5, HIGH); digitalWrite(LED_6, HIGH); digitalWrite(LED_7, HIGH); digitalWrite(LED_8, HIGH);
    LCD.setCursor(0,1);
    LCD.print("  Motor Homing");
    
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

  EasyBuzzer.beep(1000, 2);
  delay(2000);
  EasyBuzzer.stopBeep();
  
  ScannerState();
  return 1;
  }
 
  else
  return 0;
  //State_Scanner = State_Idle;
}

void Scan() {

  digitalWrite (BUSY_LED, HIGH);

  LCD.setCursor (1,0);
  LCD.print ("   Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("    Top Panel  ");
  digitalWrite (TOP_PANEL, LOW);        //Turn on Top Panel
  delay(DELAY_LEDILLUMINATE);
  digitalWrite (CAM_FOCUS, LOW);        //Turn on Camera Focus
  digitalWrite (CAM_TRIG, LOW);         //Turn on Camera Trigger
  delay(DELAY_SHUTTER);
  digitalWrite (CAM_FOCUS, HIGH);       //Turn off Camera Focus
  digitalWrite (CAM_TRIG, HIGH);        //Turn off Camera Trigger
  delay(DELAY_SHUTTER1);
  digitalWrite (TOP_PANEL, HIGH);       //Turn off Top Panel
  delay(DELAY_INPICS);
  if(Cancel())                          //Check for Cancel Button
  return;


  LCD.setCursor (1,0);
  LCD.print("Rotating Polarizer");
  LCD.setCursor (0,1);
  LCD.print("  22.5°  ");
  StepMotor.step(Steps_rotationA);
  

  LCD.setCursor (1,0);
  LCD.print ("  Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("     LED 1      ");  
  digitalWrite (LED_1, LOW);             //Turn on LED 1
  delay(DELAY_LEDILLUMINATE);
  digitalWrite (CAM_FOCUS, LOW);         //Turn on Camera Focus
  digitalWrite (CAM_TRIG, LOW);          //Turn on Camera Trigger
  delay(DELAY_SHUTTER);
  digitalWrite (CAM_FOCUS, HIGH);        //Turn off Camera Focus
  digitalWrite (CAM_TRIG, HIGH);         //Turn off Camera Trigger
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
  digitalWrite (CAM_FOCUS, LOW);        //Turn on Camera Focus
  digitalWrite (CAM_TRIG, LOW);        //Turn on Camera Trigger
  delay(DELAY_SHUTTER);
  digitalWrite (CAM_FOCUS, HIGH);      //Turn off Camera Focus
  digitalWrite (CAM_TRIG, HIGH);       //Turn off Camera Trigger
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
  digitalWrite (CAM_FOCUS, LOW);        //Turn on Camera Focus
  digitalWrite (CAM_TRIG, LOW);        //Turn on Camera Trigger
  delay(DELAY_SHUTTER);
  digitalWrite (CAM_FOCUS, HIGH);      //Turn off Camera Focus
  digitalWrite (CAM_TRIG, HIGH);       //Turn off Camera Trigger
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
  digitalWrite (CAM_FOCUS, LOW);        //Turn on Camera Focus
  digitalWrite (CAM_TRIG, LOW);        //Turn on Camera Trigger
  delay(DELAY_SHUTTER);
  digitalWrite (CAM_FOCUS, HIGH);      //Turn off Camera Focus
  digitalWrite (CAM_TRIG, HIGH);       //Turn off Camera Trigger
  delay(DELAY_SHUTTER1);
  digitalWrite (LED_7, HIGH);          //Turn off LED 7
  delay(DELAY_INPICS);
  if(Cancel())                          //Check for Cancel Button
  return;

  LCD.setCursor (1,0);
  LCD.print("  Rotating Polarizer    ");
  LCD.setCursor (0,1);
  LCD.print("  67.5°  ");
  StepMotor.step(Steps_rotationB);
  

  LCD.setCursor (1,0);
  LCD.print ("  Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("     LED 2      ");  
  digitalWrite (LED_2, LOW);             //Turn on LED 2
  delay(DELAY_LEDILLUMINATE);
  digitalWrite (CAM_FOCUS, LOW);        //Turn on Camera Focus
  digitalWrite (CAM_TRIG, LOW);         //Turn on Camera Trigger
  delay(DELAY_SHUTTER);
  digitalWrite (CAM_FOCUS, HIGH);       //Turn off Camera Focus
  digitalWrite (CAM_TRIG, HIGH);        //Turn off Camera Trigger
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
  digitalWrite (CAM_FOCUS, LOW);        //Turn on Camera Focus
  digitalWrite (CAM_TRIG, LOW);        //Turn on Camera Trigger
  delay(DELAY_SHUTTER);
  digitalWrite (CAM_FOCUS, HIGH);      //Turn off Camera Focus
  digitalWrite (CAM_TRIG, HIGH);       //Turn off Camera Trigger
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
  digitalWrite (CAM_FOCUS, LOW);        //Turn on Camera Focus
  digitalWrite (CAM_TRIG, LOW);        //Turn on Camera Trigger
  delay(DELAY_SHUTTER);
  digitalWrite (CAM_FOCUS, HIGH);      //Turn off Camera Focus
  digitalWrite (CAM_TRIG, HIGH);       //Turn off Camera Trigger
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
  digitalWrite (CAM_FOCUS, LOW);        //Turn on Camera Focus
  digitalWrite (CAM_TRIG, LOW);        //Turn on Camera Trigger
  delay(DELAY_SHUTTER);
  digitalWrite (CAM_FOCUS, HIGH);      //Turn off Camera Focus
  digitalWrite (CAM_TRIG, HIGH);       //Turn off Camera Trigger
  delay(DELAY_SHUTTER1);
  digitalWrite (LED_8, HIGH);          //Turn off LED 8
  delay(DELAY_INPICS);
  if(Cancel())                          //Check for Cancel Button
  return;  


  LCD.setCursor (1,0);
  LCD.print("  Rotating Polarizer   ");
  LCD.setCursor (0,1);
  LCD.print("  90  °");
  StepMotor.step(Steps_rotationC);
  

  LCD.setCursor (1,0);
  LCD.print ("   Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("    Top Panel  ");
  digitalWrite (TOP_PANEL, LOW);        //Turn on Top Panel
  delay(DELAY_LEDILLUMINATE);
  digitalWrite (CAM_FOCUS, LOW);        //Turn on Camera Focus
  digitalWrite (CAM_TRIG, LOW);         //Turn on Camera Trigger
  delay(DELAY_SHUTTER);
  digitalWrite (CAM_FOCUS, HIGH);       //Turn off Camera Focus
  digitalWrite (CAM_TRIG, HIGH);        //Turn off Camera Trigger
  delay(DELAY_SHUTTER1);
  digitalWrite (TOP_PANEL, HIGH);       //Turn off Top Panel
  delay(DELAY_INPICS);
  if(Cancel())                          //Check for Cancel Button
  return;


  LCD.setCursor (1,0);
  LCD.print("Rotating Polarizer");
  LCD.setCursor (0,1);
  LCD.print("  112.5°  ");
  StepMotor.step(Steps_rotationD);
  

  LCD.setCursor (1,0);
  LCD.print ("  Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("     LED 1      ");  
  digitalWrite (LED_1, LOW);             //Turn on LED 1
  delay(DELAY_LEDILLUMINATE);
  digitalWrite (CAM_FOCUS, LOW);        //Turn on Camera Focus
  digitalWrite (CAM_TRIG, LOW);         //Turn on Camera Trigger
  delay(DELAY_SHUTTER);
  digitalWrite (CAM_FOCUS, HIGH);       //Turn off Camera Focus
  digitalWrite (CAM_TRIG, HIGH);        //Turn off Camera Trigger
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
  digitalWrite (CAM_FOCUS, LOW);        //Turn on Camera Focus
  digitalWrite (CAM_TRIG, LOW);        //Turn on Camera Trigger
  delay(DELAY_SHUTTER);
  digitalWrite (CAM_FOCUS, HIGH);      //Turn off Camera Focus
  digitalWrite (CAM_TRIG, HIGH);       //Turn off Camera Trigger
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
  digitalWrite (CAM_FOCUS, LOW);        //Turn on Camera Focus
  digitalWrite (CAM_TRIG, LOW);        //Turn on Camera Trigger
  delay(DELAY_SHUTTER);
  digitalWrite (CAM_FOCUS, HIGH);      //Turn off Camera Focus
  digitalWrite (CAM_TRIG, HIGH);       //Turn off Camera Trigger
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
  digitalWrite (CAM_FOCUS, LOW);        //Turn on Camera Focus
  digitalWrite (CAM_TRIG, LOW);        //Turn on Camera Trigger
  delay(DELAY_SHUTTER);
  digitalWrite (CAM_FOCUS, HIGH);      //Turn off Camera Focus
  digitalWrite (CAM_TRIG, HIGH);       //Turn off Camera Trigger
  delay(DELAY_SHUTTER1);
  digitalWrite (LED_7, HIGH);          //Turn off LED 7
  delay(DELAY_INPICS);
  if(Cancel())                          //Check for Cancel Button
  return;


  LCD.setCursor (1,0);
  LCD.print("Rotating Polarizer");
  LCD.setCursor (0,1);
  LCD.print("157.5°");
  StepMotor.step(Steps_rotationE);


  LCD.setCursor (1,0);
  LCD.print ("  Scanning...  ");
  LCD.setCursor(0,1);
  LCD.print ("     LED 2      ");  
  digitalWrite (LED_2, LOW);             //Turn on LED 2
  delay(DELAY_LEDILLUMINATE);
  digitalWrite (CAM_FOCUS, LOW);        //Turn on Camera Focus
  digitalWrite (CAM_TRIG, LOW);         //Turn on Camera Trigger
  delay(DELAY_SHUTTER);
  digitalWrite (CAM_FOCUS, HIGH);       //Turn off Camera Focus
  digitalWrite (CAM_TRIG, HIGH);        //Turn off Camera Trigger
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
  digitalWrite (CAM_FOCUS, LOW);       //Turn on Camera Focus
  digitalWrite (CAM_TRIG, LOW);        //Turn on Camera Trigger
  delay(DELAY_SHUTTER);
  digitalWrite (CAM_FOCUS, HIGH);      //Turn off Camera Focus
  digitalWrite (CAM_TRIG, HIGH);       //Turn off Camera Trigger
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
  digitalWrite (CAM_FOCUS, LOW);        //Turn on Camera Focus
  digitalWrite (CAM_TRIG, LOW);        //Turn on Camera Trigger
  delay(DELAY_SHUTTER);
  digitalWrite (CAM_FOCUS, HIGH);      //Turn off Camera Focus
  digitalWrite (CAM_TRIG, HIGH);       //Turn off Camera Trigger
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
  digitalWrite (CAM_FOCUS, LOW);        //Turn on Camera Focus
  digitalWrite (CAM_TRIG, LOW);        //Turn on Camera Trigger
  delay(DELAY_SHUTTER);
  digitalWrite (CAM_FOCUS, HIGH);      //Turn off Camera Focus
  digitalWrite (CAM_TRIG, HIGH);       //Turn off Camera Trigger
  delay(DELAY_SHUTTER1);
  digitalWrite (LED_8, HIGH);          //Turn off LED 8
  delay(DELAY_INPICS);
  if(Cancel())                         //Check for Cancel Button
  return;


  
    LCD.setCursor(1,0);
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

    LCD.setCursor(0,1);
    LCD.print("Homing Completed!");
    delay(2000);

  StepMotor.step(Steps_HomePositioning);

  EasyBuzzer.beep(1000, 2);
  delay(2000);
  EasyBuzzer.stopBeep();
   

  State_Scanner = State_Idle;
}
