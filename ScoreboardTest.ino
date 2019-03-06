//**************************************************************//
//  Name    : Scoreboard
//  Author  : Skip Morrow
//  Date    : 2/13/2016
//  Modified: 2/13/2016
//  Version : 0.0
//  Notes   : Code for using a TPIC6A595 Shift Register 
//  This code was tested on 2/13/2016 and it works.
//
//  Updated on 6/26/2016 to add a "screensaver" functionality to
//  make the scoreboard beep after a certain period of inactivity
//  to prevent to battery from running down.
//
//****************************************************************


// Scoreboard is wired like this:
//
//    A Green
//  --------------
//  |            |
//  | B Lt       | C Brown
//  | Green      |
//  |            |
//  -------------- D Lt Brown
//  |            |
//  | E Orange   | F Lt Orange
//  |            |
//  |            |
//  --------------
//    G Blue
//
//  Segments are wired in order. A (Green) is connected
//  to output 0. And G (Blue) is connect to output 6.
//  Output 7 is not used.
// 
//  F 0010 0000  -- 1
//  G 0100 0000  -- 2
//  UNUSED       -- 4
//  A 0000 0001  -- 8
//  B 0000 0010  -- 16
//  C 0000 0100  -- 32
//  D 0000 1000  -- 64
//  E 0001 0000  -- 128

//  Digits are made like this:
//              1
//              2631
//              8426 8421
//    EDCB A GF EDCB A GF
//  0 E CB A GF 1011 1011  -- 187d BBh
//  1   C     F 0010 0001  -- 33d  21h
//  2 EDC  A G  1110 1010  -- 234d EAh
//  3  DC  A GF 0110 1011  -- 107d 68h
//  4  DCB    F 0111 0001  -- 113d 71h
//  5  D B A GF 0101 1011  -- 91d  5Bh
//  6 ED B A GF 1101 1011  -- 219d DBh
//  7   C  A  F 0010 1001  -- 41d  29h
//  8 EDCB A GF 1111 1011  -- 251d FBh
//  9  DCB A GF 0111 1011  -- 123d 7Bh

int encodedSsd[] = {
  187,
  33,
  234,
  107,
  113,
  91,
  219,
  41,
  251,
  123
};

#include "Wire.h"
#define DS3231_I2C_ADDRESS 0x68

//Pin connected to RCK (ST_CP) of TPIC (Pin 7)
int latchPin = 8;
//Pin connected to SRCK (SH_CP) of TPIC (Pin 8)
int clockPin = 11;
////Pin connected to SER IN (DS) of TPIC (Pin 18)
int dataPin = 10;

const int VIS_UP_PIN = 7; // orange-white, right side
const int CLOCK_PIN  = 6; // orange, small clock button, press to display the current time
const int HM_UP_PIN  = 5; // green-white, left side 
const int RESET_PIN  = 4; // brown-white
const int GND_PIN    = 3; // blue-white. This is hardwired to ground through the shield, so it cannot be changed.
const int HM_DN_PIN  = 2; // green, left side
const int UNUSEDPIN1  = 1;// pin 4 unused, blue
const int VIS_DN_PIN = 0; // brown, right side

const int BUZZER_PIN = 12;

const int BUTTON_RPT_DELAY = 250;
const int BUTTON_PRESS_BUZZER_LENGTH = 125;

// pins used to power the RTC
const int RTC_GND_PIN = 18;
const int RTC_VCC_PIN = 19;

// home is the left side, visitors is the right side
int homeScore = 0;
int visScore = 0;

bool clockMode = false;

// keep track of the last button press to beep if no
// buttons are pressed in a certain amount of time to
// save the battery.
unsigned long lastButtonPress = 0;
const long buttonDelay = 900000; //15 minutes * 60 sec/min * 1000 millis/sec

