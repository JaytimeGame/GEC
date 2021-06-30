/************************************************
  #include
*************************************************/
//GlOBAL
#include <Arduino.h>
#include <EEPROM.h>

//SCREEN
#include <Adafruit_I2CDevice.h>
#include <SPI.h>
#include <TouchScreen.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <gfxFont.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

//ATH20
#include <Wire.h>
#include <ATH20.h>

//Servos (Dampers)
#include <Servo.h>

/************************************************
  Definition
*************************************************/

//SCREEN
#if defined(__SAM3X8E__)
    #undef __FlashStringHelper::F(string_literal)
    #define F(string_literal) string_literal
#endif
#define YP A5  // must be an analog pin, use "An" notation!
#define XM A6  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin

#define TS_MINX 150
#define TS_MINY 120
#define TS_MAXX 920
#define TS_MAXY 940

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define GRAY_WINDOWS 0xB596

#define BOXSIZE_40 40
#define BOXSIZE_50 50

#define MINPRESSURE 10
#define MAXPRESSURE 1000
#define FONT &FreeSans9pt7b

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

int BGColor;

//ATH20
ATH20 ATH;
#define LOOP_DELAY 50
#define MAXTEMP_ADDRESS 0
#define MINTEMP_ADDRESS 1
#define MAXHUMI_ADDRESS 2
#define MINHUMI_ADDRESS 3
#define SETTINGNUMBER 5
#define tempArray  10
#define humiArray  10
float TemperatureArray[tempArray];
float TempAverage, Temperature;
float HumidityArray[humiArray];
float HumiAverage, Humidity;

float maxTemp, minTemp, maxHumi, minHumi;
float setMaxTemp, setMinTemp, setMaxHumi, setMinHumi;

//Global
#define FAN 12
String screenName = "Home";

String version = "1.1";

int showTempHumiCount;

//TouchScreen
#define POSITIONLOOPING 1
int positionX, positionY;
bool asBeenPress;
int demandAfterTouch = POSITIONLOOPING;

//Servos (Dampers)
#define CLOSE 153
#define OPEN 81
#define VENTPIN 40
#define ROOFPIN 41
#define DETACHWAIT 500
Servo VentDamper;
Servo RoofDamper;

//Error Detection
#define OPENPIN 14
#define CLOSEPIN 15
#define NOERRORLED 6
#define ERRORLED 7
#define ERRORTOPLED 5
#define ERRORVENTLED 4
#define BUZZER 10
#define ERRORLOOPINGCOUNT 500
int error[2];
int errorLoopCount = 0;
int errorOnDivider, errorOffDivider;
bool errorOnOff = false;

//Spraying
#define SPRAYPIN 16
int sprayTime = 50;
int spraying = 0;
bool notSprayed=1;
int waitSpray = 2;

//Virtual Data
#define TEMPSWITCH 2
#define HUMISWITCH 3
#define TPOTMAX 10
#define TEMPPOT A14
#define HUMIPOT A13
#define VIRTUALTEMP 9
#define VIRTUALHUMI 10

//bool EEPROM.write(VIRTUALHUMI,0);
//bool EEPROM.write(VIRTUALHUMI,0);
float virtualMinTemp = 15.0;
float virtualMaxTemp = 30.0;
float virtualMinHumi = 20;
float virtualMaxHumi = 80;

//Control
#define FANSTATE 5//0=Off 1=On
#define VENTSTATE 6//0=Closed 1=Open
#define ROOFSTATE 7//0=closed 1=Open
#define CONTROLSTATE 8 //0=Auto 1=Manual

/************************************************
  FUNCTIONS
*************************************************/

//Screen Utilities

/*
  writeText(int x, int y, int size, String text, int color)

  int x: X position of the top-left corner of the text area
  int y: Y position of the top-left corner of the text area
  int size: Size of the text (1 - 3)
  String text: text to write (i.e. "Hello")
  int color: Color of the text in Hex RGB565 (16 bits). Color need to be defined if using color name instead of Hexcode
*/
void writeText(int x, int y, int size, String text, int color) {
  tft.setFont(FONT);
  tft.setCursor(x,y);
  tft.setTextColor(color);
  tft.setTextSize(size);
  tft.print(text);
  return;
}

/*
  createBox(int x, int y, int size_X, int size_Y, int edge_Color, int edge_W, int box_Color )

  int x: X position of the top-left corner of the box (including the edge)
  int y: Y position of the top-left corner of the box (including the edge)
  int size_X: X size of the box (including the edge)
  int size_Y: Y size of the box (including the edge)
  int edge_Color: Color of the edge in Hex RGB565 (16 bits). Color need to be defined if using color name instead of Hexcode
  int edge_W: Withd of the edge
  int box_Color: Color of the box in Hex RGB565 (16 bits). Color need to be defined if using color name instead of Hexcode
*/
void createBox(int x, int y, int size_X, int size_Y, int edge_Color, int edge_W, int box_Color ) {
  tft.fillRect(x,y,size_X,size_Y,edge_Color);
  int rectSize_X = size_X - 2*edge_W;
  int rectSize_Y = size_Y - 2*edge_W;
  int rect_X = x + (edge_W);
  int rect_Y = y + (edge_W);
  tft.fillRect(rect_X,rect_Y,rectSize_X,rectSize_Y,box_Color);
}

/*
  displayTitle()

  Used to display the title of the Programe
*/
void displayTitle() {
  int addY = 15, addX = 25;
  createBox(0,0,480,46,WHITE,5,BLACK);
  writeText(42+addX,14+addY,1,"GREENHOUSE ENVIROMENT CONTOL",WHITE);
  return;
}

/*
  displayControlState()

  Used to display the Control State according to the value in the EEPROM for the Control State
*/
void displayControlState() {
  if (error[0]==0){
    int addY = 15, addX = 5;
    if (EEPROM.read(CONTROLSTATE)==0) {
      writeText(315+10+addX,54+addY,1,"Automatic",WHITE);
      writeText(332+10,79+addY,1,"Manual",GRAY_WINDOWS);
    }
    else if (EEPROM.read(CONTROLSTATE)==1) {
      writeText(315+10+addX,54+addY,1,"Automatic",GRAY_WINDOWS);
      writeText(332+10,79+addY,1,"Manual",WHITE);
    }
  }
}

/*
  displayRoofState()

  Used to display the Roof Dampers State according to the value in the EEPROM for the Roof Dampers
*/
void displayRoofState() {
  if (error[0]==0){
    int addY = 15, addX = 5,openLenght = 50, slashLenght = 15;
    writeText(288+10+addX,258+addY,1,"ROOF DAMPERS",WHITE);
    if (EEPROM.read(ROOFSTATE)==0) {
      writeText(298+20-addX,285+addY,1,"OPEN",GRAY_WINDOWS);
      writeText(298+20-addX+openLenght,285+addY,1," / ",WHITE);
      writeText(298+20-addX+openLenght+slashLenght,285+addY,1,"CLOSE",WHITE);
    }

    else if (EEPROM.read(ROOFSTATE)==1) {
      writeText(298+20-addX,285+addY,1,"OPEN",WHITE);
      writeText(298+20-addX+openLenght,285+addY,1," / ",WHITE);
      writeText(298+20-addX+openLenght+slashLenght,285+addY,1,"CLOSE",GRAY_WINDOWS);
    }
  }

}

/*
  displayVentState()

  Used to display the Vent Dampers State according to the value in the EEPROM for the Vent Dampers
*/
void displayVentState() {
  if (error[0]==0){
    int addY = 15, addX = 5,openLenght = 50, slashLenght = 15;
    writeText(330,189+addY,1,"AIR DUCT",WHITE);
    if (EEPROM.read(VENTSTATE)==0) {
      writeText(299+20-addX,216+addY,1,"OPEN",GRAY_WINDOWS);
      writeText(299+20-addX+openLenght,216+addY,1," / ",WHITE);
      writeText(299+20-addX+openLenght+slashLenght,216+addY,1,"CLOSE",WHITE);
    }

    else if (EEPROM.read(VENTSTATE)==1) {
      writeText(299+20-addX,216+addY,1,"OPEN",WHITE);
      writeText(299+20-addX+openLenght,216+addY,1," / ",WHITE);
      writeText(299+20-addX+openLenght+slashLenght,216+addY,1,"CLOSE",GRAY_WINDOWS);
    }
  }

}

/*
  displayFanState()

  Used to display the Fan State according to the value in the EEPROM for the Fans
*/
void displayFanState() {
  if (error[0]==0){
    int addY = 15, addX = 5,onLenght = 27, slashLenght = 15;
    writeText(312+addX,120+addY,1,"FAN STATUS",WHITE);
    if (EEPROM.read(FANSTATE)==0) {
      writeText(327+addX,147+addY,1,"ON", GRAY_WINDOWS);
      writeText(327+addX+onLenght,147+addY,1," / ", WHITE);
      writeText(327+addX+onLenght+slashLenght,147+addY,1,"OFF", WHITE);
    }

    else if (EEPROM.read(FANSTATE)==1) {
      writeText(327+addX,147+addY,1,"ON", WHITE);
      writeText(327+addX+onLenght,147+addY,1," / ", WHITE);
      writeText(327+addX+onLenght+slashLenght,147+addY,1,"OFF", GRAY_WINDOWS);
    }
  }

}

