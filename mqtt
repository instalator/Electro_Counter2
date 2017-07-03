#include <SPI.h>
#include <Ethernet2.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <avr/wdt.h>

#define PWR_CTRL   A2 //Контроль напряжения
#define LED     9  //Светодиод
#define CHK_ADR         100
#define CNT_ADR         10
#define PWR_ADR         20
#define POLL_ADR        30
#define RATIO_ADR       40

const char* topic_err = "myhome/elcounter/error";
const char* topic_cnt = "myhome/elcounter/count";
const char* topic_pwr = "myhome/elcounter/power";
const char* topic_amp = "myhome/elcounter/amp";
const char* topic_save = "myhome/elcounter/save";
const char* topic_correction = "myhome/elcounter/correction";
const char* topic_connection = "myhome/elcounter/connection";
const char* topic_ip = "myhome/elcounter/config/ip";
const char* topic_polling = "myhome/elcounter/config/polling";
const char* topic_bounce = "myhome/elcounter/config/bounce";
const char* topic_num = "myhome/elcounter/config/num";
const char* topic_reset = "myhome/elcounter/config/reset";
const char* topic_raw = "myhome/elcounter/RAW";
const char* subscr = "myhome/elcounter/#";

byte mac[6]     = {0xC4, 0xE7, 0xC4, 0x9E, 0x83, 0x12};
byte ip[4]     = {192, 168, 1, 52};
byte server[4] = {192, 168, 1, 190};

unsigned long cnt = 0;
unsigned long prev_cnt = 2;
unsigned long pwr = 0;
unsigned long prev_pwr = 2;
int amp = 0;
int prev_amp = 2;
int num = 0;
int prev_num;
int bounce = 10;
int prev_bounce;
int chk = 0;
int chk_S = 0;
unsigned long prevMillis = 0;
unsigned long prevMillis_cnt = 0;
int poll = 5000;
bool error = 0;
String cmd = "";
bool strComplete = false;
char buf [50];
char buffer[100];
char s[16];

////////////////////////////////////////////////////////////////////////////
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String strTopic = String(topic);
  String strPayload = String((char*)payload);
  callback_iobroker(strTopic, strPayload);
}

EthernetClient ethClient;
PubSubClient client(ethClient);

