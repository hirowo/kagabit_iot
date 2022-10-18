#define BLYNK_DEVICE_NAME "KAGABIT"
#define BLYNK_TEMPLATE_ID "KAGABITIOT_FIRM"
//#define BLYNK_AUTH_TOKEN ""

#include <SoftwareSerial.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <ESPmDNS.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "Ambient.h"
#include "time.h"


#include "PubSubClient.h"

#define RXPIN 27
#define TXPIN 26
#define CMD_MAX 80
#define SSID_MAX 40
#define PASS_MAX 40
#define AUTH_MAX 40
#define TEMP_MAX 40

#define MAX_MOJI 80
#define DATA_MAX 90

// DISPLAY SETTINGS
#define OLED_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

//MQTT関連
#define DATA_LENGTH 32
#define TIMEOUT  10

// BLYNK関連
char *temp_id ;
#define BLYNK_TEMPLATE_ID temp_id

//LED
#define PIN 16

/*** データ構造定義 ***/
/* コマンド関数とコマンド文字列のマッピング */
typedef struct {
  int  (* func)(char *);
  char    *funcname;
} COMMAND_LIST;

/* プロトタイプ宣言 */
int command_exe(char *);//コマンド実行
int ssid_set(char *);//SSIDをセット
int pas_set(char *);// PASSWORDをセット
int w_start(char *);
int d_func(char *);
int mdsn_set(char *);
int start_amb(char *);
//AMBIENT関連
int set_amb(char *);
int send_amb(char *);
//MQTT関連
int set_mqtt(char *);
int mqtt_sub(char *);
int mqtt_pub(char *);
const int mqttPort = 1883;

//NTP関連
int time_get(char *);
int read_time(char *);

//IFTTT関連
//int get_key(char *);
//int ift_send(char *);

//BLYNK関連
int get_auth(char *);
int sta_blynk(char *);
int send_blynk(char *);

int blynk;



/* コマンド一覧*/
const COMMAND_LIST CommandList[] = {
  {ssid_set,            "SS"},          // SSID設定コマンド
  {pas_set,             "PA"},          // パスワード設定コマンド
  {w_start,             "WS"},          //WIFI接続コマンド
  {mdsn_set,            "MD"},          //MDSN文字列セット
  {start_amb,           "SAMB"},        //AMBIENT開始
  {set_amb,            "STA"},        //AMBIENT用データをセット
  {send_amb,            "SEA"},        //AMBIENT用データをセット
  {set_mqtt,            "SMT"},        //MQTTサーバーに接続
  {mqtt_sub,            "SUB"},        //SUBする
  {mqtt_pub,            "PUB"},        //PUBする
  {time_get,            "TG"},          //時間を取得する
  {read_time,           "RT"},          //時間を送信する
  //    {get_key,             "GK"},          //IFTTT用のKEYとEVENT名を取得
  //    {ift_send,            "IFT"},         //IFTTTに送信する
  {get_auth,            "GTA"},        //AUTH_TOKEN設定コマンド
  {sta_blynk,           "STB"},          //BLYNK開始
  {send_blynk,           "SNB"},          //BLYNK開始


  {0, 0}

};

//コマンド文字列
char cmd[CMD_MAX];
//文字列容ポインタ
int cmd_pointer;
//SSID格納領域
char ssid[SSID_MAX];
//パスワード格納領域
char pass[PASS_MAX];
//
//BLYNK_AUTH_TOKEN格納領域
char auth[AUTH_MAX];


int server_on;
//文字列の位置
int po;
//サーバー表示用文字格納領域
typedef struct {
  char on;
  char data[MAX_MOJI];
} HTML_DAT;

HTML_DAT html_dat[DATA_MAX];
// MQTTクライアントID格納用
char mqtt_clientid[30];
int mqtt_subd;

//macアドレス格納領域
byte mac_addr[6];

WiFiClient client;
Ambient ambient;
SoftwareSerial ss(RXPIN, TXPIN, false);
PubSubClient mqttclient(client);

void handleRoot() {

}

void handleNotFound() {



}


/****************************************/
/*   ssid_set(_char *)                 */
/*  SSIDを格納する　  */
/*    引数　: input  文字列(SSID)             */
/*    戻り値　-値　エラー　0 正常終了   */
/****************************************/


int ssid_set(char *buff)
{
  int rtn;
  Serial.println("ssid");
  if (sizeof(buff) < SSID_MAX) {
    strcpy(ssid, buff);
    rtn = 0 ;
  }
  else {
    rtn = -1;
  }
  return rtn;
}

