#include <GEC_Functions.h>
int x,y;
bool press;

void setup() {
  globalSetup();

  tft_setup();

  setup_ATH20();

  stateSetup();
  HumiAverage=getTempHumi("Humi");
}

void loop() {
  while (true) {
    if (error[0] != 0) {
      errorLoop();
    }
    else if (screenName=="Home"){
      displayTempHumi();
      if (EEPROM.read(CONTROLSTATE)==0){
        checkTemp();
        checkHumi();
      }
    }
    press = isTouching();
    if (press == true ) {
      if (asBeenPress==false) {
        asBeenPress=true;
        ScreenIsTouched();
      }
    }
    else if (press==false){
      SetControlToFalse();
    }
  }
}