// runs once
void setup() {

  //set pins to output so you can control the shift register
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  
  pinMode(BUZZER_PIN, OUTPUT);

  // input pins connected to switches
  pinMode(VIS_UP_PIN, INPUT_PULLUP);
  pinMode(VIS_DN_PIN, INPUT_PULLUP);
  pinMode(HM_UP_PIN, INPUT_PULLUP);
  pinMode(HM_DN_PIN, INPUT_PULLUP);
  pinMode(RESET_PIN, INPUT_PULLUP);
  pinMode(CLOCK_PIN, INPUT_PULLUP);
  pinMode(UNUSEDPIN1, INPUT_PULLUP);

  //turn the RTC on
  pinMode(RTC_GND_PIN, OUTPUT);
  pinMode(RTC_VCC_PIN, OUTPUT);
  digitalWrite(RTC_GND_PIN, LOW);
  digitalWrite(RTC_VCC_PIN, HIGH);
  delay(1000);

  Wire.begin();

  //connected to all switches
  //pinMode(GND_PIN, OUTPUT);
  //digitalWrite(GND_PIN, LOW); //provides a ground for the buttons

  Post();

  //                      left       right
  DisplayFourDigitScore(homeScore, visScore);

  
  // I am using pins 0 & 1. If I enable the Serial.begin() line, it will cause
  // problems with pins 0 & 1.
  //Serial.begin(9600); // normally leave commented out

  // There are two ways to set the time. The easy way is to use
  // the buttons as described further below. Alternatively, you
  // can use this line of code, but it requires reflashing the
  // code to the arduino.
  //
  // To set the time, uncomment the line below and enter the
  // correct values. Then comment out the Serial.begin(9600) above.
  // Upload the code and startup the arduino. Then shut it down
  // and set everything back to normal. Restart it again and the
  // time should be set.
  //
  //              sec, min, hour, day, date, month, year
  //setDS3231time(0,   17,  21,   6,   1,    4,     16);

  lastButtonPress = millis();
  //SegmentTest(1000);
  //TestAllCombos(250);
}

void loop() {
  if(digitalRead(VIS_UP_PIN)==LOW) {
    lastButtonPress = millis();
    clockMode = false;
    //Serial.println("VIS_UP");
    visScore++;
    if (visScore>99) {visScore=99;}
    DisplayFourDigitScore(homeScore, visScore);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(BUTTON_PRESS_BUZZER_LENGTH);
    digitalWrite(BUZZER_PIN, LOW);
    delay(BUTTON_RPT_DELAY);
  }
  if(digitalRead(HM_UP_PIN)==LOW) {
    lastButtonPress = millis();
    clockMode = false;
    //Serial.println("HM_UP");
    homeScore++;
    if (homeScore>99) {homeScore=99;}
    DisplayFourDigitScore(homeScore, visScore);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(BUTTON_PRESS_BUZZER_LENGTH);
    digitalWrite(BUZZER_PIN, LOW);
    delay(BUTTON_RPT_DELAY);
  }  
  if(digitalRead(VIS_DN_PIN)==LOW) {
    lastButtonPress = millis();
    clockMode = false;
    //Serial.println("VIS_DN");
    visScore--;
    if (visScore<0) {visScore=0;}
    DisplayFourDigitScore(homeScore, visScore);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(BUTTON_PRESS_BUZZER_LENGTH);
    digitalWrite(BUZZER_PIN, LOW);
    delay(BUTTON_RPT_DELAY);
  }  
  if(digitalRead(HM_DN_PIN)==LOW) {
    lastButtonPress = millis();
    clockMode = false;
    //Serial.println("HM_DN");
    homeScore--;
    if (homeScore<0) {homeScore=0;}
    DisplayFourDigitScore(homeScore, visScore);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(BUTTON_PRESS_BUZZER_LENGTH);
    digitalWrite(BUZZER_PIN, LOW);
    delay(BUTTON_RPT_DELAY);
  }  
  if(digitalRead(RESET_PIN)==LOW) {
    lastButtonPress = millis();
    clockMode = false;
    //Serial.println("RESET BUTTON");
    homeScore=0;
    visScore=0;
    DisplayFourDigitScore(homeScore, visScore);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(750);
    digitalWrite(BUZZER_PIN, LOW);
    delay(BUTTON_RPT_DELAY);
  } 
  if(digitalRead(CLOCK_PIN)==LOW) {
    lastButtonPress = millis();
    clockMode = true;
    //Serial.println("CLOCK");
    ShowTime();
    delay(BUTTON_RPT_DELAY);
    
    while(digitalRead(CLOCK_PIN)==LOW) {
      if (digitalRead(RESET_PIN)==LOW && digitalRead(VIS_UP_PIN)==LOW) {
        // set the time by setting the score to the desired time using 24 hr
        // time. Then hold down the time button (it will show the current set time)
        // plus the reset button and the right blue button.
        //
        //            sec, min,       hour,      day, date, month, year
        setDS3231time(0,   visScore,  homeScore,   0,   1,    1,     00);
        ShowTime();
        digitalWrite(BUZZER_PIN, HIGH);
        delay(25);
        digitalWrite(BUZZER_PIN, LOW);
        delay(25);
        digitalWrite(BUZZER_PIN, HIGH);
        delay(25);
        digitalWrite(BUZZER_PIN, LOW);
        delay(2500);
        }
      };
      
    //DisplayFourDigitScore(homeScore, visScore);
    //delay(100);
  } 

  if (clockMode == true) {
    ShowTime();
    delay(BUTTON_RPT_DELAY);
  }

  if (millis() > lastButtonPress + buttonDelay) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(750);
    digitalWrite(BUZZER_PIN, LOW);
    delay(BUTTON_RPT_DELAY);

    // beep once per minute, so add some time to the lastButtonPress time
    lastButtonPress = lastButtonPress + 60000;
  }

}