//Spraying

/*
  spray()

  This function is used to turn On and Off the Humidification of the Greenhouse.
  This fonction works in 8 Steps:

  Step 1: (Spraying)
    Turn On the Humidification / Adding 1 to spraying
  Step 2:
    Adding 1 to spraying
  Step 3:
    Repeat Step 2 until spraying is equals to sprayTime
  Step 4:
    Turn Off the Humidification / reset spraying to 0 / set notSprayed to 0
  Step 5: (Cooldown)
    Adding 1 to Spraying
  Step 6:
    Repeat Step 5 until spraying is equals to sprayTime*waitSpray
  Step 7:
    Reset spraying to 0 / set notSprayed to 1
  Step 8:
    Restart at Step 1
*/
void spray() {

  if (notSprayed==1){
    Serial.print("Spraying = ");Serial.print(spraying);Serial.print("/");Serial.println(sprayTime);
    if (spraying==0){
      digitalWrite(SPRAYPIN,HIGH);
      spraying++;
    }
    else if (spraying<sprayTime){
      spraying++;
    }
    else if (spraying==sprayTime){
      digitalWrite(SPRAYPIN,LOW);
      spraying=0;
      notSprayed=0;
    }
  }
  else if (notSprayed==0){
    Serial.print("Spray Waiting = ");Serial.print(spraying);Serial.print("/");Serial.println(sprayTime*waitSpray);
    if (spraying==sprayTime*waitSpray){
      notSprayed=1;
      spraying=0;
    }
    else if (spraying<sprayTime*waitSpray){
      spraying++;
    }
  }
  return;
}

/*
  resetSpray()

  This function is used to perform a Spraying Reset during The Humidification and Cooldown
*/
void resetSpray(){
  notSprayed=1;
  spraying=0;
  digitalWrite(SPRAYPIN,LOW);
  return;
}


//Error Screen

/*
  displayErrorButton()

  This function is used to display the "ERROR REPAIRED" Button on the Error Screen
*/
void displayErrorButton() {
  int sizex = 220;
  int sizey = 100;
  createBox(260,220,sizex,sizey,WHITE,5,BLACK);
  writeText(332+5,247+15,1,"ERROR",WHITE);
  writeText(318+5,272+15,1,"REPAIRED",WHITE);
  return;
}

/*
  displayErrorLogo()

  This function is used to display the error logo on the Error Screen
*/
void displayErrorLogo() {
  int y = 180, x = 83,l = 9, a=0;
  tft.drawFastHLine(x,y,l,RED);
  y++;//y=181
  a=a+2;//a=2
  x=x-2;//x=81
  for (int i=0;i<3;i++){
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
    a++;
    x--;
  }//y=184 x=78 a=5
  a--;//a=4
  x++;//x=79
  tft.drawFastHLine(x,y,l+(2*a),RED);
  y++;//y=185
  a++;//a=5
  x--;//x=78
  for (int i=0;i<2;i++){
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
    a++;
    x--;
  }//y=187 x=76 a=7
  a--;//a=6
  x++;//x=77
  for (int i=0;i<3;i++){
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
    a++;
    x--;
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
  }//y=193 x=74 a=9
  a++;//a=10
  x--;//x=73
  tft.drawFastHLine(x,y,l+(2*a),RED);
  y++;//y=194
  for (int i=0;i<4;i++){
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
    a++;
    x--;
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
  }//y=202 x=69 a=14
  a++;//a=15
  x--;//x=68
  tft.drawFastHLine(x,y,l+(2*a),RED);
  y++;//y=203
  for (int i=0;i<2;i++){
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
    a++;
    x--;
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
  }//y=207 x=66 a=17
  a++;//a=18
  x--;//x=65
  tft.drawFastHLine(x,y,l+(2*a),RED);
  y++;//y=208
  for (int i=0;i<2;i++){
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
    a++;
    x--;
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
  }//y=212 x=63 a=20
  a++;//a=21
  x--;//x=62
  tft.drawFastHLine(x,y,l+(2*a),RED);
  y++;//y=213
  for (int i=0;i<3;i++){
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
    a++;
    x--;
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
  }//y=219 x=59 a=24
  a++;//a=25
  x--;//x=58
  tft.drawFastHLine(x,y,l+(2*a),RED);
  y++;//y=220
  for (int i=0;i<3;i++){
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
    a++;
    x--;
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
  }//y=226 x=55 a=28
  a++;//a=29
  x--;//x=54
  tft.drawFastHLine(x,y,l+(2*a),RED);
  y++;//y=227
  for (int i=0;i<3;i++){
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
    a++;
    x--;
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
  }//y=233 x=51 a=32
  a++;//a=33
  x--;//x=50
  tft.drawFastHLine(x,y,l+(2*a),RED);
  y++;//y=234
  for (int i=0;i<3;i++){
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
    a++;
    x--;
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
  }//y=240 x=47 a=36
  a++;//a=37
  x--;//x=46
  tft.drawFastHLine(x,y,l+(2*a),RED);
  y++;//y=241
  for (int i=0;i<3;i++){
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
    a++;
    x--;
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
  }//y=247 x=43 a=40
  a++;//a=41
  x--;//x=42
  tft.drawFastHLine(x,y,l+(2*a),RED);
  y++;//y=248
  for (int i=0;i<3;i++){
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
    a++;
    x--;
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
  }//y=254 x=39 a=44
  a++;//a=45
  x--;//x=38
  tft.drawFastHLine(x,y,l+(2*a),RED);
  y++;//y=255
  for (int i=0;i<3;i++){
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
    a++;
    x--;
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
  }//y=261 x=35 a=48
  a++;//a=49
  x--;//x=34
  tft.drawFastHLine(x,y,l+(2*a),RED);
  y++;//y=262
  for (int i=0;i<2;i++){
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
    a++;
    x--;
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
  }//y=266 x=32 a=51
  a++;//a=52
  x--;//x=31
  tft.drawFastHLine(x,y,l+(2*a),RED);
  y++;//y=267
  for (int i=0;i<3;i++){
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
    a++;
    x--;
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
  }//y=273 x=28 a=55
  a++;//a=56
  x--;//x=27
  tft.drawFastHLine(x,y,l+(2*a),RED);
  y++;//y=274
  for (int i=0;i<3;i++){
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
    a++;
    x--;
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
  }//y=280 x=24 a=59
  a++;//a=60
  x--;//x=23
  tft.drawFastHLine(x,y,l+(2*a),RED);
  y++;//y=281
  for (int i=0;i<3;i++){
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
    a++;
    x--;
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
  }//y=287 x=20 a=63
  for (int i=0;i<7;i++){
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
  }//y=294 x=20 a=63
  a--;//a=62
  x++;//x=21
  for (int i=0;i<2;i++){
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
  }//y=296 x=21 a=62
  a--;//a=61
  x++;//x=22
  for (int i=0;i<3;i++){
    tft.drawFastHLine(x,y,l+(2*a),RED);
    y++;
    x++;
    a--;
  }//y=299 x=25 a=58
  a--;//a=57
  x++;//x=26
  tft.drawFastHLine(x,y,l+(2*a),RED);


  x=85;
  y=203;
  a=0;
  l=5;
  tft.drawFastHLine(x,y,l+(2*a),BLACK);
  y++;//204
  x=x-2;//x=83
  a=a+2;//a=2
  for (int i = 0;i<3;i++) {
    tft.drawFastHLine(x,y,l+(2*a),BLACK);
    y++;
    x--;
    a++;
  }//y=207 x=80 a=5
  x++;//x=81
  a--;//a=4
  tft.drawFastHLine(x,y,l+(2*a),BLACK);
  x--;//x=80
  a++;//a=5
  y++;//y=208
  tft.fillRect(x,y,l+(2*a),273-y+1,BLACK);
  y=274;
  x++;
  a--;
  tft.drawFastHLine(x,y,l+(2*a),BLACK);

  x=87;
  y=287;
  int d=15;
  tft.fillCircle(x,y,d/2,BLACK);




}

/*
  Error_Screen(String whatError)

  String whatError: The name (or description) of the Error

  This function is used to display the Error Screen with the right Error message
*/
void Error_Screen(String whatError) {
  BGColor = BLACK;
  tft.fillScreen(BGColor);
  displayTitle();
  createBox(0,41,480,46,WHITE,5,BLACK);
  displayErrorButton();
  displayErrorLogo();
  writeText(202+10,54+15,1,"ERROR",RED);
  writeText(107+15,116+15,1,whatError,WHITE);
  screenName = "Error";
  return;
}

//Servos (Dampers)