/****************************************/
/*   pas_set(char *)                 */
/*  passwordを格納する*/
/*    引数　: input  文字列             */
/*    戻り値　-値　エラー　0 正常終了   */
/****************************************/
int pas_set(char *buff)
{
  int rtn;
  Serial.println("pass");
  if (sizeof(buff) < PASS_MAX) {
    strcpy(pass, buff);
    rtn = 0 ;
  }
  else {
    rtn = -1;
  }
  return rtn;
}

/****************************************/
/*   mdsn_set(char *)                 */
/*  ホスト名をセット
  /*    引数　: input  文字列ホスト名             */
/*    戻り値　-値　エラー　0 正常終了   */
/****************************************/
int mdsn_set(char *buff)
{
  MDNS.begin(buff);
  return 0;
}


/****************************************/
/*   w_start(char *)                 */
/*  SSIDとPASSWORDをセット                */
/*   wi-fiに接続出来たら、micro:bitに対して$を返す*/
/*   シリアルには、ipアドレスを出力             */
/*    引数　: input  文字列(未使用)             */
/*    戻り値　-値　エラー　0 正常終了   */
/****************************************/
int w_start(char *dumy)
{
  int rtn;
  int timeout;
  timeout = 0;
  WiFi.begin(ssid, pass);
  // Wait for connection
  while (1) {
    rtn = WiFi.status();
    if (rtn == WL_CONNECTED) {
      break;
    }
    else if (rtn == WL_CONNECT_FAILED) {
      WiFi.begin(ssid, pass);//リトライ
    }
    delay(500);
    Serial.print(".");
    if (timeout < TIMEOUT) {
      timeout++;
    }
    else {
      ss.println("X 1");
      return -1;
    }
  }
  delay(500);
  ss.println("$");
  Serial.println("connect");
  Serial.println(WiFi.localIP());
  return 0;
}



/****************************************/
/*  start_amb(char *)                 */
/*  ChannelIDとKEYをセット                  */
/*    引数　: input  文字列 ChannelIDとKEY  */
/*    戻り値　-値　エラー　0 正常終了         */
/****************************************/
int start_amb(char *buff)
{
  char *channelid_str;
  char *key;
  char *none;

  int channelid;
  channelid_str = strtok(buff, " ");
  key = strtok(NULL, " ");
  if (key == NULL) {
    Serial.println("NO_STR");
  }
  channelid = (int)strtoul(channelid_str, &none, 10);

  //  チャネルIDとライトキーを指定してAmbientの初期化
  if (ambient.begin(channelid, key, &client) != true) {
    //エラーコード送信
    ss.println("X 2");
  }


}



/****************************************/
/*  set_amb(char *)                 */
/*  チャート番号と送信データをセット                  */
/*    引数　: input  文字列 ChannelIDとKEY  */
/*    戻り値　-値　エラー　0 正常終了         */
/****************************************/
int set_amb(char *buff)
{
  char *chart_str;
  char *data_str;
  int chart;
  double txdata;
  char *none;

  chart_str = strtok(buff, " ");
  data_str = strtok(NULL, " ");

  chart = (int)strtoul(chart_str, &none, 10);
  txdata = atof(data_str);

  //  チャート番号とデータをパケットにセット

  if (ambient.set(chart, txdata) != true) {
    //エラーコード送信
    ss.println("X 3");
  }
  return 0;
}


/****************************************/
/*  send_amb(char *)                    */
/*  ambientにデータを送信する                      */
/*    引数　: ダミー                      */
/*    戻り値　-値　エラー　0 正常終了         */
/****************************************/
int send_amb(char *buff)
{
  //  チャート番号とデータを送信する
  if (ambient.send() != true) {
    ss.println("X 4");
  }
  return 0;
}
/****************************************/
/*  callback(char *)                    */
/*  MQTTサーバーで受信したらコールされる。      */
/*    引数　: １トピック　2 受信データ先頭アドレス　3 受信長　*/
/*    戻り値　なし         */
/****************************************/

void callback(char* topic, byte* payload, unsigned int length) {
  char payload_ch[DATA_LENGTH];
  int chlen = min(DATA_LENGTH - 1, (int)length);
  memcpy(payload_ch, payload, chlen);
  payload_ch[chlen] = 0;

  Serial.println(payload_ch);
  //micro:bitに送信
  ss.write('#');
  ss.write(' ');
  ss.println(payload_ch);
}