void ShowTime() {
  byte second, minute, hour;
  getTime(&second, &minute, &hour);
  if (hour % 12 == 0) {
    hour = 12;
  } else {
    hour = hour % 12;
  }
  DisplayTime(hour, minute); 
}

void DisplayTime(int hour, int minute) {
  int hourOnesDigit = hour % 10;
  int hourTensDigit = (hour - hourOnesDigit) / 10;
  int minuteOnesDigit = minute % 10;
  int minuteTensDigit = (minute - minuteOnesDigit) / 10;
  digitalWrite(latchPin, LOW);
  // shift out the bits:

  // minutes first
  // we show leading zeroes for minutes
  shiftOut(dataPin, clockPin, MSBFIRST, encodedSsd[minuteOnesDigit]);
  shiftOut(dataPin, clockPin, MSBFIRST, encodedSsd[minuteTensDigit]);

  // hour
  shiftOut(dataPin, clockPin, MSBFIRST, encodedSsd[hourOnesDigit]);
  // do not show leading zeroes for hours
  if(hourTensDigit==0) {
    shiftOut(dataPin, clockPin, MSBFIRST, 0);
  } else {
    shiftOut(dataPin, clockPin, MSBFIRST, encodedSsd[hourTensDigit]);
  }
  //take the latch pin high so the LEDs will light up:
  digitalWrite(latchPin, HIGH);
}

void DisplayFourDigitScore(int homescore, int guestscore) {
  int homeOnesDigit = homescore % 10;
  int homeTensDigit = (homescore - homeOnesDigit) / 10;
  int guestOnesDigit = guestscore % 10;
  int guestTensDigit = (guestscore - guestOnesDigit) / 10;
  digitalWrite(latchPin, LOW);
  // shift out the bits:

  // visitors first
  shiftOut(dataPin, clockPin, MSBFIRST, encodedSsd[guestOnesDigit]);
  if(guestTensDigit==0) {
    shiftOut(dataPin, clockPin, MSBFIRST, 0);
  } else {
    shiftOut(dataPin, clockPin, MSBFIRST, encodedSsd[guestTensDigit]);
  }

  // home team
  shiftOut(dataPin, clockPin, MSBFIRST, encodedSsd[homeOnesDigit]);
  if(homeTensDigit==0) {
    shiftOut(dataPin, clockPin, MSBFIRST, 0);
  } else {
    shiftOut(dataPin, clockPin, MSBFIRST, encodedSsd[homeTensDigit]);
  }
  //take the latch pin high so the LEDs will light up:
  digitalWrite(latchPin, HIGH);
}

void Flash(int d) {
  TurnAllOn();
  delay(d);
  TurnAllOff();
  delay(d);
}

void Countup(int d) {
  LightSegments(encodedSsd[0]); //0
  delay(d);
  LightSegments(encodedSsd[1]);  //1
  delay(d);
  LightSegments(encodedSsd[2]);  //2
  delay(d);
  LightSegments(encodedSsd[3]); //3
  delay(d);
  LightSegments(encodedSsd[4]);  //4
  delay(d);
  LightSegments(encodedSsd[5]); //5
  delay(d);
  LightSegments(encodedSsd[6]); //6
  delay(d);
  LightSegments(encodedSsd[7]);  //7
  delay(d);
  LightSegments(encodedSsd[8]);  //8
  delay(d);
  LightSegments(encodedSsd[9]);  //9
  delay(d);
}

void TestAllCombos (int d) {
  int i = 0;
  for (i = 0; i <= 255; i++) {
    LightSegments(i);
    delay(d);
    if (i==255) 
    {
      i = 0;
    }
  }
}