/*
  attachVent()

  This function is used to attach VentDamper to the right pin and power the Vent Servo
*/
void attachVent(){
  VentDamper.attach(VENTPIN, 500, 2500);
  return;
}

/*
  detachVent()

  This function is used to detach VentDamper from its pin and remove the power to the Vent Servo
*/
void detachVent(){
  VentDamper.detach();
  return;
}

/*
  openVentDamper()

  This function is used to open the Vent Dampers
  This function works in 5 Steps:

  Step 1:
    Righ to the Vent State EEPROM 1, to represent that the vent is Open
  Step 2:
    Verify the State of the Close and Open switchs
  Step 3: (Correct: Damper in the Correct Position)
    attachVent() / Move the Servo to the OPEN Position / wait a set amount of time / detachVent()
  Step 3: (Error: Damper In the Wrong Position / In between the two switchs / In Two Position ant the same time)
    Set the error array to the right ID / Error_Screen() with the Error name or description
    error[0]: Location of the Error
    error[1]: Error Type
  Step 4: (If There as been no Error in Step 3)
    Verify the State of the Close and Open switchs
  Step 5: (Correct: Damper in the Correct Position)
    Set error[0] to 0
  Step 5: (Error: Damper In the Wrong Position / In between the two switchs / In Two Position ant the same time)
    Set the error array to the right ID / Error_Screen() with the Error name or description
    error[0]: Location of the Error
    error[1]: Error Type
*/
void openVentDamper() {
  EEPROM.write(VENTSTATE,1);
  if (digitalRead(CLOSEPIN)==true && digitalRead(OPENPIN) == false) {
    attachVent();
    VentDamper.write(OPEN);
    delay(DETACHWAIT);
    detachVent();
    error[0] = 0;
  }
  else if (digitalRead(CLOSEPIN) == false && digitalRead(OPENPIN) == false) {
    error[0] = 1;
    error[1] = 1;
    Error_Screen("Vent Stuck / Broken Vent Switch(s)");
  }
  else if (digitalRead(CLOSEPIN) == true && digitalRead(OPENPIN) == true) {
    error[0] = 1;
    error[1] = 2;
    Error_Screen("Broken Vent Switch(s)");
  }
  else if (digitalRead(CLOSEPIN) == false && digitalRead(OPENPIN) == true) {
    error[0] = 1;
    error[1] = 3;
    Error_Screen("Vent Already Open");
  }

  if (error[0] == 0) {
    if (digitalRead(CLOSEPIN) == false && digitalRead(OPENPIN) == true) {
      error[0] = 0;
    }
    else if (digitalRead(CLOSEPIN) == false && digitalRead(OPENPIN) == false) {
      error[0] = 1;
      error[1] = 1;
      Error_Screen("Vent Stuck / Broken Vent Switch(s)");
    }
    else if (digitalRead(CLOSEPIN) == true && digitalRead(OPENPIN) == true) {
      error[0] = 1;
      error[1] = 2;
      Error_Screen("Broken Vent Switch(s)");
    }
    else if (digitalRead(CLOSEPIN) == true && digitalRead(OPENPIN) == false) {
      error[0] = 1;
      error[1] = 4;
      Error_Screen("Vent Did Not Open");
    }
  }
  return;
}

/*
  openVentDamperNOINT()

  This function is used to Open the Vent Dampers Without the EDS
*/
void openVentDamperNOINT() {
  EEPROM.write(VENTSTATE,1);
  attachVent();
  VentDamper.write(OPEN);
  delay(DETACHWAIT);
  detachVent();
  return;
}

/*
  closeVentDamper()

  This function is used to close the Vent Dampers
  This function works the same ways as the openVentDamper() function but with the position CLOSE instead of OPEN
  and different switchs combination.
*/
void closeVentDamper() {
  EEPROM.write(VENTSTATE,0);
  if (digitalRead(CLOSEPIN)==false && digitalRead(OPENPIN) == true) {
    attachVent();
    VentDamper.write(CLOSE);
    delay(DETACHWAIT);
    detachVent();
    error[0] = 0;
  }
  else if (digitalRead(CLOSEPIN) == true && digitalRead(OPENPIN) == true) {
    error[0] = 1;
    error[1] = 1;
    Error_Screen("Vent Stuck / Broken Vent Switch(s)");
  }
  else if (digitalRead(CLOSEPIN) == false && digitalRead(OPENPIN) == false) {
    error[0] = 1;
    error[1] = 2;
    Error_Screen("Broken Vent Switch(s)");
  }
  else if (digitalRead(CLOSEPIN) == true && digitalRead(OPENPIN) == false) {
    error[0] = 1;
    error[1] = 3;
    Error_Screen("Vent Already Close");
  }

  if (error[0] == 0) {
    if (digitalRead(CLOSEPIN) == true && digitalRead(OPENPIN) == false) {
      error[0] = 0;
    }
    else if (digitalRead(CLOSEPIN) == true && digitalRead(OPENPIN) == true) {
      error[0] = 1;
      error[1] = 1;
      Error_Screen("Vent Stuck / Broken Vent Switch(s)");
    }
    else if (digitalRead(CLOSEPIN) == false && digitalRead(OPENPIN) == false) {
      error[0] = 1;
      error[1] = 2;
      Error_Screen("Broken Vent Switch(s)");
    }
    else if (digitalRead(CLOSEPIN) == false && digitalRead(OPENPIN) == true) {
      error[0] = 1;
      error[1] = 4;
      Error_Screen("Vent Did Not Close");
    }
  }


  attachVent();
  VentDamper.write(CLOSE);
  delay(DETACHWAIT);
  detachVent();
  return;
}

/*
  closeVentDamperNOINT()

  This function is used to Close the Vent Dampers Without the EDS
*/
void closeVentDamperNOINT() {
  EEPROM.write(VENTSTATE,0);
  attachVent();
  VentDamper.write(CLOSE);
  delay(DETACHWAIT);
  detachVent();
  return;
}

/*
  attachRoof()

  This function is used to attach RoofDamper to the right pin and power the Roof Servo
*/
void attachRoof(){
  RoofDamper.attach(ROOFPIN, 500, 2500);
  return;
}

/*
  detachVent()

  This function is used to detach RoofDamper from its pin and remove the power to the Roof Servo
*/
void detachRoof(){
  RoofDamper.detach();
  return;
}

/*
  openRoofDamper()

  This function is used to Open the Roof Dampers Without the EDS (For Now)
*/
void openRoofDamper() {
  EEPROM.write(ROOFSTATE,1);
  attachRoof();
  RoofDamper.write(OPEN);
  delay(DETACHWAIT);
  detachRoof();
  return;
}

/*
  closeRoofDamper()

  This function is used to Close the Roof Dampers Without the EDS (For Now)
*/
void closeRoofDamper() {
  EEPROM.write(ROOFSTATE,0);
  attachRoof();
  RoofDamper.write(CLOSE);
  delay(DETACHWAIT);
  detachRoof();
  return;
}

/*
  setupVent()

  This function is used to make sure that the Vent Dampers are in the right position during the setup loop
*/
void setupVent() {
  if (EEPROM.read(VENTSTATE)==0) {
    closeVentDamperNOINT();
    Serial.println("Vent OPEN");
    return;
  }
  else if (EEPROM.read(VENTSTATE)==1) {
    openVentDamperNOINT();
    Serial.println("Vent Closed");
    return;
  }
}

/*
  setupRoof()

  This function is used to make sure that the Roof Dampers are in the right position during the setup loop
*/
void setupRoof() {
  if (EEPROM.read(ROOFSTATE)==0) {
    closeRoofDamper();
    Serial.println("Roof OPEN");
    return;
  }
  else if (EEPROM.read(ROOFSTATE)==1) {
    openRoofDamper();
    Serial.println("Roof CLOSED");
    return;
  }
}

/*
  setupFan()

  This function is used to make sure that the Fans are in the right state during the setup loop
*/
void setupFan() {
  if (EEPROM.read(FANSTATE)==0) {
    digitalWrite(FAN,LOW);
    Serial.println("Fan OFF");
    return;
  }
  else if (EEPROM.read(FANSTATE)==1) {
    digitalWrite(FAN,HIGH);
    Serial.println("Fan ON");
    return;
  }
}

/*
  stateSetup()

  This function is used to call the three previus fonctions during the setup loop
*/
void stateSetup() {
  VentDamper.attach(VENTPIN, 500, 2500);
  setupVent();
  RoofDamper.attach(ROOFPIN, 500, 2500);
  setupRoof();
  setupFan();
}
// Global

/*
  functionVersion()

  This function is used to provide the main program with the Version of the Function file
*/
String functionVersion() {
  return version;
}

//Global States

/*
  switchControlState()

  This function is used to switch the value in the Control State EEPROM
*/
void switchControlState() {
  if (EEPROM.read(CONTROLSTATE)==0) {
    EEPROM.write(CONTROLSTATE,1);
    Serial.println("Manual Control");
  }
  else if (EEPROM.read(CONTROLSTATE)==1) {
    EEPROM.write(CONTROLSTATE,0);
    Serial.println("Automatic Control");
  }
  return;
}

