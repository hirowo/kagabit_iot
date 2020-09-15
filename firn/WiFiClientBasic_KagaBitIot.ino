#include <SoftwareSerial.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <string.h>
#include <stdlib.h>


#define RXPIN 27
#define TXPIN 26
#define CMD_MAX 20
#define SSID_MAX 20
#define PASS_MAX 20
#define MAX_MOJI 80
#define DATA_MAX 10


/*** データ構造定義 ***/
/* コマンド関数とコマンド文字列のマッピング */
typedef struct {
    int  (* func)(char *);
    char    *funcname;
} COMMAND_LIST;

/* プロトタイプ宣言 */
int command_exe(char *);
int ssid_set(char *);
int pas_set(char *);
int w_start(char *);
int d_func(char *);
int mdsn_set(char *);
int s_send(char *);
int s_web(char *);

const COMMAND_LIST CommandList[] = {
    {ssid_set,            "SS"},          // SSID設定コマンド
    {pas_set,             "PA"},          // パスワード設定コマンド
    {w_start,             "WS"},          //WIFI接続コマンド
    {mdsn_set,            "MD"},          //MDSN文字列セット
    {s_send,              "SSD"},         //webサーバー文字列表示
    {s_web,               "SWEB"},        //WEBサーバー開始
    {0,0}

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
int server_on;
//文字列の位置
int po;
//サーバー表示用文字格納領域
typedef struct{
   char on;
   char data[MAX_MOJI];
}HTML_DAT;

HTML_DAT html_dat[DATA_MAX];


WebServer server(80);

SoftwareSerial ss(RXPIN,TXPIN,false);

void handleRoot() {
  int i;
  for(i=0;i<DATA_MAX;i++){
    if(html_dat[i].on == 1){
      server.send(200, "text/plain", html_dat[i].data);
    }
  }
}

void handleNotFound() {

  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);

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
  if(sizeof(buff) < SSID_MAX){
    strcpy(ssid,buff);
    rtn = 0 ;
  }
  else{
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
  if(sizeof(buff) < PASS_MAX){
    strcpy(pass,buff);
    rtn = 0 ;
  }
  else{
    rtn = -1;
  }
  return rtn;
}

/****************************************/
/*   mdsn_set(char *)                 */
/*  SSIDとPASSWORDをセット
/*    引数　: input  文字列ホスト名             */
/*    戻り値　-値　エラー　0 正常終了   */
/****************************************/
int mdsn_set(char *buff)
{
  MDNS.begin(buff);
  return 0;
}

/****************************************/
/*   s_send(char *)                 */
/*  webサーバーに表示する文字列を送信
/*    引数　: input  文字列ホスト名             */
/*    戻り値　-値　エラー　0 正常終了   */
/****************************************/
int s_send(char *buff)
{
  strcpy(html_dat[po].data,buff);
  html_dat[po].on =1;
  po++;
  return 0;
}


/****************************************/
/*   s_web(char *)                 */
/*  webサーバー開始
/*    引数　: input  文字列ホスト名             */
/*    戻り値　-値　エラー　0 正常終了   */
/****************************************/
int s_web(char *buff)
{
  server.on("/", handleRoot);

  server.onNotFound(handleNotFound);
  server.begin();
  server_on = 1;
  return 0;
}

/****************************************/
/*   w_start(char *)                 */
/*  SSIDとPASSWORDをセット
/*    引数　: input  文字列(未使用)             */
/*    戻り値　-値　エラー　0 正常終了   */
/****************************************/
int w_start(char *dumy)
{
  WiFi.begin(ssid, pass);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  ss.write('$');  
  Serial.println("connect");
  return 0;
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
  char *cmnd,*cmdstring;
  int cmd_on;
  int rtn;
  select =0;
  cmnd = strtok(buff," ");
  Serial.println(cmnd);

 
  cmdstring = strtok(NULL," ");
  /* コマンド文字列検索および実行 */
  cmd_on = 0;
  while(CommandList[select].funcname != 0){
    if(strcmp(CommandList[select].funcname,cmnd) == 0) {
      CommandList[select].func(cmdstring); 
      Serial.println("CMDOK");
      cmd_on =1;
      break;
    }
    select++; 
  }
  if(cmd_on == 0){
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

void setup(){
  ss.begin(9600);
  Serial.begin(115200);
  cmd_pointer =0;
  server_on =0;
  po = 0;
}


/************/
/*メインループ*/
/***********/
void loop(){
  char str;
  int i;
  //1文字以上受信していたら
  while (ss.available() > 0) {
    
    str = ss.read();
    Serial.println(str);
    Serial.println(str,HEX);
    
    if(cmd_pointer < CMD_MAX){
      if(str > 0x09 && str < 0x7a){
        cmd[cmd_pointer] = str;
        cmd_pointer++;
      }
      else {
        Serial.print("null");
      }

    }
    if(str == '\n'){
      //リターン検出でコマンド実行
      cmd_pointer--;
      cmd[cmd_pointer] = '\0';
     
      command_exe(cmd);
      //コマンドポインタ初期化
      cmd_pointer = 0;
    }
  }
  if(server_on == 1){
    server.handleClient();
  }
}
