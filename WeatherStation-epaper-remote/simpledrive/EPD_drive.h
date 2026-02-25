 extern unsigned char UNICODEbuffer[200];
 extern String fontname;
#ifndef _EPD_DRIVE_H_
#define _EPD_DRIVE_H_

const unsigned char lut_vcomDC2[] ={
0x20,
0x0E ,0x14 ,0x01 ,
0x0A ,0x06 ,0x04 ,
0x0A ,0x0A ,0x0F ,
0x03 ,0x03 ,0x0C ,
0x06 ,0x0A ,0x00 };
const unsigned char lut_vcomDCfast[] ={
0x20,
0x00 ,0x00 ,0x00 ,
0x00 ,0x00 ,0x00 ,
0x00 ,0x00 ,0x00 ,
0x00 ,0x00 ,0x00 ,
0x00 ,0x00 ,0x00 };
const unsigned char lut_ww2[] ={//w
0x21,
0x0E ,0x14 ,0x01 ,
0x0A ,0x46 ,0x04 ,
0x8A ,0x4A ,0x0F ,
0x83 ,0x43 ,0x0C ,
0x86 ,0x0A ,0x04};

const unsigned char lut_bw2[] ={//b
0x22,
0x0E ,0x14 ,0x01 ,
0x8A ,0x06 ,0x04 ,
0x8A ,0x4A ,0x0F ,
0x83 ,0x43 ,0x0C ,
0x06 ,0x4A ,0x04 };
const unsigned char lut_wwfast[] ={//w
0x21,
0x83 ,0x00 ,0x08,
0x00 ,0x00 ,0x00 ,
0x00 ,0x00 ,0x00 ,
0x00 ,0x00 ,0x00 ,
0x00 ,0x00 ,0x00};
const unsigned char lut_bwfast[] ={//b
0x22,
0x43 ,0x04 ,0x12 ,
0x00 ,0x00 ,0x00 ,
0x00 ,0x00 ,0x00 ,
0x00 ,0x00 ,0x00 ,
0x00 ,0x00 ,0x00 };
const unsigned char lut_wb2[] ={
  0x23,
0x8E  ,0x94 ,0x01 ,
0x8A ,0x06 ,0x04 ,
0x8A ,0x4A ,0x0F ,
0x83 ,0x43 ,0x0C ,
0x06 ,0x0A ,0x04 };
const unsigned char lut_bb2[] ={
  0x24,
0x8E  ,0x94 ,0x01 ,
0x8A ,0x06 ,0x04 ,
0x8A ,0x4A ,0x0F ,
0x83 ,0x43 ,0x0C ,
0x06 ,0x0A ,0x04 };
enum FONT {
  FONT16 = 0,
  FONT32 = 1,
  FONT10 = 2,
  FONT12 = 3,
  FONT70 =5,
  FONT12_NUM=6,
  FONT24=7,
  FONT8=8,
  DIGI_NUM_100=9,
  ICON32=13,
  ICON80=12,
  ICON50=14,
};
enum epd_type{
  WF29BZ03=4,
  };
/*
 * 典型使用流程
 * 1.EPD_init_Full/EPD_init_Part初始化
 * 2.clearbuffer清除缓存图像
 * 3.SetFont设置字体
 * 4.fontscale=1设置字体缩放
 * 5.DrawUTF 绘制文字。图像等
 * 6.EPD_DisFull/EPD_Dis_Part更新屏幕图像
 * 7.deepsleep睡眠
 * 
 * 屏幕有圆点的角为（0，0）点，x方向为垂直方向，y方向为水平方向
 */
class WaveShare_EPD {
public:
  WaveShare_EPD();
  void deepsleep(void);
  
  byte fontscale;///> 字体缩放，仅支持1，2
 
  unsigned char EPDbuffer[400*300/8];///>屏幕图像
  epd_type EPD_Type;
 
  void EPD_Set_Model(byte model);
  void EPD_init_Full(void);
  void EPD_init_Part(void);
  void EPD_Dis_Full(unsigned char *DisBuffer,unsigned char Label);//1正常2全白3全黑
  void EPD_Dis_Part(int xStart,int xEnd,int yStart,int yEnd,unsigned char *DisBuffer,unsigned char Label);
  
  void SetFont(FONT fontindex);
  void DrawUTF(int16_t x,int16_t y,String code);
  void clearbuffer();
  
  void DrawYline(int start,int end, int y);//画竖线
  void DrawXline(int start,int end, int x);//画横线
  void DrawLine(int xstart,int ystart, int xend,int yend);
  void Inverse(int xStart,int xEnd,int yStart,int yEnd);

  void DrawWeatherChart(int xmin,int xmax,int ymin,int ymax,int point_n,int line_n,String tmax,String tmin,String code_d,String code_n,String text_d,String text_n,String date,String week);//绘制天气温度变化曲线
  void DrawCircle(int x,int y,int r,bool fill);//画圆圈，xy圆心，r半径
  void DrawBox(byte x,int y,int w,int h);//画矩形，填充
  void DrawEmptyBox(int x,int y,int w,int h);//画矩形，空心
  void DrawChart(int x,int y,int w, int c1, int c2, int c3, int c4, int c5,int c6);//画水平方向的柱状图，w图表宽度，c1-C6变量
  void DrawCircleChart(int x,int y,int r,int w,int c1,int c2,int c3);//画圆环图表，r半径，w圆环宽，c1-c3变量


  void DrawUnicodeChar(int16_t x,int16_t y,byte width,byte height,unsigned char *code);
  void DrawUnicodeStr(int16_t x,int16_t y,byte width,byte height,byte strlength,unsigned char *code);
  void drawXbm(int16_t xMove, int16_t yMove, int16_t width, int16_t height,unsigned char *xbm);//绘制图像
  void DrawXbm_P(int16_t xMove, int16_t yMove, int16_t width, int16_t height,const unsigned char *xbm);

  void EPD_Write(unsigned char *value, unsigned char datalen);
  unsigned char ReadBusy(void);
private:
  byte FontIndex; 
  
  int xDot;
 int yDot;
  int16_t CurrentCursor;
  byte fontwidth;
  byte fontheight;
	
	void EPD_WriteCMD(unsigned char command);
  void EPD_WriteData (unsigned char data);
	void EPD_WriteCMD_p1(unsigned char command,unsigned char para);	

  void EPD_WriteDispRam(unsigned int XSize,unsigned int YSize,unsigned char *Dispbuff,unsigned int offset,byte label);	
	void EPD_SetRamArea(uint16_t Xstart,uint16_t Xend,unsigned char Ystart,unsigned char Ystart1,unsigned char Yend,unsigned char Yend1);
	void EPD_SetRamPointer(uint16_t addrX,unsigned char addrY,unsigned char addrY1);
	void EPD_WirteLUT(unsigned char *LUTvalue,unsigned char Size);

  
	void EPD_Init(void);
  void EPD_Update_Part(void);
  void EPD_Update(void);

  int getIcon(int weathercodeindex);
  void SetPixel(int16_t x, int16_t y);
  void InversePixel(int16_t x, int16_t y);
  void DrawUTF(int16_t x,int16_t y,byte width,byte height,unsigned char *code);
  int UTFtoUNICODE(unsigned char *code);



};

#endif