/*
  switchFan()

  This function is used to switch the value in the Fan State EEPROM and turn On or Off the fans accordingly
*/
void switchFan() {
  if (EEPROM.read(FANSTATE)==0){
    EEPROM.write(FANSTATE,1);
    digitalWrite(FAN,HIGH);
    Serial.println("Fan ON");
    return;
  }
  else if (EEPROM.read(FANSTATE)==1) {
    EEPROM.write(FANSTATE,0);
    digitalWrite(FAN,LOW);
    Serial.println("Fan OFF");
    return;
  }
}

/*
  switchVent()

  This function is used to switch the Vent Dampers
*/
void switchVent() {
  if (EEPROM.read(VENTSTATE)==0) {
    openVentDamper();
    Serial.println("Vent OPEN");
    return;
  }
  else if (EEPROM.read(VENTSTATE)==1) {
    closeVentDamper();
    Serial.println("Vent Closed");
    return;
  }
}

/*
  switchVentNOINT()

  This function is used to switch the Vent Dampers Without the EDS
*/
void switchVentNOINT() {
  if (EEPROM.read(VENTSTATE)==0) {
    openVentDamperNOINT();
    Serial.println("Vent OPEN");
    return;
  }
  else if (EEPROM.read(VENTSTATE)==1) {
    closeVentDamperNOINT();
    Serial.println("Vent Closed");
    return;
  }
}

/*
  switchRoof()

  This function is used to switch the Roof Dampers Without the EDS (For Now)
*/
void switchRoof() {
  if (EEPROM.read(ROOFSTATE)==0) {
    openRoofDamper();
    Serial.println("Roof OPEN");
    return;
  }
  else if (EEPROM.read(ROOFSTATE)==1) {
    closeRoofDamper();
    Serial.println("Roof CLOSED");
    return;
  }
}

//Error Detection System (EDS)

/*
  errorLoop()

  This fonction is being called when the location number of the error array (error[0]) is not equals to 0
  This function is used to give a visual and audial representation of the Error.
*/
void errorLoop() {
  int lightLED;
  digitalWrite(NOERRORLED,LOW);
  digitalWrite(ERRORLED,HIGH);
  if (error[0] >= 1 && error[0] < 2) {
    lightLED = ERRORVENTLED;
  }
  else if (error[0] >= 2 && error[0] < 3) {
    lightLED = ERRORTOPLED;
  }

  errorOnDivider = error[0];
  errorOffDivider = error[1];

  if (errorOnOff == false) {
    if (errorLoopCount == 0) {
      digitalWrite(lightLED,HIGH);
      digitalWrite(BUZZER,HIGH);
      errorOnOff = true;
      errorLoopCount++;
    }
    else if (errorLoopCount < ERRORLOOPINGCOUNT/errorOffDivider) {
      errorLoopCount++;
    }
    else if (errorLoopCount == ERRORLOOPINGCOUNT/errorOffDivider) {
      digitalWrite(lightLED,HIGH);
      digitalWrite(BUZZER,HIGH);
      errorOnOff = true;
      errorLoopCount = 0;
    }
  }
  else if (errorOnOff == true) {
    if (errorLoopCount < ERRORLOOPINGCOUNT/errorOnDivider) {
      errorLoopCount++;
    }
    else if (errorLoopCount == ERRORLOOPINGCOUNT/errorOnDivider) {
      digitalWrite(lightLED,LOW);
      digitalWrite(BUZZER,LOW);
      errorOnOff = false;
      errorLoopCount = 1;
    }
  }

}

/*
  removeError()

  This function is used to reset the error array to 0 0, errorLoopCount to 0, errorOnOff to false,
  turn On the NOERRORLED, and turn Off the all the Error LEDs and the Buzzer.
*/
void removeError() {
  error[0] = 0;
  error[1] = 0;
  errorLoopCount=0;
  errorOnOff=false;
  digitalWrite(NOERRORLED, HIGH);
  digitalWrite(ERRORLED, LOW);
  digitalWrite(ERRORTOPLED, LOW);
  digitalWrite(ERRORVENTLED, LOW);
  digitalWrite(BUZZER, LOW);
  return;
}

//ATH20

/*
  FToC(int fahr)

  int fahr: temperature in FAHRENHEIT

  This function is used to convert the temperature from CELSIUS to FAHRENHEIT
*/
float FToC(int fahr){
  float cels=fahr-32;
  cels= cels*5;
  cels=cels/9;
  return cels;
}

/*
  CToF(int cels)

  int cels: temperature in CELSIUS

  This function is used to convert the temperature from FAHRENHEIT to CELSIUS
*/
float CToF(int cels){
  float fahr=cels*9;
  fahr=fahr/5;
  fahr=fahr+32;
  return fahr;
}

/*
  getVirtualTemp()

  This function is used to get the temperature from a potentiometer
*/
float getVirtualTemp() {
  float potValue;
  float temp;
  potValue = analogRead(TEMPPOT);
  temp = map(potValue,0,1023,virtualMinTemp,virtualMaxTemp);
  return temp;
}

/*
  getVirtualHumi()

  This function is used to get the humidity from a potentiometer
*/
float getVirtualHumi() {
  float potValue;
  float humi;
  potValue = analogRead(HUMIPOT);
  humi = map(potValue,0,1023, virtualMinHumi,virtualMaxHumi);
  return humi;
}

/*
  getTempHumi(String what)

  String what: Temp or Humi

  This function is used to get the humidity or the temperature.
  It will fist get it from the Sensor and then get the virtual one if the virtual data is on
*/
float getTempHumi(String what) {
  float temp, humi;
  int ret;
  bool loop=1;
  while(loop==1){
    ret = ATH.getSensor(&humi,&temp);
    if (ret) {
      loop=0;
    }
    else {
      Serial.println("Getting Data from Sensor");
    }
  }
  humi = humi*100;
  if (EEPROM.read(VIRTUALHUMI)==1) {
    humi=getVirtualHumi();
  }
  if (EEPROM.read(VIRTUALTEMP)==1) {
    temp=getVirtualTemp();
  }

  if (what=="humi"||what=="Humi"||what=="HUMI") {
    return humi;
  }
  else if (what=="temp"||what=="Temp"||what=="TEMP") {
    return temp;
  }
}

/*
  refreshTempAverage()

  This function is used to refresh the average of the temperature
*/
void refreshTempAverage() {
  if (showTempHumiCount < tempArray) {
    TemperatureArray[showTempHumiCount] = Temperature;
  }
  if (showTempHumiCount == tempArray) {
    TempAverage=0;
    for (int i = 0 ; i<tempArray ; i++) {
      TempAverage = TempAverage + TemperatureArray[i];
    }
    TempAverage = TempAverage/tempArray;
  }
}

/*
  refreshTemperature()

  This function is used to refresh the temperature
*/
void refreshTemperature() {
  Temperature = getTempHumi("Temp");
}

/*
  refreshHumiAverage()

  This function is used to refresh the average of the humidity
*/
void refreshHumiAverage() {
  if (showTempHumiCount < humiArray) {
    HumidityArray[showTempHumiCount] = Humidity;
  }
  else if (showTempHumiCount == humiArray) {
    HumiAverage=0;
    for (int i = 0 ; i<humiArray ; i++) {
      HumiAverage = HumiAverage + HumidityArray[i];
    }
    HumiAverage = HumiAverage/humiArray;
  }
}

/*
  refreshHumidity()

  This function is used to refresh the humidity
*/
void refreshHumidity() {
  Humidity = getTempHumi("Humi");
}

/*
  checkTemp()

  This function is used to verify if the temperature is between the accepteble values.
  If the temperature is higher then the maximum it will open the dampers and turn On the Fans
  If the Temperature is in between the maximum and minimum it will make sure that the dampers are close and the fan is OFF
*/
void checkTemp() {
  Serial.print("Temperature = ");Serial.println(Temperature);
  Serial.print("Temperature Average = ");Serial.println(TempAverage);
  minTemp = EEPROM.read(MINTEMP_ADDRESS);
  maxTemp = EEPROM.read(MAXTEMP_ADDRESS);
  float TempAverageF=CToF(TempAverage);
  if (TempAverageF >= minTemp && TempAverageF <= maxTemp){\
    if (EEPROM.read(VENTSTATE)==1){
      switchVent();
      displayVentState();
    }
    if (EEPROM.read(ROOFSTATE)==1){
      switchRoof();
      displayRoofState();
    }
    if (EEPROM.read(FANSTATE)==1){
      switchFan();
      displayFanState();
    }
  }
  else if (TempAverageF > maxTemp) {
    if (EEPROM.read(VENTSTATE)==0){
      switchVent();
      displayVentState();
    }
    if (EEPROM.read(ROOFSTATE)==0){
      switchRoof();
      displayRoofState();
    }
    if (EEPROM.read(FANSTATE)==0){
      switchFan();
      displayFanState();
    }
  }
  else if (TempAverageF < minTemp) {
    //Nothing for now
  }
  return;
}

