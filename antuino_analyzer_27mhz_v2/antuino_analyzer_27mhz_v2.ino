#include <Wire.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>

#include "common.h"
#include "si5351.h"

//#define XTAL_FREQ 24996900l;             // Frequency of Quartz-Oszillator
uint32_t xtal_freq_calibrated = 27000000l;

unsigned long f = 10000000l;
unsigned long mode = MODE_ANTENNA_ANALYZER;

LiquidCrystal lcd(8,9,10,11,12,13);
char b[32], c[32], buff[32], serial_in[32];
int return_loss;
unsigned long frequency = 10000000l;
unsigned long fromFrequency = 14150000;
unsigned long toFrequency = 30000000;
int targetSWR = 20;  // 2.0
unsigned long sweepRange = 300000;

// unsigned long stepSize = 10000;  // 10 KHz, for sweeping
unsigned long stepSize = 50000;  // 10 KHz, for sweeping
//int openReading = 93; // in dbm
int openHF = 96;
int openVHF = 96;
int openUHF = 68;

int dbmOffset = -114;

int menuOn = 0;
unsigned long timeOut = 0;

/* for reading and writing from serial port */
unsigned char serial_in_count = 0;

void active_delay(unsigned int delay_by){
  unsigned long timeStart = millis();

  while (millis() - timeStart <= delay_by) {
      //Background Work
  }
}

void calibrateClock(){
  int knob = 0;
  // int32_t prev_calibration;


  //keep clear of any previous button press
  while (btnDown())
    active_delay(100);
  active_delay(100);

  // prev_calibration = xtal_freq_calibrated;
  xtal_freq_calibrated = 27000000l;

  si5351aSetFrequency_clk1(10000000l);
  ltoa(xtal_freq_calibrated - 27000000l, c, 10);
  printLine2(c);

  while (!btnDown())
  {
    knob = enc_read();

    if (knob > 0)
      xtal_freq_calibrated += 10;
    else if (knob < 0)
      xtal_freq_calibrated -= 10;
    else
      continue; //don't update the frequency or the display

    si5351aSetFrequency_clk1(10000000l);

    ltoa(xtal_freq_calibrated - 27000000l, c, 10);
    printLine2(c);
  }

  printLine2("Calibration set!");
  EEPROM.put(MASTER_CAL, xtal_freq_calibrated);

  while(btnDown())
    active_delay(50);
  active_delay(100);
}


#define ENC_A (A0)
#define ENC_B (A1)
#define FBUTTON (A2)
#define BACK_LIGHT   (A3)
#define DBM_READING (A6)


const int PROGMEM vswr[] = {
999,
174,
87,
58,
44,
35,
30,
26,
23,
21,
19,
18,
17,
16,
15,
14,
14,
13,
13,
12,
12,
12,
12,
11,
11,
11,
11,
10,
10,
10,
10
};

void printLine1(const char *c){
    lcd.setCursor(0, 0);
    lcd.print(c);
}

void printLine2(const char *c){
  lcd.setCursor(0, 1);
  lcd.print(c);
}



int enc_prev_state = 3;

//returns true if the button is pressed
int btnDown(){
  if (digitalRead(FBUTTON) == HIGH)
    return 0;
  else
    return 1;
}

void resetTimer(){
  //push the timer to the next
  timeOut = millis() + 30000l;
  digitalWrite(BACK_LIGHT, HIGH);
}


void checkTimeout(){
  unsigned long last_freq;

  if (timeOut > millis())
    return;
  digitalWrite(BACK_LIGHT, LOW);
  EEPROM.get(LAST_FREQ, last_freq);
  if (last_freq != frequency)
    EEPROM.put(LAST_FREQ, frequency);
}

byte enc_state (void) {
    return (analogRead(ENC_A) > 500 ? 1 : 0) + (analogRead(ENC_B) > 500 ? 2: 0);
}