/****************************************/
/*  set_mqtt(char *)                    */
/*  MQTTサーバーに接続する                      */
/*    引数　: MQTTサーバーアドレス                      */
/*    戻り値　-値　エラー　0 正常終了         */
/****************************************/
int  set_mqtt(char *buff)
{
  int timeout;
  timeout = 0;
  mqttclient.setServer(buff, mqttPort);
  mqttclient.setCallback(callback);
  while (!mqttclient.connected()) {
    Serial.println("Connecting to MQTT…");
    sprintf(mqtt_clientid, "ESP32CLI_%02x%02x%02x%02x%02x%02x", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.println("MAC");
    if (mqttclient.connect(mqtt_clientid)) {
      Serial.println("connected");
    } else {
      Serial.print("Failed with state ");
      Serial.print(mqttclient.state());
      delay(2000);
    }
    if (timeout < TIMEOUT) {
      timeout ++;
    }
    else {//タイムアウト処理
      ss.println("X 5");
      break;
    }
  }


}
/****************************************/
/*  mqtt_sub(char *)                    */
/*  MQTTサーバーからサブスクライブする          */
/*  受け取っとったら、コールバック関数が呼ばれます */
/*    引数　: mqttトピック                      */
/*    戻り値　-値　エラー　0 正常終了         */
/****************************************/
int mqtt_sub(char *buff) {

  int timeout;
  timeout = 0;
  if (!mqttclient.connected()) {
    Serial.print("Waiting MQTT connection...");
    while (!mqttclient.connected()) { // 非接続のあいだ繰り返す
      if (mqttclient.connect(mqtt_clientid)) {
        mqttclient.subscribe(buff);
      } else {
        delay(2000);
      }
      if (timeout < TIMEOUT) {
        timeout ++;
      }
      else {//タイムアウト処理
        ss.println("X 5");
        break;
      }
    }
    Serial.println("connected");
  }
  Serial.println(buff);
  mqttclient.subscribe(buff);
  mqtt_subd = 1;
  Serial.println("subsc");

  return 0;
}
/****************************************/
/*  mqtt_sub(char *)                    */
/*  MQTTサーバーにパブリッシュする          */
/*    引数　: mqttトピック　データ           */
/*    戻り値　-値　エラー　0 正常終了         */
/****************************************/
int mqtt_pub(char *buff) {
  char *topic_str;
  char *data_str;
  int mqtt_topic, txdata;
  char *none;
  int timeout;
  timeout = 0;


  topic_str = strtok(buff, " ");
  data_str = strtok(NULL, " ");
  if (!mqttclient.connected()) {
    Serial.print("Waiting MQTT connection...");
    while (!mqttclient.connected()) { // 非接続のあいだ繰り返す
      if (mqttclient.connect(mqtt_clientid)) {
        mqttclient.subscribe(buff);
      } else {
        delay(2000);
      }
      if (timeout < TIMEOUT) {
        timeout ++;
      }
      else {//タイムアウト処理
        ss.println("X 5");
        break;
      }

    }
    Serial.println("connected");
  }
  txdata = (int)strtoul(data_str, &none, 10);
  mqttclient.publish(topic_str , data_str, true);
}

/****************************************/
/*  get_tim(char *)                    */
/*  NTPサーバーから時間を取得する          */
/*    引数　: ダミー＾　　           */
/*    戻り値　-値　エラー　0 正常終了         */
/****************************************/
const char* ntpServer = "ntp.jst.mfeed.ad.jp";
const long  gmtOffset_sec = 3600 * 9;
const int   daylightOffset_sec = 0;
struct tm timeinfo;
//init and get the time

int time_get(char *) {
  int rtn;
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    ss.println("X 6");
    rtn = -1;
  }
  else {
    rtn = 0;
  }
  return rtn;

}

/****************************************/
/*  read_tim(char *)                    */
/*  読み取った時間データを送信          */
/*    引数　: mqttトピック　データ           */
/*    戻り値　-値　エラー　0 正常終了         */
/****************************************/
int read_time(char *date) {

  ss.write('T');
  ss.write(' ');

  if (*date == '1') {
    ss.println(timeinfo.tm_year + 1900);
    Serial.println(timeinfo.tm_year + 1900);
  }
  else if (*date == '2') {
    ss.println((timeinfo.tm_mon + 1));
    Serial.println((timeinfo.tm_mon + 1));
  }
  else if (*date == '3') {
    ss.println(timeinfo.tm_mday);
    Serial.println(timeinfo.tm_mday);
  }
  else if (*date == '4') {
    ss.println(timeinfo.tm_hour);
    Serial.println(timeinfo.tm_hour);
  }
  else if (*date == '5') {
    ss.println(timeinfo.tm_min);
    Serial.println(timeinfo.tm_min);
  }
  else if (*date == '6') {
    ss.println(timeinfo.tm_sec);
    Serial.println(timeinfo.tm_sec);
  }
  else {
    ss.println("error\n");
  }

}

//  ここからIFTTT関連



//ここからBLYNK関連

int get_auth(char *buff)
{
  int rtn;
  Serial.println("auth");
  if (sizeof(buff) < AUTH_MAX) {
    
    strcpy(auth, buff);
//    strcpy(auth,"Vi3ID2pethvehJpMH5BLGqs6WJuXhv8z");
  
    Serial.println(auth);
    rtn = 0 ;
  }
  else {
    rtn = -1;
  }
  return rtn;
}

