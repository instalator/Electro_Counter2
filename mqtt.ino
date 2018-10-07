#include <SPI.h>
#include <Ethernet2.h>
#include <PubSubClient.h>
#include <TextFinder.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#define PWR_CTRL   A2 //Контроль напряжения
#define LED     9  //Светодиод
#define CHK_ADR         100
#define CNT_ADR         10
#define PWR_ADR         20
#define POLL_ADR        30
#define RATIO_ADR       40
#define ID_CONNECT "elcounter"

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

byte mac[6]     = {0xC4, 0xE7, 0xC4, 0x9E, 0x83, 0x12}; //MAC адрес контроллера
byte mqtt_serv[] = {192, 168, 1, 190}; //IP MQTT брокера

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
const byte ID = 0x91;
char s[16];

const char htmlx0[] PROGMEM = "<html><title>Controller IO setup Page</title><body marginwidth=\"0\" marginheight=\"0\" ";
const char htmlx1[] PROGMEM = "leftmargin=\"0\" \"><table bgcolor=\"#999999\" border";
const char htmlx2[] PROGMEM = "=\"0\" width=\"100%\" cellpadding=\"1\" ";
const char htmlx3[] PROGMEM = "\"><tr><td>&nbsp Controller IO setup Page</td></tr></table><br>";
const char* const string_table0[] PROGMEM = {htmlx0, htmlx1, htmlx2, htmlx3};

const char htmla0[] PROGMEM = "<script>function hex2num (s_hex) {eval(\"var n_num=0X\" + s_hex);return n_num;}";
const char htmla1[] PROGMEM = "</script><table><form><input type=\"hidden\" name=\"SBM\" value=\"1\"><tr><td>MAC:&nbsp&nbsp&nbsp";
const char htmla2[] PROGMEM = "<input id=\"T1\" type=\"text\" size=\"1\" maxlength=\"2\" name=\"DT1\" value=\"";
const char htmla3[] PROGMEM = "\">.<input id=\"T3\" type=\"text\" size=\"1\" maxlength=\"2\" name=\"DT2\" value=\"";
const char htmla4[] PROGMEM = "\">.<input id=\"T5\" type=\"text\" size=\"1\" maxlength=\"2\" name=\"DT3\" value=\"";
const char htmla5[] PROGMEM = "\">.<input id=\"T7\" type=\"text\" size=\"1\" maxlength=\"2\" name=\"DT4\" value=\"";
const char htmla6[] PROGMEM = "\">.<input id=\"T9\" type=\"text\" size=\"1\" maxlength=\"2\" name=\"DT5\" value=\"";
const char htmla7[] PROGMEM = "\">.<input id=\"T11\" type=\"text\" size=\"1\" maxlength=\"2\" name=\"DT6\" value=\"";
const char* const string_table1[] PROGMEM = {htmla0, htmla1, htmla2, htmla3, htmla4, htmla5, htmla6, htmla7};

const char htmlb0[] PROGMEM = "\"><input id=\"T2\" type=\"hidden\" name=\"DT1\"><input id=\"T4\" type=\"hidden\" name=\"DT2";
const char htmlb1[] PROGMEM = "\"><input id=\"T6\" type=\"hidden\" name=\"DT3\"><input id=\"T8\" type=\"hidden\" name=\"DT4";
const char htmlb2[] PROGMEM = "\"><input id=\"T10\" type=\"hidden\" name=\"DT5\"><input id=\"T12\" type=\"hidden\" name=\"D";
const char htmlb3[] PROGMEM = "T6\"></td></tr><tr><td>MQTT: <input type=\"text\" size=\"1\" maxlength=\"3\" name=\"DT7\" value=\"";
const char htmlb4[] PROGMEM = "\">.<input type=\"text\" size=\"1\" maxlength=\"3\" name=\"DT8\" value=\"";
const char htmlb5[] PROGMEM = "\">.<input type=\"text\" size=\"1\" maxlength=\"3\" name=\"DT9\" value=\"";
const char htmlb6[] PROGMEM = "\">.<input type=\"text\" size=\"1\" maxlength=\"3\" name=\"DT10\" value=\"";
const char* const string_table2[] PROGMEM = {htmlb0, htmlb1, htmlb2, htmlb3, htmlb4, htmlb5, htmlb6};

const char htmlc0[] PROGMEM = "\"></td></tr><tr><td><br></td></tr><tr><td><input id=\"button1\"type=\"submit\" value=\"SAVE\" ";
const char htmlc1[] PROGMEM = "></td></tr></form></table></body></html>";
const char* const string_table3[] PROGMEM = {htmlc0, htmlc1};