int enc_read(void) {
  int result = 0;
  byte newState;
  int enc_speed = 0;

  unsigned long stop_by = millis() + 50;

  while (millis() < stop_by) { // check if the previous state was stable
    newState = enc_state(); // Get current state

    if (newState != enc_prev_state)
      delay (1);

    if (enc_state() != newState || newState == enc_prev_state)
      continue;
    //these transitions point to the encoder being rotated anti-clockwise
    if ((enc_prev_state == 0 && newState == 2) ||
      (enc_prev_state == 2 && newState == 3) ||
      (enc_prev_state == 3 && newState == 1) ||
      (enc_prev_state == 1 && newState == 0)){
        result--;
      }
    //these transitions point o the enccoder being rotated clockwise
    if ((enc_prev_state == 0 && newState == 1) ||
      (enc_prev_state == 1 && newState == 3) ||
      (enc_prev_state == 3 && newState == 2) ||
      (enc_prev_state == 2 && newState == 0)){
        result++;
      }
    enc_prev_state = newState; // Record state for next pulse interpretation
    enc_speed++;
    active_delay(1);
  }
  return(result);
}

// this builds up the top line of the display with frequency and mode
void updateDisplay() {
  int vswr_reading;
  // tks Jack Purdum W8TEE
  // replaced fsprint commmands by str commands for code size reduction

  memset(c, 0, sizeof(c));
  memset(b, 0, sizeof(b));

  ultoa(frequency, b, DEC);


  //one mhz digit if less than 10 M, two digits if more

   if (frequency >= 100000000l){
    strncat(c, b, 3);
    strcat(c, ".");
    strncat(c, &b[3], 3);
    strcat(c, ".");
    strncat(c, &b[6], 3);
  }
  else if (frequency >= 10000000l){
    strcpy(c, " ");
    strncat(c, b, 2);
    strcat(c, ".");
    strncat(c, &b[2], 3);
    strcat(c, ".");
    strncat(c, &b[5], 3);
  }
  else {
    strcpy(c, "  ");
    strncat(c, b, 1);
    strcat(c, ".");
    strncat(c, &b[1], 3);
    strcat(c, ".");
    strncat(c, &b[4], 3);
  }
  if (mode == MODE_ANTENNA_ANALYZER)
    strcat(c, "  ANT");
  else if (mode == MODE_MEASUREMENT_RX)
    strcat(c, "  MRX");
  else if (mode == MODE_NETWORK_ANALYZER)
    strcat(c, "  SNA");
  printLine1(c);

  active_delay(1); // wait for the circuit to settle down
  if (mode == MODE_ANTENNA_ANALYZER){
    return_loss = openReading(frequency) - analogRead(DBM_READING)/5;
    if (return_loss > 30)
       return_loss = 30;
    if (return_loss < 0)
       return_loss = 0;

    vswr_reading = pgm_read_word_near(vswr + return_loss);
    sprintf (c, "%ddb VSWR=%d.%01d     ", return_loss, vswr_reading/10, vswr_reading%10);
  }else if (mode == MODE_MEASUREMENT_RX){
    sprintf(c, "%d dbm         ", analogRead(DBM_READING)/5 + dbmOffset);
  }
  else if (mode == MODE_NETWORK_ANALYZER) {
    sprintf(c, "%d dbm         ", analogRead(DBM_READING)/5 + dbmOffset);
  }
  printLine2(c);
}


long prev_freq = 0;
void takeReading(long newfreq){
  long local_osc;

  if (newfreq < 20000l)
      newfreq = 20000l;
  if (newfreq < 150000000l)
  {
    if (newfreq < 50000000l)
      local_osc = newfreq + IF_FREQ;
    else
      local_osc = newfreq - IF_FREQ;
  } else {
    newfreq = newfreq / 3;
    local_osc = newfreq - IF_FREQ/3;
  }

  if (prev_freq != newfreq){
    switch(mode){
    case MODE_MEASUREMENT_RX:
      si5351aSetFrequency_clk2(local_osc);
    break;
    case MODE_NETWORK_ANALYZER:
      si5351aSetFrequency_clk2(local_osc);
      si5351aSetFrequency_clk0(newfreq);
      Serial.print(local_osc);
      Serial.print(' ');
      Serial.println(newfreq);
    break;
    default:
      si5351aSetFrequency_clk2(local_osc);
      si5351aSetFrequency_clk1(newfreq);
    }
    prev_freq = newfreq;
  }
}