/*
  checkHumi()

  This function is used to verify if the Humidity is between the accepteble values.
  If the humidity is lower then the minimum call the spray() function.
  If the humidity is in between the maximum and minimum it will call the resetSpray() function.
*/
void checkHumi() {
  Serial.print("Humidity = ");Serial.println(Humidity);
  Serial.print("Humidity Average = ");Serial.println(HumiAverage);
  minHumi = EEPROM.read(MINHUMI_ADDRESS);
  maxHumi = EEPROM.read(MAXHUMI_ADDRESS);
  if (HumiAverage >= minHumi && HumiAverage <= maxHumi){
    resetSpray();
    //It's good so Nothing
  }
  else if (HumiAverage > maxHumi) {
    //Nothing for now
  }
  else if (HumiAverage < minHumi) {
    spray();
  }
  return;
}

/*
  setup_ATH20()

  This function is used to Setup and Start the ATH20 (temperature and humidity Sensor)
*/
void setup_ATH20() {

    Serial.println("ATH20 Setup");
    ATH.begin();

    minTemp = EEPROM.read(MINTEMP_ADDRESS);
    maxTemp = EEPROM.read(MAXTEMP_ADDRESS);
    minHumi = EEPROM.read(MINHUMI_ADDRESS);
    maxHumi = EEPROM.read(MAXHUMI_ADDRESS);

    refreshTemperature();
    refreshHumidity();
    TempAverage=Temperature;
    HumiAverage=Humidity;
    return;
}

//SCREEN

