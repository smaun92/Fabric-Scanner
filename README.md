# Fabric-Scanner
Arduino code for 1x1 meter Scanner which uses Arduino Mega 2560
Three Scanner States : Idle, LED test and Auto-Scan 
8 Side Lights at 70 degree angle and 1 Top Panel are turned on and off with relays
28BYJ-48 geared motor is used with ULN2003 Motor driver
16x2 LCD is used to display the state of the Scanner
Stepper library is used to control the motor and LiquidCrystal library is used for 16x2 LCD
EasyBuzzer is used for the Buzzer for it to beep at the end of the Scan
Limit Switch is used for homing the Motor at Start up and end of every Scan 
RF receiver is being used to Test Panels or do Auto-Scan
In Auto-Scan sequence Camera take 18 images. 9 Polarized and 9 Non-Polarized in following order
Top Panel Polarized
LED 1 Non-Polarized
LED 3 Polarized
LED 5 Non-Polarized
LED 7 Polarized
LED 2 Non-Polarized
LED 4 Polarized
LED 6 Non-Polarized
LED 8 Polarized
Top Panel Non-Polarized
LED 1 Polarized
LED 3 Non-Polarized
LED 5 Polarized
LED 7 Non-Polarized
LED 2 Polarized
LED 4 Non-Polarized
LED 6 Polarized
LED 8 Non-Polarized
Camera: Fujifilm GFX 100s
Lens: Fixed Lens 63mm and 250mm