// https://github.com/sweebee/Arduino-home-automation/blob/master/libraries/readVcc/readVcc.h
long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
     ADMUX = _BV(MUX5) | _BV(MUX0) ;
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  long result = (high<<8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

void setup() {
  lcd.begin(16, 2);
  Wire.begin();
  Serial.begin(9600); // This can be increased to 115200 (tested limit). However, some CH340 chips may have problems with baud rates > 9600.
  Serial.flush();
  Serial.println("i Antuino v1.3");
  analogReference(DEFAULT);

  unsigned long last_freq = 0;
  EEPROM.get(MASTER_CAL, xtal_freq_calibrated);
  EEPROM.get(LAST_FREQ, last_freq);
  EEPROM.get(OPEN_HF, openHF);
  EEPROM.get(OPEN_VHF, openVHF);
  EEPROM.get(OPEN_UHF, openUHF);

  if (0< last_freq && last_freq < 500000000l)
      frequency = last_freq;

  if (xtal_freq_calibrated < 26900000l || xtal_freq_calibrated > 27100000l)
    xtal_freq_calibrated = 27000000l;

  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(FBUTTON, INPUT_PULLUP);
  pinMode(BACK_LIGHT, OUTPUT);

  digitalWrite(BACK_LIGHT, LOW);
  printLine1("Antuino v1.3");
  sprintf(c, "Voltage = ");
  dtostrf(readVcc()/1000.0, 4, 2, &c[10]);
  printLine2(c);
  delay(2000);
  digitalWrite(BACK_LIGHT, HIGH);

  if (btnDown()){
    calibrateClock();
  }

  si5351aOutputOff(SI_CLK0_CONTROL);
  takeReading(frequency);
  updateDisplay();
}

void  menuBand(int btn){
  int knob = 0;
  int prev = 0, r;
  unsigned long prev_freq;


  if (!btn){
   printLine2("Band Select    \x7E");
   return;
  }

  printLine2("Band Select:    ");
  //wait for the button menu select button to be lifted)
  while (btnDown())
    active_delay(50);
  active_delay(50);

  while(!btnDown()){

    prev_freq = frequency;
    knob = enc_read();
    checkTimeout();

    if (knob != 0){

      resetTimer();

      if (knob < 0 && frequency > 3000000l)
        frequency -= 250000l;
      if (knob > 0 && frequency < 500000000l)
        frequency += 250000l;

      if (prev_freq <= 150000000l && frequency > 150000000l)
        frequency = 350000000l;
      if (prev_freq >= 350000000l && frequency < 350000000l)
        frequency = 149999000l;


      takeReading(frequency);
      updateDisplay();
      printLine2("Band Select");
    }
    else{
      r = analogRead(DBM_READING);
      if (r != prev){
        takeReading(frequency);
        updateDisplay();
        prev = r;
      }
    }
    active_delay(20);
  }

  menuOn = 0;
  while(btnDown())
    active_delay(50);
  active_delay(50);

  printLine2("");
  updateDisplay();
}


int tuningClicks = 0;
int tuningSpeed = 0;
void doTuning(){
  int s;

  s = enc_read();

  if (s < 0 && tuningClicks > 0)
    tuningClicks = s;
  else if (s > 0 && tuningClicks < 0)
    tuningClicks = s;
  else
    tuningClicks += s;

  tuningSpeed = (tuningSpeed * 4 + s)/5;
  //Serial.print(tuningSpeed);
  //Serial.print(",");
  //Serial.println(tuningClicks);
  if (s != 0){
    resetTimer();
    prev_freq = frequency;

    if (s > 4 && tuningClicks > 100)
      frequency += 200000l;
    else if (s > 2)
      frequency += 1000l;
    else if (s > 0)
      frequency +=  20l;
    else if (s > -2)
      frequency -= 20l;
    else if (s > -4)
      frequency -= 1000l;
    else if (tuningClicks < -100)
      frequency -= 200000l;

    takeReading(frequency);
    updateDisplay();
  }
}

void doTuning2(){
  int s;

  s = enc_read();

  if (s < 0 && tuningClicks > 0)
    tuningClicks = s;
  else if (s > 0 && tuningClicks < 0)
    tuningClicks = s;
  else
    tuningClicks += s;

  tuningSpeed = (tuningSpeed * 4 + s)/5;
//  Serial.print(tuningSpeed);
//  Serial.print(",");
//  Serial.println(tuningClicks);
  if (s != 0){
    resetTimer();

    if (tuningSpeed >= 5 && tuningClicks > 100)
      frequency += 10000000l;
    else if (tuningSpeed== 4 && tuningClicks > 100)
      frequency += 1000000l;
    else if (tuningSpeed == 3)
      frequency += 100000l;
    else if (tuningSpeed == 2)
      frequency += 10000l;
    else if (tuningSpeed == 1)
      frequency += 1000l;
    else if (tuningSpeed == 0 && s > 0)
      frequency += 100l;
    else if (tuningSpeed == 0 && s < 0)
      frequency -= 100l;
    else if (tuningSpeed == -1)
      frequency -= 1000l;
    else if (tuningSpeed == -2)
      frequency -= 10000;
    else if (tuningSpeed == -3)
      frequency -= 100000l;
    else if (tuningSpeed== -4 && tuningClicks < -100 && frequency > 1000000l)
      frequency -= 1000000l;
    else if (tuningSpeed <= -5 && tuningClicks < -100 && frequency > 10000000l)
      frequency -= 10000000l;

    if (frequency < 100000l)
      frequency = 100000l;
    if (frequency > 450000000l)
      frequency = 450000000l;
    if (tuningSpeed < 0 && frequency < 350000000l && frequency > 150000000l)
      frequency = 149999000l;
    if (tuningSpeed > 0 && frequency > 150000000l && frequency < 350000000l)
      frequency  = 350000000l;
    takeReading(frequency);
    updateDisplay();
  }
}



void menuSelectAntAnalyzer(int btn){
  if (!btn){
   printLine2("Ant. Analyzer  \x7E");
  }
  else {
    mode = MODE_ANTENNA_ANALYZER;
    printLine2("Ant. Analyzer!   ");
    active_delay(500);
    printLine2("");
    menuOn = 0;

    //switch off just the tracking source
    si5351aOutputOff(SI_CLK0_CONTROL);
    takeReading(frequency);
    updateDisplay();
  }
}

int readOpen(unsigned long f){
  int i, r;

  takeReading(f);
  delay(100);
  r = 0;
  for (i = 0; i < 10; i++){
    r += analogRead(DBM_READING)/5;
    delay(50);
  }
  sprintf(b, "%ld: %d  ", f, r/10);
  printLine2(b);
  delay(1000);

  return r/10;
}

void menuCalibrate2(int btn){
  if (!btn){
   printLine2("Calibrate SWR  \x7E");
  }
  else {
    printLine1("Disconnect ant &");
    printLine2("Click to Start..");

    //wait for the button to be raised up
    while(btnDown())
      active_delay(50);
    active_delay(50);  //debounce

    //wait for a button down
    while(!btnDown())
      active_delay(50);

    printLine1("Calibrating.....");

    int r;
    mode = MODE_ANTENNA_ANALYZER;
    printLine2("");
    delay(100);
    r = readOpen(20000000l);
    EEPROM.put(OPEN_HF, r);
    r = readOpen(140000000l);
    EEPROM.put(OPEN_VHF, r);
    r = readOpen(440000000l);    EEPROM.put(OPEN_UHF, r);

    menuOn = 0;

    printLine1("Calibrating.....");
    printLine2("OK! Reboot!     ");
    delay(1000);

    //switch off just the tracking source
    si5351aOutputOff(SI_CLK0_CONTROL);
    takeReading(frequency);
    updateDisplay();
  }
}



void menuSwitchBands(int btn){
  if (!btn){
    printLine2("Switch Bands   \x7E");
  }
  else {
    printLine1("Select a band:    ");
    printLine2("                  ");

    // wait for the button to be raised up
    while(btnDown())
      active_delay(50);
    active_delay(50);  //debounce

    int select = 0, i, btnState;

    menuOn = 2;

    while (menuOn) {
      i = enc_read();
      btnState = btnDown();

      checkTimeout();

      if (i != 0)
        resetTimer();

      if (select + i < 100)
        select += i;

      if (i < 0 && select - i >= 0)
        select += i;      //caught ya, i is already -ve here, so you add it

      // https://en.wikipedia.org/wiki/List_of_amateur_radio_frequency_bands_in_India
      if (select < 10) {
        if (!btnState) {
          printLine2("7 MHz (40m)   ");
        } else {
          frequency = 7000000;
          takeReading(frequency);
          updateDisplay();
          menuOn = 0;
        }
      } else if (select < 20) {
        if (!btnState) {
          printLine2("14 MHz (20m)  ");
        } else {
          frequency = 14000000;
          takeReading(frequency);
          updateDisplay();
          menuOn = 0;
        }
      } else if (select < 30) {
        if (!btnState) {
          printLine2("18 MHz (17m)  ");
        } else {
          frequency = 18000000;
          takeReading(frequency);
          updateDisplay();
          menuOn = 0;
        }
      } else if (select < 40) {
        if (!btnState) {
          printLine2("21 MHz (15m)  ");
        } else {
          frequency = 21000000;
          takeReading(frequency);
          updateDisplay();
          menuOn = 0;
        }
      } else if (select < 50) {
        if (!btnState) {
          printLine2("24 MHz (12m)  ");
        } else {
          frequency = 24000000;
          takeReading(frequency);
          updateDisplay();
          menuOn = 0;
        }
      } else if (select < 60) {
        if (!btnState) {
          printLine2("28 MHz (10m)  ");
        } else {
          frequency = 28000000;
          takeReading(frequency);
          updateDisplay();
          menuOn = 0;
        }
      } else if (select < 70) {
        if (!btnState) {
          printLine2("50 MHz (6m)   ");
        } else {
          frequency = 50000000;
          takeReading(frequency);
          updateDisplay();
          menuOn = 0;
        }
      } else if (select < 80) {
        if (!btnState) {
          printLine2("144 MHz (VHF) ");
        } else {
          frequency = 144000000;
          takeReading(frequency);
          updateDisplay();
          menuOn = 0;
        }
      } else if (select < 90) {
        if (!btnState) {
          printLine2("434 MHz (UHF) ");
        } else {
          frequency = 434000000;
          takeReading(frequency);
          updateDisplay();
          menuOn = 0;
        }
      } else {
        menuExit(btnState);
        menuOn = 0;
      }
    }
  }

  //debounce the button
  while(btnDown())
    active_delay(50);
  active_delay(50);
}



void menuSelectSweepWindow(int btn){
  if (!btn){
    printLine2("Sweep Range   \x7E");
  }
  else {
    printLine1("Select Sweep Range");
    printLine2("                  ");

    // wait for the button to be raised up
    while(btnDown())
      active_delay(50);
    active_delay(50);  //debounce

    int select = 0, i, btnState;

    menuOn = 2;

    while (menuOn) {
      i = enc_read();
      btnState = btnDown();

      checkTimeout();

      if (i != 0)
        resetTimer();

      if (select + i < 80)
        select += i;

      if (i < 0 && select - i >= 0)
        select += i;      //caught ya, i is already -ve here, so you add it

      if (select < 10) {
        if (!btnState) {
          printLine2("1 KHz         ");
        } else {
          sweepRange = 1000;
          updateDisplay();
          menuOn = 0;
        }
      } else if (select < 20) {
        if (!btnState) {
          printLine2("10 KHz        ");
        } else {
          sweepRange = 10000;
          takeReading(frequency);
          updateDisplay();
          menuOn = 0;
        }
      } else if (select < 30) {
        if (!btnState) {
          printLine2("100 KHz       ");
        } else {
          sweepRange = 100000;
          updateDisplay();
          menuOn = 0;
        }
      } else if (select < 40) {
        if (!btnState) {
          printLine2("500 KHz       ");
        } else {
          sweepRange =  500000;
          updateDisplay();
          menuOn = 0;
        }
      } else if (select < 50) {
        if (!btnState) {
          printLine2("1 MHz         ");
        } else {
          sweepRange = 1000000;
          updateDisplay();
          menuOn = 0;
        }
      } else if (select < 60) {
        if (!btnState) {
          printLine2("2 MHz         ");
        } else {
          sweepRange = 2000000;
          updateDisplay();
          menuOn = 0;
        }
      } else if (select < 70) {
        if (!btnState) {
          printLine2("3 MHz         ");
        } else {
          sweepRange = 3000000;
          updateDisplay();
          menuOn = 0;
        }
      } else if (select < 80) {
        if (!btnState) {
          printLine2("10 MHz        ");
        } else {
          sweepRange = 10000000;
          updateDisplay();
          menuOn = 0;
        }
      } else {
        menuExit(btnState);
        menuOn = 0;
      }
    }
  }

  //debounce the button
  while(btnDown())
    active_delay(50);
  active_delay(50);
}



void menuSelectTargetSWR(int btn){
  if (!btn){
    printLine2("Set Target SWR \x7E");
  }
  else {
    printLine1("Set Target SWR    ");
    printLine2("                  ");

    // wait for the button to be raised up
    while(btnDown())
      active_delay(50);
    active_delay(50);  //debounce

    int select = 0, i, btnState;

    menuOn = 2;

    while (menuOn) {
      i = enc_read();
      btnState = btnDown();

      checkTimeout();

      if (i != 0)
        resetTimer();

      if (select + i < 70)
        select += i;

      if (i < 0 && select - i >= 0)
        select += i;      //caught ya, i is already -ve here, so you add it

      if (select < 10) {
        if (!btnState) {
          printLine2("1.5           ");
        } else {
          targetSWR = 15;
          updateDisplay();
          menuOn = 0;
        }
      } else if (select < 20) {
        if (!btnState) {
          printLine2("2             ");
        } else {
          targetSWR = 20;
          updateDisplay();
          menuOn = 0;
        }
      } else if (select < 30) {
          printLine2("2.5           ");
        if (!btnState) {
        } else {
          targetSWR = 25;
          updateDisplay();
          menuOn = 0;
        }
      } else if (select < 40) {
        if (!btnState) {
          printLine2("3             ");
        } else {
          targetSWR = 30;
          updateDisplay();
          menuOn = 0;
        }
      } else if (select < 50) {
        if (!btnState) {
          printLine2("3.5           ");
        } else {
          targetSWR = 35;
          updateDisplay();
          menuOn = 0;
        }
      } else if (select < 60) {
        if (!btnState) {
          printLine2("4             ");
        } else {
          targetSWR = 40;
          updateDisplay();
          menuOn = 0;
        }
      } else {
        menuExit(btnState);
        menuOn = 0;
      }
    }
  }

  //debounce the button
  while(btnDown())
    active_delay(50);
  active_delay(50);
}



void frequencyToString(unsigned long f)
{
    memset(c, 0, sizeof(c));
    memset(b, 0, sizeof(b));

    ultoa(f, b, DEC);
    if (f >= 100000000l){
      strncat(c, b, 3);
      strcat(c, ".");
      strncat(c, &b[3], 2);
    }
    else if (f >= 10000000l){
      strncat(c, b, 2);
      strcat(c, ".");
      strncat(c, &b[2], 2);
    }
    else {
      strncat(c, b, 1);
      strcat(c, ".");
      strncat(c, &b[1], 2);
    }
}



void menuSweeper(int btn) {

  if (!btn){
    printLine2("SWR sweep    ");
  }
  else {
    int reading, vswr_reading;
    unsigned long x;

    unsigned long fs = 0;
    unsigned long fe = 0;
    int lvswr = 9999;

    unsigned long tfs = 0;
    unsigned long tfe = 0;

    if (frequency < 400000000) {
      fromFrequency = frequency - 3000000;
      toFrequency = frequency + 3000000;
    } else {
      fromFrequency = frequency - 10000000;
      toFrequency = frequency + 10000000;
    }
    printLine1("SWR sweeping...   ");

    memset(buff, 0, sizeof(buff));
    frequencyToString(fromFrequency);
    strcat(buff, c);
    strcat(buff, " - ");
    frequencyToString(toFrequency);
    strcat(buff, c);
    strcat(buff, "         ");
    printLine2(buff);
    active_delay(3000);

    for (x = fromFrequency; x < toFrequency; x = x + stepSize) {
      takeReading(x);
      delay(25);
      reading = openReading(x) - analogRead(DBM_READING)/5;
      if (reading < 0)
        reading = 0;
      vswr_reading = pgm_read_word_near(vswr + reading);
      if (vswr_reading/10 < 1)  // dirty stability hack
        continue;

      // Keep three lowest SWR values
      /* if (vswr_reading < vs[0]) {
        vs[2] = vs[1];
        vs[1] = vs[0];
        vs[0] = vswr_reading;

        fs[2] = fs[1];
        fs[1] = fs[0];
        fs[0] = x;
      } else if (vswr_reading < vs[1]) {
        vs[2] = vs[1];
        vs[1] = vswr_reading;

        fs[2] = fs[1];
        fs[1] = x;
      } else if (vswr_reading < vs[2]) {
        vs[2] = vswr_reading;
        fs[2] = x;
      } */

      if (vswr_reading < lvswr) {
        lvswr = vswr_reading;
        fs = x;
        fe = x;
      } else if (vswr_reading == lvswr) {
        fe = x;
      }

      if (vswr_reading < targetSWR) {
        if (tfs == 0) {
          tfs = x;
        }

        tfe = x;
      }
    }

    // showing one dip is recommended by vu2ash
    memset(buff, 0, sizeof(buff));
    frequencyToString(fs);
    strcat(buff, c);
    strcat(buff, " - ");
    frequencyToString(fe);
    strcat(buff, c);
    strcat(buff, "      ");
    printLine1(buff);
    memset(buff, 0, sizeof(buff));
    frequencyToString(fs);
    vswr_reading = lvswr;
    sprintf(buff, "%s => %d.%01d     ", c, vswr_reading/10, vswr_reading%10);
    printLine2(buff);
    active_delay(4000);

    // show bandwidth for the target swr value
    vswr_reading = targetSWR;
    if (tfs != 0) {
      memset(buff, 0, sizeof(buff));
      frequencyToString(tfs);
      strcat(buff, c);
      strcat(buff, " - ");
      frequencyToString(tfe);
      strcat(buff, c);
      strcat(buff, "      ");
      printLine1(buff);
      memset(buff, 0, sizeof(buff));
      frequencyToString(tfs);
      sprintf(buff, "%s => %d.%01d     ", c, vswr_reading/10, vswr_reading%10);
      printLine2(buff);
    } else {
      sprintf(buff, "TGT %d.%01d unmet ", vswr_reading/10, vswr_reading%10);
      printLine2(buff);
    }
    active_delay(4000);

    // debounce the button
    while(btnDown())
      active_delay(50);
    active_delay(50);

    menuOn = 0;
    return;
  }
}



void menuSelectMeasurementRx(int btn){
  if (!btn){
   printLine2("Measurement RX \x7E");
  }
  else {
    mode = MODE_MEASUREMENT_RX;
    printLine2("Measurement RX!");
    active_delay(500);
    printLine2("");
    menuOn = 0;

    //only allow the local oscillator to work
    si5351aOutputOff(SI_CLK0_CONTROL);
    si5351aOutputOff(SI_CLK1_CONTROL);
    takeReading(frequency);
    updateDisplay();
  }
}

void menuSelectNetworkAnalyzer(int btn){
  if (!btn){
   printLine2("SNA            \x7E");
  }
  else {
    mode = MODE_NETWORK_ANALYZER;
    printLine2("SNA!           ");
    active_delay(500);
    printLine2("");
    menuOn = 0;

    //switch off the clock2 that drives the return loss bridge
    si5351aOutputOff(SI_CLK1_CONTROL);
    takeReading(frequency);
    updateDisplay();
  }
}

void menuExit(int btn){

  if (!btn){
      printLine2("Exit Menu      \x7E");
  }
  else{
      printLine2("Exiting...");
      active_delay(500);
      printLine2("");
      updateDisplay();
      menuOn = 0;
  }
}

void doMenu(){
  int select=0, i,btnState;

  //wait for the button to be raised up
  while(btnDown())
    active_delay(50);
  active_delay(50);  //debounce

  menuOn = 2;

  while (menuOn){
    i = enc_read();
    btnState = btnDown();

    checkTimeout();

    if (i != 0)
        resetTimer();

    if (select + i < 90)
      select += i;

    if (i < 0 && select - i >= 0)
      select += i;      //caught ya, i is already -ve here, so you add it

    if (select < 10)
      menuBand(btnState);
    else if (select < 20)
      menuSwitchBands(btnState);
    else if (select < 30)
      menuSweeper(btnState);
    else if (select < 40)
      menuSelectNetworkAnalyzer(btnState);
    else if (select < 50)
      menuCalibrate2(btnState);
    else if (select < 60)
      menuSelectAntAnalyzer(btnState);
    else if (select < 70)
      menuSelectMeasurementRx(btnState);
    else if (select < 80)
      menuSelectSweepWindow(btnState);
     else if (select < 90)
      menuSelectTargetSWR(btnState);
    else
      menuExit(btnState);
  }

  //debounce the button
  while(btnDown())
    active_delay(50);
  active_delay(50);
}

void checkButton(){
  //only if the button is pressed
  if (!btnDown())
    return;
  active_delay(50);
  if (!btnDown()) //debounce
    return;

  doMenu();
  //wait for the button to go up again
  while(btnDown())
    active_delay(10);
  active_delay(50);//debounce
}

int openReading(unsigned long f){
  if (f < 60000000l)
    return openHF;
  else if (f < 150000000l)
    return openVHF;
  else
    return openUHF;
}

void readDetector(unsigned long f){
  int i = openReading(f) - analogRead(DBM_READING)/5;
  sprintf(c, "d%d\n", i);
  Serial.write(c);
}

void doSweep(){
  unsigned long x;
  int reading, vswr_reading;

  /* stepSize = (toFrequency - fromFrequency) / 3000;
  if (stepSize <= 0) {
    stepSize = 300;
  } */
  Serial.write("begin\n");
  for (x = fromFrequency; x < toFrequency; x = x + stepSize){
    takeReading(x);
    delay(10);
    reading = openReading(x) - analogRead(DBM_READING)/5;
    if (mode == MODE_ANTENNA_ANALYZER){
      if (reading < 0)
        reading = 0;
      vswr_reading = pgm_read_word_near(vswr + reading);
      sprintf (c, "r:%ld:%d:%d\n", x, reading, vswr_reading);
    }else
      sprintf(c, "r:%ld:%d\n", x, analogRead(DBM_READING)/5 + dbmOffset);
    Serial.write(c);
  }
  Serial.write("end\n");
}

char *readNumber(char *p, unsigned long *number){
  *number = 0;

  sprintf(c, "#%s", p);
  while (*p){
    char c = *p;
    if ('0' <= c && c <= '9')
      *number = (*number * 10) + c - '0';
    else
      break;
     p++;
  }
  return p;
}

char *skipWhitespace(char *p){
  while (*p && (*p == ' ' || *p == ','))
    p++;
  return p;
}

/* command 'h' */
void sendStatus(){
  Serial.write("helo v1\n");
  sprintf(c, "from %ld\n", fromFrequency);
  Serial.write(c);

  sprintf(c, "to %ld\n", toFrequency);
  Serial.write(c);

  sprintf(c, "mode %ld\n", mode);
  Serial.write(c);

}

void parseCommand(char *line){
  char *p = line;
  char command;

  while (*p){
    p = skipWhitespace(p);
    command = *p++;

    switch (command){
      case 'f' : //from - start frequency
        p = readNumber(p, &fromFrequency);
        takeReading(fromFrequency);
        break;
      case 'm':
        p = readNumber(p, &mode);
        updateDisplay();
        break;
      case 't':
        p = readNumber(p, &toFrequency);
        break;
      case 'v':
        sendStatus();
        break;
      case 'g':
         doSweep();
         break;
      case 'r':
         readDetector(frequency);
         break;
      case 's':
        p = readNumber(p, &stepSize);
        break;
      case 'i': /* identifies itself */
        Serial.write("i Antuino 1.3\n");
        break;
    }
  } /* end of the while loop */
}

void acceptCommand(){
  int inbyte = 0;
  inbyte = Serial.read();

  if (inbyte == '\n'){
    parseCommand(serial_in);
    serial_in_count = 0;
    return;
  }

  if (serial_in_count < sizeof(serial_in)){
    serial_in[serial_in_count] = inbyte;
    serial_in_count++;
    serial_in[serial_in_count] = 0;
  }
}

int prev = 0;
void loop() {

  doTuning2();
  checkButton();

  checkTimeout();
  if (Serial.available()>0)
    acceptCommand();

  delay(50);
  int r = analogRead(DBM_READING);
  if (r != prev){
    takeReading(frequency);
    updateDisplay();
    prev = r;
  }
}