const char htmld0[] PROGMEM = "Onclick=\"document.getElementById('T2').value ";
const char htmld1[] PROGMEM = "= hex2num(document.getElementById('T1').value);";
const char htmld2[] PROGMEM = "document.getElementById('T4').value = hex2num(document.getElementById('T3').value);";
const char htmld3[] PROGMEM = "document.getElementById('T6').value = hex2num(document.getElementById('T5').value);";
const char htmld4[] PROGMEM = "document.getElementById('T8').value = hex2num(document.getElementById('T7').value);";
const char htmld5[] PROGMEM = "document.getElementById('T10').value = hex2num(document.getElementById('T9').value);";
const char htmld6[] PROGMEM = "document.getElementById('T12').value = hex2num(document.getElementById('T11').value);\"";
const char* const string_table4[] PROGMEM = {htmld0, htmld1, htmld2, htmld3, htmld4, htmld5, htmld6};

////////////////////////////////////////////////////////////////////////////
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String strTopic = String(topic);
  String strPayload = String((char*)payload);
  callback_iobroker(strTopic, strPayload);
}

EthernetClient ethClient;
EthernetServer http_server(80);
PubSubClient mqtt(ethClient);

void setup() {
  MCUSR = 0;
  wdt_disable();
  cmd.reserve(100);
    pinMode(LED, OUTPUT);
  for (int i = 0 ; i < 100; i++) {
    digitalWrite(LED, !digitalRead(LED));
    delay(200 - i);
  }
  digitalWrite(LED, LOW);
  
  Serial.begin(115200);  
  if (EEPROM.read(30) != 99) { //Если первый запуск
    EEPROM.write(30, 99);
  } else {
    chk = EEPROMReadInt(CHK_ADR);
    cnt = EEPROMReadLong(CNT_ADR);
    poll = EEPROMReadInt(POLL_ADR);
    chk_S = poll - 500;
    if (chk != chk_S) {
      error = 2;
    }
  }
  httpSetup();
  mqttSetup();
  delay(200);
  wdt_enable(WDTO_8S);
  if (mqtt.connect(ID_CONNECT)) {
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
    mqtt.publish(topic_correction, "0");
  }
  else if (strTopic == topic_save) {
    if (strPayload == "true") {
      Serial.println("SS");
      save();
      mqtt.publish(topic_save, "false");
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
    mqtt.publish(topic_polling, IntToChar(poll));
  }
  else if (strTopic == topic_reset) {
    if (strPayload == "true") {
      mqtt.publish(topic_reset, "false");
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
    mqtt.publish(topic_bounce, IntToChar(bounce));
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
    mqtt.publish(topic_num, IntToChar(num));
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
  char s[16];
  sprintf(s, "%d.%d.%d.%d", Ethernet.localIP()[0], Ethernet.localIP()[1], Ethernet.localIP()[2], Ethernet.localIP()[3]);
  mqtt.publish(topic_err, " ");
  mqtt.publish(topic_cnt, IntToChar(cnt));
  mqtt.publish(topic_pwr, IntToChar(pwr));
  mqtt.publish(topic_amp, IntToChar(amp));
  mqtt.publish(topic_save, "false");
  mqtt.publish(topic_correction, "0");
  mqtt.publish(topic_connection, "true");
  mqtt.publish(topic_ip, s);
  mqtt.publish(topic_polling, IntToChar(poll));
  mqtt.publish(topic_bounce, IntToChar(bounce));
  mqtt.publish(topic_num, IntToChar(num));
  mqtt.publish(topic_reset, "false");
  mqtt.subscribe(subscr);
}
void reconnect() {
  int a = 0;
  while (!mqtt.connected()) {
    a++;    
    digitalWrite(LED, !digitalRead(LED));
    wdt_reset();
    checkHttp();
    if (mqtt.connect("elcounter")) {
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
  checkHttp();
  mqtt.loop();
  if (!mqtt.connected()) {
    if (Ethernet.begin(mac) == 0) {
        Reset();
      } else {
        reconnect();
      }
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
    mqtt.publish(topic_raw, StrToChar(cmd));
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
      mqtt.publish(topic_cnt, IntToChar(cnt));
      mqtt.publish(topic_pwr, IntToChar(pwr));
      mqtt.publish(topic_amp, IntToChar(amp));
    }
    if (num != prev_num || bounce != prev_bounce) {
      prev_num = num;
      prev_bounce = bounce;
      mqtt.publish(topic_num, IntToChar(num));
      mqtt.publish(topic_bounce, IntToChar(bounce));
    }
    if (error > 0) {
      mqtt.publish(topic_err, "err");
    } else {
      mqtt.publish(topic_err, " ");
    }
  }
}

void mqttSetup() {
  int idcheck = EEPROM.read(0);
  if (idcheck == ID) {
    for (int i = 0; i < 4; i++) {
      mqtt_serv[i] = EEPROM.read(i + 7);
    }
  }
  mqtt.setServer(mqtt_serv, 1883);
  mqtt.setCallback(callback);
}

void httpSetup() {
  int idcheck = EEPROM.read(0);
  if (idcheck == ID) {
    for (int i = 0; i < 6; i++) {
      mac[i] = EEPROM.read(i + 1);
    }
  }
  Ethernet.begin(mac);
}

void checkHttp() {
  EthernetClient http = http_server.available();
  if (http) {
    TextFinder  finder(http );
    while (http.connected()) {
      if (http.available()) {
        if ( finder.find("GET /") ) {
          if (finder.findUntil("setup", "\n\r")) {
            if (finder.findUntil("SBM", "\n\r")) {
              byte SET = finder.getValue();
              while (finder.findUntil("DT", "\n\r")) {
                int val = finder.getValue();
                if (val >= 1 && val <= 6) {
                  mac[val - 1] = finder.getValue();
                }
                if (val >= 7 && val <= 10) {
                  mqtt_serv[val - 7] = finder.getValue();
                }
              }
              for (int i = 0 ; i < 6; i++) {
                EEPROM.write(i + 1, mac[i]);
              }
              for (int i = 0 ; i < 4; i++) {
                EEPROM.write(i + 7, mqtt_serv[i]);
              }
              EEPROM.write(0, ID);
              http.println("HTTP/1.1 200 OK");
              http.println("Content-Type: text/html");
              http.println();
              for (int i = 0; i < 4; i++) {
                strcpy_P(buffer, (char*)pgm_read_word(&(string_table0[i])));
                http.print( buffer );
              }
              http.println();
              http.print("Saved!");
              http.println();
              http.print("Restart");
              for (int i = 1; i < 10; i++) {
                http.print(".");
                delay(500);
              }
              http.println("OK");
              Reset(); // ребутим с новыми параметрами
            }
            http.println("HTTP/1.1 200 OK");
            http.println("Content-Type: text/html");
            http.println();
            for (int i = 0; i < 4; i++) {
              strcpy_P(buffer, (char*)pgm_read_word(&(string_table0[i])));
              http.print( buffer );
            }
            for (int i = 0; i < 3; i++) {
              strcpy_P(buffer, (char*)pgm_read_word(&(string_table1[i])));
              http.print( buffer );
            }
            http.print(mac[0], HEX);
            strcpy_P(buffer, (char*)pgm_read_word(&(string_table1[3])));
            http.print( buffer );
            http.print(mac[1], HEX);
            strcpy_P(buffer, (char*)pgm_read_word(&(string_table1[4])));
            http.print( buffer );
            http.print(mac[2], HEX);
            strcpy_P(buffer, (char*)pgm_read_word(&(string_table1[5])));
            http.print( buffer );
            http.print(mac[3], HEX);
            strcpy_P(buffer, (char*)pgm_read_word(&(string_table1[6])));
            http.print( buffer );
            http.print(mac[4], HEX);
            strcpy_P(buffer, (char*)pgm_read_word(&(string_table1[7])));
            http.print( buffer );
            http.print(mac[5], HEX);
            for (int i = 0; i < 4; i++) {
              strcpy_P(buffer, (char*)pgm_read_word(&(string_table2[i])));
              http.print( buffer );
            }
            http.print(mqtt_serv[0], DEC);
            strcpy_P(buffer, (char*)pgm_read_word(&(string_table2[4])));
            http.print( buffer );
            http.print(mqtt_serv[1], DEC);
            strcpy_P(buffer, (char*)pgm_read_word(&(string_table2[5])));
            http.print( buffer );
            http.print(mqtt_serv[2], DEC);
            strcpy_P(buffer, (char*)pgm_read_word(&(string_table2[6])));
            http.print( buffer );
            http.print(mqtt_serv[3], DEC);
            strcpy_P(buffer, (char*)pgm_read_word(&(string_table3[0])));
            http.print( buffer );
            for (int i = 0; i < 7; i++) {
              strcpy_P(buffer, (char*)pgm_read_word(&(string_table4[i])));
              http.print( buffer );
            }
            strcpy_P(buffer, (char*)pgm_read_word(&(string_table3[1])));
            http.print( buffer );
            break;
          }
        }
        http.println("HTTP/1.1 200 OK");
        http.println("Content-Type: text/html");
        http.println();
        http.print("IOT controller [");
        http.print(ID_CONNECT);
        http.print("]: go to <a href=\"/setup\"> setup</a>");
        break;
      }
    }
    delay(1);
    http.stop();
  } else {
    return;
  }
}

void Reset() {
  for (;;) {}
}
