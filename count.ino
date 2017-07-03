#include <EEPROM.h>
#include <avr/wdt.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define PWR_CTRL A0
#define CNT_ADR 10
#define PWR_ADR 20
#define IMP_ADR 30
#define BNC_ADR 40
#define NUM_ADR 50
#define CHK_ADR 100

volatile unsigned long cnt = 1 ;
volatile unsigned long prev_cnt = 0;
volatile unsigned long pwr = 0;
volatile unsigned long prev_pwr = 1;
unsigned long chk = 0;
int num = 3200;
int imp = 0;
int amp = 0;
int prev_amp = 0;
int bounce = 1;
unsigned long prev;
volatile unsigned long prev_millis;
volatile unsigned long cur_tmS = millis();
volatile unsigned long pre_tmS = cur_tmS;
volatile float diffS = 0;
volatile unsigned long tm_diffS = 0;
bool err = false;
String cmd = "";
bool strComplete = false;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  MCUSR = 0;
  wdt_disable();
  Serial.begin(115200);
  cmd.reserve(200);
  pinMode(2, INPUT);
  digitalWrite(2, HIGH);
  delay(200);
  if (EEPROM.read(1) != 88) { //Если первый запуск
    EEPROM.write(1, 88);
  } else {
    chk = EEPROMReadLong(CHK_ADR);
    cnt = EEPROMReadLong(CNT_ADR);
    pwr = EEPROMReadLong(PWR_ADR);
    imp = EEPROMReadInt(IMP_ADR);
    num = EEPROMReadInt(NUM_ADR);
    bounce = EEPROMReadInt(BNC_ADR);
    if (chk != cnt + imp + bounce) {
      err = true;
    }
  }
  wdt_enable(WDTO_8S);
  attachInterrupt(0, count, FALLING);
}

void loop() {
  wdt_reset();
  if (analogRead(PWR_CTRL) < 1000) {
    save();
  }

  if (strComplete) {
    //Serial.println(inputString);
    if (cmd.substring(0, 1) == "S") {
      if (cmd.substring(1, 2) == "N") {
        num = cmd.substring(cmd.lastIndexOf('N') + 1).toInt();
      }
      else if (cmd.substring(1, 2) == "S") {
        save();
      }
      else if (cmd.substring(1, 2) == "C") {
        cnt = atol(cmd.substring(cmd.lastIndexOf('C') + 1).c_str());
      }
      else if (cmd.substring(1, 2) == "B") {
        bounce = cmd.substring(cmd.lastIndexOf('B') + 1).toInt();
      }
      save();
    }
    cmd = "";
    strComplete = false;
  }

  tm_diffS = cur_tmS - pre_tmS;
  diffS = (float(tm_diffS) / 1000);
  pwr = 3600000 / (num * diffS);
  if ((cnt != prev_cnt || amp != prev_amp || pwr >= (prev_pwr + 10) || pwr <= (prev_pwr - 10)) && pwr > 0 && millis() - 2000 > prev) {
    prev = millis();
    Serial.print(cnt);
    Serial.print(";");
    if(pwr > (prev_pwr + 4000)){
      Serial.print(prev_pwr);
    } else {
      Serial.print(pwr);
    }
    Serial.print(";");
    Serial.print(amp);
    Serial.print(";");
    Serial.print(bounce);
    Serial.print(";");
    Serial.print(num);
    Serial.print(";");
    Serial.print(err);
    Serial.print('\n');
  }
    prev_cnt = cnt;
    prev_amp = amp;
    prev_pwr = pwr;
}

void count() {
  detachInterrupt(0);
  if (millis() - bounce > prev_millis) {
    prev_millis = millis();
    pre_tmS = cur_tmS;
    cur_tmS = millis();
    imp++;
    if (imp >= num) {
      imp = 0;
      cnt++;
    }
  }
  attachInterrupt(0, count, FALLING);
}

void serialEvent() {
  while (Serial.available() > 0) {
    char inChar = (char)Serial.read();
    cmd += inChar;
    if (inChar == '\n') {
      strComplete = true;
    }
  }
}

void save() {
  chk = cnt + imp + bounce;
  EEPROMWriteLong(CNT_ADR, cnt);
  EEPROMWriteLong(PWR_ADR, pwr);
  EEPROMWriteInt(IMP_ADR, imp);
  EEPROMWriteInt(NUM_ADR, num);
  EEPROMWriteInt(BNC_ADR, bounce);
  EEPROMWriteLong(CHK_ADR, chk);
}


void EEPROMWriteLong(int p_address, unsigned long p_value) {
  byte four = (p_value & 0xFF);
  byte three = ((p_value >> 8) & 0xFF);
  byte two = ((p_value >> 16) & 0xFF);
  byte one = ((p_value >> 24) & 0xFF);
  EEPROM.write(p_address, four);
  EEPROM.write(p_address + 1, three);
  EEPROM.write(p_address + 2, two);
  EEPROM.write(p_address + 3, one);
}

unsigned long EEPROMReadLong(int p_address) {
  long four = EEPROM.read(p_address);
  long three = EEPROM.read(p_address + 1);
  long two = EEPROM.read(p_address + 2);
  long one = EEPROM.read(p_address + 3);
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

void EEPROMWriteInt(int p_address, int p_value) {
  byte lowByte = ((p_value >> 0) & 0xFF);
  byte highByte = ((p_value >> 8) & 0xFF);
  EEPROM.write(p_address, lowByte);
  EEPROM.write(p_address + 1, highByte);
}
unsigned int EEPROMReadInt(int p_address) {
  byte lowByte = EEPROM.read(p_address);
  byte highByte = EEPROM.read(p_address + 1);
  return ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);
}