/*
  Not Used
*/
void displaySetupButton() {

  int x = 15;
  int y = 56;
  int h = 35;
  int w = 35;
  tft.fillRect((2*x),y,5,h,WHITE);
  tft.fillRect((2*x)-1,y+1,7,h-2,WHITE);
  tft.fillRect((2*x)-3,y+4,11,h-8,WHITE);

  tft.fillRect(x,(y+x),w,5,WHITE);
  tft.fillRect(x+1,(y+x)-1,w-2,7,WHITE);
  tft.fillRect(x+4,(y+x)-3,w-8,11,WHITE);

  int pixel1_x =  x+6;
  int pixel1_y = y+2;
  int row = 0;
  int plusMinus = -1;
  for(int i= plusMinus*0; i<2; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row++;
  for(int i= plusMinus*1; i<3; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row++;
  for(int i= plusMinus*2; i<4; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row++;
  for(int i= plusMinus*3; i<7; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row++;
  for(int i= plusMinus*3; i<7; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row++;
  for(int i= plusMinus*2; i<7; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row++;
  for(int i= plusMinus*1; i<7; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row++;
  for(int i= plusMinus*0; i<7; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row++;
  for(int i= plusMinus*0; i<7; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row++;
  for(int i= plusMinus*0; i<7; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }

  pixel1_x = x+w-7;
  tft.drawPixel(pixel1_x,pixel1_y,RED);
  pixel1_y = y+2;
  row = 0;
  plusMinus = -1;
  for(int i= plusMinus*0; i<2; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row++;
  for(int i= plusMinus*1; i<3; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row++;
  for(int i= plusMinus*2; i<4; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row++;
  for(int i= plusMinus*5; i<5; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row++;
  for(int i= plusMinus*5; i<5; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row++;
  for(int i= plusMinus*5; i<4; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row++;
  for(int i= plusMinus*5; i<3; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row++;
  for(int i= plusMinus*5; i<2; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row++;
  for(int i= plusMinus*5; i<2; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row++;
  for(int i= plusMinus*5; i<2; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }

  pixel1_x =  x+6;
  pixel1_y = y+h-3;
  row = 0;
  plusMinus = -1;
  for(int i= plusMinus*0; i<2; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row--;
  for(int i= plusMinus*1; i<3; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row--;
  for(int i= plusMinus*2; i<4; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row--;
  for(int i= plusMinus*3; i<7; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row--;
  for(int i= plusMinus*3; i<7; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row--;
  for(int i= plusMinus*2; i<7; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row--;
  for(int i= plusMinus*1; i<7; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row--;
  for(int i= plusMinus*0; i<7; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row--;
  for(int i= plusMinus*0; i<7; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row--;
  for(int i= plusMinus*0; i<7; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }

  pixel1_x = x+w-7;
  tft.drawPixel(pixel1_x,pixel1_y,RED);
  pixel1_y = y+h-3;
  row = 0;
  plusMinus = -1;
  for(int i= plusMinus*0; i<2; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row--;
  for(int i= plusMinus*1; i<3; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row--;
  for(int i= plusMinus*2; i<4; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row--;
  for(int i= plusMinus*5; i<5; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row--;
  for(int i= plusMinus*5; i<5; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row--;
  for(int i= plusMinus*5; i<4; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row--;
  for(int i= plusMinus*5; i<3; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row--;
  for(int i= plusMinus*5; i<2; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row--;
  for(int i= plusMinus*5; i<2; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }
  row--;
  for(int i= plusMinus*5; i<2; i++) {
    tft.drawPixel(pixel1_x+i,pixel1_y+row,WHITE);
  }



  /*int size = 100;
  createBox(0, 0, size, size, GRAY_WINDOWS, 10, BLACK);
  writeText(20,45, 1, "Setup", BLACK);
  return;*/
}

/*
  displayTempHumi()

  This function is used to display the Temperature and the Humidity
  This function also refresh the Temperature and Humidity (Current and Average)
*/
void displayTempHumi() {
  String TempStringC, TempStringF, HumiString;
  float tempF;

  refreshTemperature();
  refreshTempAverage();
  refreshHumidity();
  refreshHumiAverage();
  Serial.println(showTempHumiCount);
  if (showTempHumiCount == 10) {
    Serial.println("Showing Temperature & Humidity");
    TempStringC = String(Temperature);
    tempF=CToF(Temperature);
    TempStringF = String(tempF);
    HumiString = String(Humidity);
    tft.fillRect(124,135,60,20,BLACK);
    tft.fillRect(123,205,60,20,BLACK);
    tft.fillRect(124,286,60,20,BLACK);
    writeText(124,135+15,1,TempStringC, WHITE);
    writeText(125,205+15,1,TempStringF, WHITE);
    writeText(126,286+15,1, HumiString, WHITE);
    showTempHumiCount = 0;
  }
  else {
    showTempHumiCount++;
  }
  return;
}

//Home Screen

/*
  displaySetTempButton()

  This function is used to display the SetTemp Button on the Home Screen
*/
void displaySetTempButton() {
  createBox(0,46,65,69,WHITE,5,BLACK);
  int y=55;
  tft.drawFastHLine(23,y,5,WHITE);
  y++;
  tft.drawFastHLine(21,y,9,WHITE);
  y++;
  tft.drawFastHLine(20,y,4,WHITE);
  tft.drawFastHLine(27,y,4,WHITE);
  y++;
  tft.drawFastHLine(20,y,3,WHITE);
  tft.drawFastHLine(28,y,3,WHITE);
  y++;
  tft.fillRect(19,y,3,32,WHITE);
  tft.fillRect(29,y,3,32,WHITE);
  y=65;
  tft.drawPixel(25,y,WHITE);
  for (int i = 0; i<3; i++){
    if (i == 0){
      y=65;
    }
    else if (i == 1){
      y=75;
    }
    else if (i == 2){
      y=85;
    }
    tft.drawFastHLine(35,y,12,WHITE);
    y++;
    tft.fillRect(34,y,(47-34+1),3,WHITE);
    y=y+3;
    tft.drawFastHLine(35,y,12,WHITE);
  }
  y=66;
  tft.fillRect(24,y,3,28,WHITE);
  y=91;
  tft.drawFastHLine(18,y,4,WHITE);
  tft.drawFastHLine(29,y,4,WHITE);
  y++;
  tft.fillRect(18,y,3,2,WHITE);
  tft.fillRect(30,y,3,2,WHITE);
  y=94;
  tft.fillRect(17,y,3,5,WHITE);
  tft.drawFastHLine(23,y,5,WHITE);
  tft.fillRect(31,y,3,5,WHITE);
  y++;
  tft.fillRect(22,y,7,3,WHITE);
  y=98;
  tft.drawFastHLine(23,y,5,WHITE);
  y++;
  tft.drawFastHLine(18,y,3,WHITE);
  tft.drawFastHLine(24,y,3,WHITE);
  tft.drawFastHLine(30,y,3,WHITE);
  y++;
  int j=0;
  int x1=18,x2=29;
  do {
    tft.drawFastHLine((x1+j),(y+j),4,WHITE);
    tft.drawFastHLine((x2-j),(y+j),4,WHITE);
    if (j >= 2) {
      tft.drawFastHLine((x1+j+4),(y+j),(x2-j-(x1+j+4)),WHITE);
    }
    j++;
  } while(j<4);
  y=y+j;
  tft.drawFastHLine(23,y,5,WHITE);
  return;
}

/*
  displaySetHumiButton()

  This function is used to display the SetHumi Button on the Home Screen
*/
void displaySetHumiButton() {
  createBox(0,110,65,69,WHITE,5,BLACK);
  int y=123;
  for(int i = 0; i<2;i++){
    tft.drawPixel(32,(y),WHITE);
    y++;
  }//y=125
  tft.fillRect(31,y,3,2,WHITE);
  y=y+2;//127
  tft.fillRect(30,y,5,2,WHITE);
  y=y+2;//129
  int x1=29,x2=33;
  for(int i = 0; i<3;i++){
    tft.fillRect(x1,y,3,2,WHITE);
    tft.fillRect(x2,y,3,2,WHITE);
    x1--;
    x2++;
    y=y+2;
  }//y=135  x1=26 x2=36
  for(int i = 0; i<4;i++){
    tft.fillRect(x1,y,3,1,WHITE);
    tft.fillRect(x2,y,3,1,WHITE);
    x1--;
    x2++;
    y++;
  }//y=139  x1=22 x2=40
  tft.fillRect(x1,y,3,2,WHITE);
  tft.fillRect(x2,y,3,2,WHITE);
  y=y+2;//141
  x1--;//21
  x2++;//41
  tft.fillRect(x1,y,3,1,WHITE);
  tft.fillRect(x2,y,3,1,WHITE);
  y++;//142
  x1--;//20
  x2++;//42
  tft.fillRect(x1,y,3,2,WHITE);
  tft.fillRect(x2,y,3,2,WHITE);
  y=y+2;//144
  x1--;//19
  x2++;//43
  tft.fillRect(x1,y,3,3,WHITE);
  tft.fillRect(x2,y,3,3,WHITE);
  y=y+3;//147
  x1--;//18
  x2++;//44
  tft.fillRect(x1,y,3,7,WHITE);
  tft.fillRect(x2,y,3,7,WHITE);
  y=y+7;//154
  x1++;//19
  x2--;//43
  tft.fillRect(x1,y,3,2,WHITE);
  tft.fillRect(x2,y,3,2,WHITE);
  y=y+2;//156
  x2--;//42
  tft.drawFastHLine(x1,y,4,WHITE);
  tft.drawFastHLine(x2,y,4,WHITE);
  x1++;//20
  y++;//157
  tft.drawFastHLine(x1,y,3,WHITE);
  tft.drawFastHLine(x2,y,3,WHITE);
  x2--;//41
  y++;//158
  for(int i = 0; i<2;i++){
    tft.drawFastHLine(x1,y,4,WHITE);
    tft.drawFastHLine(x2,y,4,WHITE);
    x1++;
    x2--;
    y++;
  }//y=160  x1=22 x2=39
  x2--;//38
  for(int i = 0; i<2;i++){
    tft.drawFastHLine(x1,y,5,WHITE);
    tft.drawFastHLine(x2,y,5,WHITE);
    x1++;
    x2--;
    y++;
  }//y=162  x1=24 x2=36
  tft.drawFastHLine(x1,y,17,WHITE);
  y++;//163
  x1=x1+2;//26
  tft.drawFastHLine(x1,y,13,WHITE);
  y++;//164
  x1=x1+3;//29
  tft.drawFastHLine(x1,y,7,WHITE);

  x1=26;
  y=143;
  tft.fillRect(x1,y,3,2,WHITE);
  tft.fillRect(x1-2,y+2,2,3,WHITE);
  tft.fillRect(x1,y+5,3,2,WHITE);
  tft.fillRect(x1+3,y+2,2,3,WHITE);
  tft.drawPixel(x1-1,y+1,WHITE);
  tft.drawPixel(x1-1,y+5,WHITE);
  tft.drawPixel(x1+3,y+5,WHITE);
  tft.drawPixel(x1+3,y+1,WHITE);

  x1=36;
  y=151;
  tft.fillRect(x1,y,3,2,WHITE);
  tft.fillRect(x1-2,y+2,2,3,WHITE);
  tft.fillRect(x1,y+5,3,2,WHITE);
  tft.fillRect(x1+3,y+2,2,3,WHITE);
  tft.drawPixel(x1-1,y+1,WHITE);
  tft.drawPixel(x1-1,y+5,WHITE);
  tft.drawPixel(x1+3,y+5,WHITE);
  tft.drawPixel(x1+3,y+1,WHITE);

  y=144;
  x1=37;
  tft.drawFastHLine(x1,y,2,WHITE);
  y++;
  x1--;
  for (int i = 0; i < 11; i++) {
    tft.drawFastHLine(x1,y,3,WHITE);
    y++;
    x1--;
  }
  x1++;
  tft.drawFastHLine(x1,y,2,WHITE);
}

/*
  displayHomeLayout()

  This function is used the display the layout of the home screen
*/
void displayHomeLayout() {
  createBox(65,46,210,47,WHITE,5,BLACK);

  tft.fillRect(65,93,210,40,WHITE);
  tft.fillRect(70,98,200,32,BLACK);
  int x1=65, x2=(65+210-5);
  tft.fillRect(x1,130,5,32,WHITE);
  tft.fillRect(x2,130,5,32,WHITE);
  tft.fillRect(x1,(130+32-3),210,3,WHITE);

  tft.fillRect(x1,159,5,9,WHITE);
  tft.fillRect(x2,159,5,9,WHITE);
  tft.fillRect(x1,(159+9-3),210,3,WHITE);

  tft.fillRect(x1,165,5,37,WHITE);
  tft.fillRect(x2,165,5,37,WHITE);
  tft.fillRect(x1,(165+37-3),210,3,WHITE);

  tft.fillRect(x1,199,5,33,WHITE);
  tft.fillRect(x2,199,5,33,WHITE);
  tft.fillRect(x1,(199+33-3),210,3,WHITE);

  createBox(65,232,210,47,WHITE,5,BLACK);
  createBox(65,279,210,36,WHITE,5,BLACK);

  createBox(270,46,210,62,WHITE,5,BLACK);
  createBox(270,104,210,73,WHITE,5,BLACK);
  createBox(270,172,210,74,WHITE,5,BLACK);
  createBox(270,241,210,74,WHITE,5,BLACK);
}

/*
  displayHomeText()

  This function is used to write the text of the Home Screen
*/
void displayHomeText() {
  int addY = 15;
  displayControlState();
  displayRoofState();
  displayVentState();
  displayFanState();

  writeText(121,243+addY,1,"HUMIDITY",WHITE);
  writeText(126,286+addY,1,"999.99 %",WHITE);

  writeText(93,59+addY,1,"TEMPERATURE",WHITE);
  writeText(126,106+addY,1,"CELSIUS",WHITE);
  writeText(124,135+addY,1,"999.99 °C",WHITE);
  writeText(105,175+addY,1,"FAHRENHEIT",WHITE);
  writeText(125,205+addY,1,"999.99 °F",WHITE);
}

/*
  Home_Screen()

  This function is used to display the Home Screen and set screenName to "Home"
*/
void Home_Screen() {
  BGColor = BLACK;
  tft.fillScreen(BGColor);
  displayTitle();
  displayHomeLayout();
  displaySetTempButton();
  displaySetHumiButton();
  displayHomeText();
  screenName = "Home";
  return;
}

//Setting Screens

/*
  displaySettingLayout()

  This function is used the display the layout of the Setting Screens
*/
void displaySettingLayout() {
  createBox(0,41,480,46,WHITE,5,BLACK);
  createBox(0,82,240,238,WHITE,5,BLACK);
  createBox(240,82,240,238,WHITE,5,BLACK);
  return;
}

/*
  displayDoneButton()

  This function is used to display the "Done" Buttons on the Setting Screens
*/
void displayDoneButton() {
  createBox(200,181,80,40,WHITE,5,BLACK);
  writeText(210+5,191+15,1,"DONE",WHITE);
  return;
}

/*
  displayUpArrow(int x, int y)

  int x: Position in X of the upper-left corner of the Up Arrow Buttons
  int y: Position in Y of the upper-left corner of the Up Arrow Buttons

  Thus function is used to display the Up Arrow Buttons on the Setting Screens
  (Only display ONE Up Arrow Button)
*/
void displayUpArrow(int x, int y) {
  createBox(x,y,100,50,WHITE,5,BLACK);
  x=x+30;
  y=y+50-10;
  int a=0;
  int l=40;
  for (int i=0;i<10;i++){
    for(int j=0;j<2;j++){
      tft.drawFastHLine(x,y,l-(2*a),WHITE);
      x++;
      a++;
      y--;
    }
    x--;
    a--;
    tft.drawFastHLine(x,y,l-(2*a),WHITE);
    y--;
    x++;
    a++;
  }
}

/*
  displayDownArrow(int x, int y)

  int x: Position in X of the upper-left corner of the Down Arrow Buttons
  int y: Position in Y of the upper-left corner of the UDownp Arrow Buttons

  Thus function is used to display the Down Arrow Buttons on the Setting Screens
  (Only display ONE Down Arrow Button)
*/
void displayDownArrow(int x, int y){
  createBox(x,y,100,50,WHITE,5,BLACK);
  x=x+30;
  y=y+10;
  int a=0;
  int l=40;
  for (int i=0;i<10;i++){
    for(int j=0;j<2;j++){
      tft.drawFastHLine(x,y,l-(2*a),WHITE);
      x++;
      a++;
      y++;
    }
    x--;
    a--;
    tft.drawFastHLine(x,y,l-(2*a),WHITE);
    y++;
    x++;
    a++;
  }
}

/*
  SetupTemp_Screen()

  This function is used to display the Setting Screen for the Temperature
*/
void SetupTemp_Screen() {
  setMaxTemp=EEPROM.read(MAXTEMP_ADDRESS);
  setMinTemp=EEPROM.read(MINTEMP_ADDRESS);
  float minTempC=FToC(setMinTemp);
  float maxTempC=FToC(setMaxTemp);
  BGColor=BLACK;
  tft.fillScreen(BGColor);
  displayTitle();
  displaySettingLayout();
  displayDoneButton();
  displayUpArrow(70,127);
  displayUpArrow(310,127);
  displayDownArrow(70,250);
  displayDownArrow(310,250);
  writeText(108+5,54+15,1,"TEMPERATURE SETTINGS",WHITE);
  writeText(71+5,94+15,1,"MAXIMUM",WHITE);
  writeText(314+5,94+15,1,"MINIMUM",WHITE);
  writeText(81+5,217+15,1, String(setMaxTemp),WHITE);
  writeText(78+5,186+15,1, String(maxTempC),WHITE);
  writeText(321+5,217+15,1, String(setMinTemp),WHITE);
  writeText(318+5,186+15,1, String(minTempC),WHITE);
  writeText(81+60+5,217+15,1, "F",WHITE);
  writeText(78+60+5,186+15,1, "C",WHITE);
  writeText(321+60+5,217+15,1, "F",WHITE);
  writeText(318+60+5,186+15,1, "C",WHITE);
  screenName = "SetupTemp";
  return;
}

/*
  SetupHumi_Screen()

  This function is used to display the Setting Screen for the Humidity
*/
void SetupHumi_Screen() {
  setMaxHumi=EEPROM.read(MAXHUMI_ADDRESS);
  setMinHumi=EEPROM.read(MINHUMI_ADDRESS);
  BGColor=BLACK;
  tft.fillScreen(BGColor);
  displayTitle();
  displaySettingLayout();
  displayDoneButton();
  displayUpArrow(70,127);
  displayUpArrow(310,127);
  displayDownArrow(70,250);
  displayDownArrow(310,250);
  writeText(136+5,54+15,1,"HUMIDITY SETTINGS",WHITE);
  writeText(71+5,94+15,1,"MAXIMUM",WHITE);
  writeText(314+5,94+15,1,"MINIMUM",WHITE);
  writeText(77+5,203+15,1, String(setMaxHumi),WHITE);
  writeText(316+5,203+15,1, String(setMinHumi),WHITE);
  writeText(77+60+5,203+15,1, "%",WHITE);
  writeText(316+60+5,203+15,1, "%",WHITE);
  screenName = "SetupHumi";
  return;
}

/*
  displayHumiLimit()

  This function is used to display the modified Humidity limit
*/
void displayHumiLimit(){
  tft.fillRect(77,203,65,25,BLACK);
  tft.fillRect(316,203,65,25,BLACK);
  writeText(77+5,203+15,1, String(setMaxHumi),WHITE);
  writeText(316+5,203+15,1, String(setMinHumi),WHITE);
}

/*
  displayTempLimit()

  This function is used to display the modified Temperature limit
*/
void displayTempLimit() {
  float minTempC=FToC(setMinTemp);
  float maxTempC=FToC(setMaxTemp);
  tft.fillRect(78,186,65,50,BLACK);
  tft.fillRect(318,186,65,50,BLACK);
  writeText(81+5,217+15,1, String(setMaxTemp),WHITE);
  writeText(78+5,186+15,1, String(maxTempC),WHITE);
  writeText(321+5,217+15,1, String(setMinTemp),WHITE);
  writeText(318+5,186+15,1, String(minTempC),WHITE);
}

/*
  humiMax(String upDown)

  String upDown: "UP" or "DOWN"

  This function is used when an Arrow of the Maximum Humidity is pressed.
  It will add/remove a set omount (SETTINGNUMBER) from setMaxHumi
*/
void humiMax(String upDown){
  if (upDown=="UP"){
    setMaxHumi=setMaxHumi+SETTINGNUMBER;
    if (setMaxHumi > 100.00){
      setMaxHumi=100.00;
    }
  }
  else if (upDown=="DOWN"){
    setMaxHumi=setMaxHumi-SETTINGNUMBER;
    if (setMaxHumi < 0.00){
      setMaxHumi=0.00;
    }
  }
  displayHumiLimit();
  return;
}

/*
  humiMin(String upDown)

  String upDown: "UP" or "DOWN"

  This function is used when an Arrow of the Minimum Humidity is pressed.
  It will add/remove a set omount (SETTINGNUMBER) from setMinHumi
*/
void humiMin(String upDown){

  if (upDown=="UP"){
    setMinHumi=setMinHumi+SETTINGNUMBER;
    if (setMinHumi > 100.00){
      setMinHumi=100.00;
    }
  }
  else if (upDown=="DOWN"){
    setMinHumi=setMinHumi-SETTINGNUMBER;
    if (setMinHumi < 0.00){
      setMinHumi=0.00;
    }
  }
  displayHumiLimit();
  return;
}

/*
  tempMax(String upDown)

  String upDown: "UP" or "DOWN"

  This function is used when an Arrow of the Maximum Temperature is pressed.
  It will add/remove a set omount (SETTINGNUMBER) from setMaxTemp
*/
void tempMax(String upDown){

  if (upDown=="UP"){
    setMaxTemp=setMaxTemp+SETTINGNUMBER;
    if (setMaxTemp > 255.00){
      setMaxTemp=255.00;
    }
  }
  else if (upDown=="DOWN"){
    setMaxTemp=setMaxTemp-SETTINGNUMBER;
    if (setMaxTemp < 0.00){
      setMaxTemp=0.00;
    }
  }
  displayTempLimit();
  return;
}

/*
  tempMin(String upDown)

  String upDown: "UP" or "DOWN"

  This function is used when an Arrow of the Minimum Temperature is pressed.
  It will add/remove a set omount (SETTINGNUMBER) from setMinTemp
*/
void tempMin(String upDown){
  if (upDown=="UP"){
    setMinTemp=setMinTemp+SETTINGNUMBER;
    if (setMinTemp > 255.00){
      setMinTemp=255.00;
    }
  }
  else if (upDown=="DOWN"){
    setMinTemp=setMinTemp-SETTINGNUMBER;
    if (setMinTemp < 0.00){
      setMinTemp=0.00;
    }
  }
  displayTempLimit();
  return;
}

/*
  setHumi()

  This function is used when the Done Button from the Humidity Setting Screen is pressed
  This function saved the modified Humidity limits in the Humidity limits EEPROMs
*/
void setHumi(){
  EEPROM.write(MAXHUMI_ADDRESS,setMaxHumi);
  EEPROM.write(MINHUMI_ADDRESS,setMinHumi);
  return;
}

/*
  setTemp()

  This function is used when the Done Button from the Temperature Setting Screen is pressed
  This function saved the modified Temperature limits in the Temperature limits EEPROMs
*/
void setTemp(){
  EEPROM.write(MAXTEMP_ADDRESS,setMaxTemp);
  EEPROM.write(MINTEMP_ADDRESS,setMinTemp);
  return;
}

//Global tft

/*
  SetControlToFalse()

  This fonction is used to set asBeenPress to false
*/
void SetControlToFalse() {
  asBeenPress = false;
}

/*
  tft_setup()

  This function is used to Setup and Start the screen.
*/
void tft_setup() {
  tft.reset();

  uint16_t identifier = tft.readID();

  if(identifier == 0x9325) {
    Serial.println(F("Found ILI9325 LCD driver"));
  } else if(identifier == 0x9328) {
    Serial.println(F("Found ILI9328 LCD driver"));
  } else if(identifier == 0x7575) {
    Serial.println(F("Found HX8347G LCD driver"));
  } else if(identifier == 0x9341) {
    Serial.println(F("Found ILI9341 LCD driver"));
  } else if(identifier == 0x8357) {
    Serial.println(F("Found HX8357D LCD driver"));
  } else {
    Serial.print(F("Unknown LCD driver chip: "));
    Serial.println(identifier, HEX);
    Serial.println(F("If using the Adafruit 2.8\" TFT Arduino shield, the line:"));
    Serial.println(F("  #define USE_ADAFRUIT_SHIELD_PINOUT"));
    Serial.println(F("should appear in the library header (Adafruit_TFT.h)."));
    Serial.println(F("If using the breakout board, it should NOT be #defined!"));
    Serial.println(F("Also if using the breakout, double-check that all wiring"));
    Serial.println(F("matches the tutorial."));
    return;
  }

  tft.begin(identifier);

  tft.setFont(&FreeSans9pt7b);

  tft.setRotation(1);

  Home_Screen();
  return;
}

/*
  NOT USED
*/
int getPosition(char axis) {

  TSPoint position;
  int p;
  if (axis == 'X' || axis == 'x') {
    position = ts.getPoint();
    position.y = map(position.y, TS_MINX, TS_MAXX, tft.width(), 0);
    p = position.y;
  }
  else if (axis == 'Y' || axis == 'y') {
    position = ts.getPoint();
    position.x = map(position.x, TS_MINY, TS_MAXY, tft.height(), 0);
    p = position.x;
  }
  return p;
}

/*
  pressingPosition(TSPoint p)

  TSPoint p: position where a press was made on the screen

  This function is used to saved the pressing possition for usage
*/
void pressingPosition(TSPoint p) {
  p.x = map(p.x, TS_MAXX, TS_MINX, tft.height(), 0);
  p.y = map(p.y, TS_MINY, TS_MAXY, tft.width(), 0);
  positionX = p.y;
  positionY = p.x;
  Serial.println("Possition Taken");
  Serial.print("x == ");Serial.print(positionX);Serial.print(" | y == ");Serial.println(positionY);
  return;
}

/*
  NOT USED
*/
bool Touching() {
  bool press;
  TSPoint pressValue;
  pressValue = ts.getPoint();
  if (pressValue.z == 0){
    press = false;
  }
  else {
    press = true;
  }
  return press;
}

/*
  positionScreen_Home()

  This function is used to determin what action to take when a press was made while on the Home Screen
*/
void positionScreen_Home() {
  if (positionX >= 5 && positionX <= 60 ) {
    if (positionY >= 51 && positionY <= 110) {
      SetupTemp_Screen();
      Serial.print("Screen Name = ");Serial.println(screenName);
      return;
    }
    else if (positionY >= 115 && positionY <= 174) {
      SetupHumi_Screen();
      Serial.print("Screen Name = ");Serial.println(screenName);
      return;
    }
  }
  else if (positionX >= 275 && positionX <= 475) {
    if (positionY >= 51 && positionY <= 103) {
      switchControlState();
      displayControlState();
      Serial.print("Screen Name = ");Serial.println(screenName);
      return;
    }
    else if (positionY >= 108 && positionY <= 172) {
      if (EEPROM.read(CONTROLSTATE)==1) {
        switchFan();
        if (error[0] ==0) {
          displayFanState();
        }
        Serial.print("Screen Name = ");Serial.println(screenName);
        return;
      }
    }
    else if (positionY >= 177 && positionY <= 241) {
      if (EEPROM.read(CONTROLSTATE)==1) {
        switchVent();
        if (error[0] ==0) {
          displayVentState();
        }
        Serial.print("Screen Name = ");Serial.println(screenName);
        return;
      }
    }
    else if (positionY >= 246 && positionY <= 310) {
      if (EEPROM.read(CONTROLSTATE)==1) {
        switchRoof();
        if (error[0] ==0) {
          displayRoofState();
        }
        Serial.print("Screen Name = ");Serial.println(screenName);
        return;
      }
    }
  }
}

/*
  positionScreen_SetupTemp()

  This function is used to determin what action to take when a press was made while on the Temperature Setting Screen
*/
void positionScreen_SetupTemp() {
  if (positionX >= 75 && positionX <= 165 ) {
    if (positionY >= 132 && positionY <= 172) {
      tempMax("UP");
      return;
    }
    else if (positionY >= 255 && positionY <= 295) {
      tempMax("DOWN");
      return;
    }
  }
  else if (positionX >= 205 && positionX <= 275 ) {
    if (positionY >= 186 && positionY <= 216) {
      setTemp();
      Home_Screen();
      return;
    }
  }
  else if (positionX >= 315 && positionX <= 405 ) {
    if (positionY >= 132 && positionY <= 172) {
      tempMin("UP");
      return;
    }
    else if (positionY >= 255 && positionY <= 295) {
      tempMin("DOWN");
      return;
    }
  }
}

/*
  positionScreen_SetupHumi()

  This function is used to determin what action to take when a press was made while on the Humidity Setting Screen
*/
void positionScreen_SetupHumi() {
  if (positionX >= 75 && positionX <= 165 ) {
    if (positionY >= 132 && positionY <= 172) {
      humiMax("UP");
      return;
    }
    else if (positionY >= 255 && positionY <= 295) {
      humiMax("DOWN");
      return;
    }
  }
  else if (positionX >= 205 && positionX <= 275 ) {
    if (positionY >= 186 && positionY <= 216) {
      setHumi();
      Home_Screen();
      return;
    }
  }
  else if (positionX >= 315 && positionX <= 405 ) {
    if (positionY >= 132 && positionY <= 172) {
      humiMin("UP");
      return;
    }
    else if (positionY >= 255 && positionY <= 295) {
      humiMin("DOWN");
      return;
    }
  }
}

/*
  positionScreen_Error()

  This function is used to determin what action to take when a press was made while on the Error Screen
*/
void positionScreen_Error() {
  if (positionX >= 265 && positionX <= 475) {
    if (positionY >= 225 && positionY <= 315) {
      removeError();
      Home_Screen();
      return;
    }
  }
}

/*
  ScreenIsTouched()

  This function is used to verify which screen is active at the moment and
  called the corresponding function for the action decision
*/
void ScreenIsTouched(){
  if (screenName == "Home") {
    positionScreen_Home();
    return;
  }
  else if (screenName == "SetupTemp") {
    positionScreen_SetupTemp();
    return;
  }
  else if (screenName == "SetupHumi") {
    positionScreen_SetupHumi();
    return;
  }
  else if (screenName == "Error") {
    positionScreen_Error();
    return;
  }
  return;
}

/*
  isTouching()

  This function is used to take the pressing position and call pressingPosition if all the condition are meet.
  The first time the function is called the function will call pressingPosition, set demandAfterTouch to 0 and touching to true
  The next few times the function is being called demandAfterTouch won't be equals to POSITIONLOOPING so it will add 1 to demandAfterTouch each times
  until it is equals to POSITIONLOOPING.

  The variable asBeenPress is being set to true (in the main loop) when this function is returning true

*/
bool isTouching() {
  bool touching;
  TSPoint p;
  p = ts.getPoint();
  if (demandAfterTouch==POSITIONLOOPING) {
    if (p.z > 10 && p.z < 1000) {
      demandAfterTouch=0;
      if (asBeenPress==false){
        touching = true;
        pressingPosition(p);
      }
      else {
        touching=true;
      }
    }
    else {
      touching = false;
      demandAfterTouch=POSITIONLOOPING;
    }
  }
  else {
    touching=true;
    Serial.print("TouchScreen Cooldown:");Serial.println(demandAfterTouch);
    demandAfterTouch++;
  }

  return touching;
}

//INTERRUPT

/*
  virtualTemperature()

  This function is used in accordance with an interrupt to allow a switch from real Teemperature to a
  virtual Temperature coming from a potentiometer.
*/
void virtualTemperature() {
  if (EEPROM.read(VIRTUALTEMP)==0) {
    EEPROM.write(VIRTUALTEMP,1);
  }
  else {
    EEPROM.write(VIRTUALTEMP,0);
  }
  return;
}

/*
  virtualHumidity()

  This function is used in accordance with an interrupt to allow a switch from real Humidity to a
  virtual Humidity coming from a potentiometer.
*/
void virtualHumidity() {
  if (EEPROM.read(VIRTUALHUMI)==0) {
    EEPROM.write(VIRTUALHUMI,1);
  }
  else {
    EEPROM.write(VIRTUALHUMI,0);
  }
  return;
}

//GLOBAL 2

/*
  globalSetup()

  This function is used during the setup loop to;
  - Start the Serial communication
  - Provide Program information
  - Setup Pins as input and outputs
  - Attach the Interupts
*/
void globalSetup() {
  Serial.begin(115200);
  Serial.println("Programe Started");

  version = functionVersion();
  Serial.print("Fonction Version: ");
  Serial.println(version);

  pinMode(CLOSEPIN, INPUT);
  pinMode(OPENPIN, INPUT);
  pinMode(NOERRORLED ,OUTPUT);
  pinMode(ERRORLED, OUTPUT);
  pinMode(ERRORTOPLED, OUTPUT);
  pinMode(ERRORVENTLED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(TEMPSWITCH),virtualTemperature, FALLING);
  attachInterrupt(digitalPinToInterrupt(HUMISWITCH),virtualHumidity, FALLING);
  //EEPROM.write(CONTROLSTATE,0);
  removeError();
}

/*
  tempLoop()

  This function is used to call the checkTemp and checkHumi functions
*/
void tempLoop() {
  //displayTemperature();
  //displayHumidity();
  checkTemp();
  checkHumi();

}
