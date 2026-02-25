/**
GPL v3.0
Copyright (c) 2017 by Hui Lu
*/

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include "Wire.h"
#include <EEPROM.h>
#include <SPI.h>

#include "EPD_drive.h"
#include "EPD_drive_gpio.h"
#include "bitmaps.h"

#include "FS.h"

#define debug 1; ///< 调试模式，打开串口输出
WaveShare_EPD EPD;

void setup() { 
  
  #ifdef debug
  Serial.begin(115200);  
  Serial.println(ESP.getResetInfoPtr()->reason);
  Serial.printf("Serial begins at %dms\n",millis()); 
  #endif
 
  
  pinMode(CS,OUTPUT);//io初始化
  pinMode(DC,OUTPUT);
  pinMode(RST,OUTPUT);
  pinMode(BUSY,INPUT);
  pinMode(CLK,OUTPUT);
  pinMode(DIN,OUTPUT);
  
  SPIFFS.begin();

 EPD.EPD_Set_Model(OPM42);//设置屏幕类型
 EPD.EPD_init_Full();//全刷初始化
 EPD.clearbuffer();//清空缓存
 EPD.fontscale=2;//字体缩放系数
 EPD.SetFont(FONT12);//选择字体
 EPD.DrawUTF(0,0,"六畜兴旺");//显示字符串
 EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer,1);//刷新屏幕
 EPD.ReadBusy_long();//等待屏幕刷新完成


 EPD.deepsleep();//睡眠
}
void loop() 
{
  
}