/****************************************/
/*  sta_blynk(char *)                    */
/*  BLYNK開始          */
/*    引数　: 　ダミー           */
/*    戻り値　-値　エラー　0 正常終了         */
/****************************************/
int sta_blynk(char *buff)
{
  Blynk.begin(auth, ssid, pass);
  blynk = 1;

}

/****************************************/
/*  send_blynk(char *)                    */
/*  BLYNK開始          */
/*    引数　: 　ダミー           */
/*    戻り値　-値　エラー　0 正常終了         */
/****************************************/
int send_blynk(char *buff)
{
  char *chart_str;
  char *data_str;
  int chart;
  double txdata;
  char *none;

  
  chart_str = strtok(buff, " ");
  data_str = strtok(NULL, " ");

  chart = (int)strtoul(chart_str, &none, 10);
  txdata = atof(data_str);
 digitalWrite(PIN,HIGH);
 delay(100);
  Blynk.virtualWrite(chart, txdata);
  
  digitalWrite(PIN,LOW);

}
/****************************************/
/*   command(_UBYTE *)                          */
/*  文字列から、コマンドを抽出して実行  */
/*    引数　: input  文字列             */
/*    戻り値　-値　エラー　0 正常終了   */
/****************************************/
int command_exe(char *buff)
{
  int select;
  char *cmnd, *cmdstring;
  int cmd_on;
  int rtn;
  select = 0;
  cmnd = strtok(buff, " ");
  Serial.println(cmnd);

  cmdstring = strtok(NULL, "\n");
  /* コマンド文字列検索および実行 */
  cmd_on = 0;
  while (CommandList[select].funcname != 0) {
    if (strcmp(CommandList[select].funcname, cmnd) == 0) {
      CommandList[select].func(cmdstring);
      Serial.println("CMDOK");
      cmd_on = 1;
      break;
    }
    select++;
  }
  if (cmd_on == 0) {
    Serial.println("CMDERR");
    rtn = -1;
  }
  else {
    rtn = 0;
  }
  return rtn;

}


/*********/
/* 初期化 */
/*********/

void setup() {
  int i;
  pinMode(PIN, OUTPUT);
  ss.begin(9600);
  Serial.begin(9600);

  //バージョン出力
  Serial.println("start ver 1.3\r\n");

  //macアドレス読み取り
  WiFi.macAddress(mac_addr);
  //端末に出力
  Serial.println("MAC ADDRESS IS");
  for (i = 0; i < 6; i++) {
    Serial.print(mac_addr[i], HEX);
    Serial.print(' ');
  }
  Serial.println("\r\n");

  digitalWrite(PIN,HIGH);
  
  //変数初期化
  cmd_pointer = 0;
  server_on = 0;
  po = 0;
  mqtt_subd = 0;
  blynk = 0;
}


/************/
/*メインループ*/
/***********/
void loop() {
  char str;
  int i;


  //1文字以上受信していたら
  while (ss.available() > 0) {

    str = ss.read();
    //Serial.println(str);

    if (cmd_pointer < CMD_MAX) {
      if (str > 0x09 && str < 0x7b) {
        cmd[cmd_pointer] = str;
        cmd_pointer++;
      }
      else {
        //        Serial.print("null");
      }

    }
    if (str == '\n') {
      //リターン検出でコマンド実行
      cmd_pointer--;
      cmd[cmd_pointer] = '\0';

      command_exe(cmd);
      //コマンドポインタ初期化
      cmd_pointer = 0;
    }
  }
  if (mqtt_subd == 1) {
    mqttclient.loop();
  }
  if (blynk == 1) {
 
    Blynk.run();
  }
}

/* BLYNKで受信したときに呼ばれる*/

BLYNK_WRITE(V0) {
  //スマホ側 Blynk アプリで設定したスライダー値の受信
  ss.write('B');
  ss.write('0');
  ss.write(' ');
  ss.println(param.asInt());
}
BLYNK_WRITE(V1) {
  //スマホ側 Blynk アプリで設定したスライダー値の受信
  ss.write('B');
  ss.write('1');
  ss.write(' ');
  ss.println(param.asInt());

}
BLYNK_WRITE(V2) {
  //スマホ側 Blynk アプリで設定したスライダー値の受信
  ss.write('B');
  ss.write('2');
  ss.write(' ');
  ss.println(param.asInt());

}
BLYNK_WRITE(V3) {
  //スマホ側 Blynk アプリで設定したスライダー値の受信
  ss.write('B');
  ss.write('3');
  ss.write(' ');
  ss.println(param.asInt());
}