void setup() {
  MCUSR = 0;
  wdt_disable();
  cmd.reserve(100);
  delay(10000);
  Serial.begin(115200);
  
  if (EEPROM.read(1) != 99) { //Если первый запуск
    EEPROM.write(1, 99);
    for (int i = 0 ; i < 4; i++) {
      EEPROM.write(110 + i, ip[i]);
    }
    for (int i = 0 ; i < 4; i++) {
      EEPROM.write(120 + i, server[i]);
    }
  } else {
    chk = EEPROMReadInt(CHK_ADR);
    cnt = EEPROMReadLong(CNT_ADR);
    poll = EEPROMReadInt(POLL_ADR);
    chk_S = poll - 500;
    if (chk != chk_S) {
      error = 2;
    }
    for (int i = 0; i < 4; i++) {
      ip[i] = EEPROM.read(110 + i);
    }
    for (int i = 0; i < 4; i++) {
      server[i] = EEPROM.read(120 + i);
    }
  }

  pinMode(LED, OUTPUT);
  
  client.setServer(server, 1883);
  client.setCallback(callback);
  Ethernet.begin(mac, ip);
  delay(200);
  wdt_enable(WDTO_8S);
  if (client.connect("elcounter")) {
      digitalWrite(LED, LOW);
      Public();
    }
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

void callback_iobroker(String strTopic, String strPayload) {
 if (strTopic == topic_correction) {
    cnt = atol((strPayload).c_str());
    Serial.print("SC");
    Serial.println(cnt);
    client.publish(topic_correction, "0");
  }
  else if (strTopic == topic_save) {
    if (strPayload == "true") {
      Serial.println("SS");
      save();
      client.publish(topic_save, "false");
    }
  }
  else if (strTopic == topic_polling) {
    poll = strPayload.toInt();
    if (poll < 1000) {
      poll = 1000;
    } else if (poll > 32767) {
      poll = 32767;
    }
    EEPROMWriteInt(POLL_ADR, poll);  //Интервал публикации
    client.publish(topic_polling, IntToChar(poll));
  }
  else if (strTopic == topic_reset) {
    if (strPayload == "true") {
      client.publish(topic_reset, "false");
      reboot();
    }
  }
  else if (strTopic == topic_bounce) {
    bounce = strPayload.toInt();
    if (bounce < 0) {
      bounce = 0;
    } else if (bounce > 10000) {
      bounce = 10000;
    }
    Serial.print("SB");
    Serial.println(bounce);
    client.publish(topic_bounce, IntToChar(bounce));
  }
  else if (strTopic == topic_num) {
    num = strPayload.toInt();
    if (num < 0) {
      num = 0;
    } else if (num > 32767) {
      num = 32767;
    }
    Serial.print("SN");
    Serial.println(num);
    client.publish(topic_num, IntToChar(num));
  }
}

void save() {
  chk = poll - 500;
  EEPROMWriteLong(CNT_ADR, cnt);  //Пишем показания счетчика в eeprom из переменной
  EEPROMWriteInt(POLL_ADR, poll);  //Интервал публикации
  EEPROMWriteInt(CHK_ADR, chk); //чек
}

void reboot() {
  save();
  delay(500);
  wdt_enable(WDTO_1S);
  for (;;) {}
}

char* StrToChar (String str) {
  int length = str.length();
  str.toCharArray(buf, length + 1);
  return buf;
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

char* IntToChar (unsigned long a) {
  sprintf(buf, "%lu", a);
  return buf;
}

void Public(){
  sprintf(s, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  client.publish(topic_err, " ");
  client.publish(topic_cnt, IntToChar(cnt));
  client.publish(topic_pwr, IntToChar(pwr));
  client.publish(topic_amp, IntToChar(amp));
  client.publish(topic_save, "false");
  client.publish(topic_correction, "0");
  client.publish(topic_connection, "true");
  client.publish(topic_ip, s);
  client.publish(topic_polling, IntToChar(poll));
  client.publish(topic_bounce, IntToChar(bounce));
  client.publish(topic_num, IntToChar(num));
  client.publish(topic_reset, "false");
  client.subscribe(subscr);
}
void reconnect() {
  int a = 0;
  while (!client.connected()) {
    a++;
    digitalWrite(LED, !digitalRead(LED));
    wdt_reset();
    if (client.connect("elcounter")) {
      digitalWrite(LED, LOW);
      Public();
    } else {
      delay(10000);
    }
    if (a >= 10) {
      wdt_enable(WDTO_1S);
    }
  }
}

void loop() {   
  wdt_reset();
  client.loop();
  if (!client.connected()) {
    reconnect();
  }

  if (analogRead(PWR_CTRL) < 1000) {
    digitalWrite(LED, LOW);
    save();
    delay(10000);
  }

  if (strComplete) {
    digitalWrite(LED, !digitalRead(LED));
    cmd.toCharArray(buffer, 100);
    cnt = atoi(strtok(buffer, ";"));
    pwr = atoi(strtok(NULL, ";"));
    amp = atoi(strtok(NULL, ";"));
    bounce = atoi(strtok(NULL, ";"));
    num = atoi(strtok(NULL, ";"));
    error = atoi(strtok(NULL, ";"));
    client.publish(topic_raw, StrToChar(cmd));
    cmd = "";
    strComplete = false;
  }

  if (millis() - prevMillis >= poll) {
    wdt_reset();
    prevMillis = millis();
    if (cnt != prev_cnt || pwr != prev_pwr || amp != prev_amp) {
      prev_cnt = cnt;
      prev_pwr = pwr;
      prev_amp = amp;
      client.publish(topic_cnt, IntToChar(cnt));
      client.publish(topic_pwr, IntToChar(pwr));
      client.publish(topic_amp, IntToChar(amp));
    }
    if (num != prev_num || bounce != prev_bounce) {
      prev_num = num;
      prev_bounce = bounce;
      client.publish(topic_num, IntToChar(num));
      client.publish(topic_bounce, IntToChar(bounce));
    }
    if (error > 0) {
      client.publish(topic_err, "err");
    } else {
      client.publish(topic_err, " ");
    }
  }
}
