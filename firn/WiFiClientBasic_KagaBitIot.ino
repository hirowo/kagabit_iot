#include <SoftwareSerial.h>
#include <WiFi.h>
#include <string.h>
#include <stdlib.h>


#define RXPIN 27
#define TXPIN 26
#define CMD_MAX 20
#define SSID_MAX 20
#define PASS_MAX 20
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

const COMMAND_LIST CommandList[] = {
    {ssid_set,            "SS"},          // SSID設定コマンド
    {pas_set,            "PA"},          // パスワード設定コマンド
    {w_start,            "WS"},          //WIFI接続コマンド
    {d_func,              "DM"},
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


SoftwareSerial ss(RXPIN,TXPIN,false);

/* 初期化 */

void setup(){
  ss.begin(9600);
  Serial.begin(9600);
  cmd_pointer =0;
}

int d_func(char *buff)
{
  int rtn;
  Serial.println("dumy");
  return rtn;
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
/*   w_start(char *)                 */
/*  SSIDとPASSWORDをセット
/*    引数　: input  文字列(未使用)             */
/*    戻り値　-値　エラー　0 正常終了   */
/****************************************/
int w_start(char *dumy)
{
  Serial.println("connect");
  WiFi.begin(ssid, pass);
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
  select =0;
  cmnd = strtok(buff," ");
  Serial.println(cmnd);

 
  cmdstring = strtok(NULL," ");
  /* コマンド文字列検索および実行 */
  while(CommandList[select].funcname != 0){
    if(strcmp(CommandList[select].funcname,cmnd) == 0) {
      CommandList[select].func(cmdstring); 
      Serial.println("CMDOK");
      break;
    }
    select++; 
  }
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
    Serial.println(str,HEX);
    
    if(cmd_pointer < CMD_MAX){
      if(str > 0x09){
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
//      Serial.print(cmd_pointer);
      
      command_exe(cmd);
      //コマンドポインタ初期化
      cmd_pointer = 0;
    }
  }
}