void SegmentTest (int d) {
  while(1) {
    //flash all segments so we know we are starting
    LightSegments(255);  //8
    delay(100);
    LightSegments(0);  //8
    delay(100);
    LightSegments(255);  //8
    delay(100);
    LightSegments(0);  //8
    delay(100);
    LightSegments(255);  //8
    delay(100);
    LightSegments(0);  //8
    delay(100);

    // now flash each segment one at a time
    LightSegments(1);  //8
    delay(d);
    LightSegments(2);  //8
    delay(d);
    LightSegments(4);  //8
    delay(d);
    LightSegments(8);  //8
    delay(d);
    LightSegments(16);  //8
    delay(d);
    LightSegments(32);  //8
    delay(d);
    LightSegments(64);  //8
    delay(d);
    LightSegments(128);  //8
    delay(d);
  }
}


//Power On Self Test
void Post() {
  Spin(100);
  Spin(100);
  TurnAllOff();
  delay(500);
  Countup(200);
  Countup(200);
  Countup(200);
  TurnAllOff();
  delay(500);
  Flash(500);
  Flash(500);
  Flash(2000);
}

void Spin(int d) {
  LightSegments(0);
  delay(d);
  LightSegments(1);
  delay(d);
  LightSegments(2);
  delay(d);
  LightSegments(4);
  delay(d);
  LightSegments(8);
  delay(d);
  LightSegments(16);
  delay(d);
  LightSegments(32);
  delay(d);
  LightSegments(64);
  delay(d);
  LightSegments(128);
  delay(d);
}

void LightSegments(int a) {
    digitalWrite(latchPin, LOW);
    // shift out the bits:
    shiftOut(dataPin, clockPin, MSBFIRST, a);
    shiftOut(dataPin, clockPin, MSBFIRST, a);
    shiftOut(dataPin, clockPin, MSBFIRST, a);
    shiftOut(dataPin, clockPin, MSBFIRST, a);
    //take the latch pin high so the LEDs will light up:
    digitalWrite(latchPin, HIGH);
    // pause before next value:
}

void TurnAllOn() {
    digitalWrite(latchPin, LOW);
    // shift out the bits:
    shiftOut(dataPin, clockPin, MSBFIRST, 255);
    shiftOut(dataPin, clockPin, MSBFIRST, 255);
    shiftOut(dataPin, clockPin, MSBFIRST, 255);
    shiftOut(dataPin, clockPin, MSBFIRST, 255);
    //take the latch pin high so the LEDs will light up:
    digitalWrite(latchPin, HIGH);
    // pause before next value:
}

void TurnAllOff() {
    digitalWrite(latchPin, LOW);
    // shift out the bits:
    shiftOut(dataPin, clockPin, MSBFIRST, 0);
    shiftOut(dataPin, clockPin, MSBFIRST, 0);
    shiftOut(dataPin, clockPin, MSBFIRST, 0);
    shiftOut(dataPin, clockPin, MSBFIRST, 0);
    //take the latch pin high so the LEDs will light up:
    digitalWrite(latchPin, HIGH);
    // pause before next value:
}

void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte
dayOfMonth, byte month, byte year)
{
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month)); // set month
  Wire.write(decToBcd(year)); // set year (0 to 99)
  Wire.endTransmission();
}


void getTime(byte *second, byte *minute, byte *hour)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
}


void readDS3231time(byte *second,
byte *minute,
byte *hour,
byte *dayOfWeek,
byte *dayOfMonth,
byte *month,
byte *year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}


// nothing calls this, which is good because of all of the Seial calls.
void displayTime()
{
  Serial.begin(9600);
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month,
  &year);
  // send it to the serial monitor
  Serial.print(hour, DEC);
  // convert the byte variable to a decimal number when displayed
  Serial.print(":");
  if (minute<10)
  {
    Serial.print("0");
  }
  Serial.print(minute, DEC);
  Serial.print(":");
  if (second<10)
  {
    Serial.print("0");
  }
  Serial.print(second, DEC);
  Serial.print(" ");
  Serial.print(dayOfMonth, DEC);
  Serial.print("/");
  Serial.print(month, DEC);
  Serial.print("/");
  Serial.print(year, DEC);
  Serial.print(" Day of week: ");
  switch(dayOfWeek){
  case 1:
    Serial.println("Sunday");
    break;
  case 2:
    Serial.println("Monday");
    break;
  case 3:
    Serial.println("Tuesday");
    break;
  case 4:
    Serial.println("Wednesday");
    break;
  case 5:
    Serial.println("Thursday");
    break;
  case 6:
    Serial.println("Friday");
    break;
  case 7:
    Serial.println("Saturday");
    break;
  }
  Serial.end();
}

byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}
