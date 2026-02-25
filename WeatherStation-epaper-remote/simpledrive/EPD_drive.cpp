#include "FS.h"
#include "EPD_drive.h"
#include "EPD_drive_gpio.h"


unsigned char UNICODEbuffer[200];
String fontname;



 WaveShare_EPD::WaveShare_EPD(){
 
}


void WaveShare_EPD::SetFont(FONT fontindex)
{
     FontIndex=fontindex;
     switch (fontindex)
     {
     case 0:
     fontname="/font16";fontwidth=16;fontheight=16;break;
     case 1:
     fontname="/font32";fontwidth=32;fontheight=32;break;
     case 2:
     fontname="/font10";fontwidth=10;fontheight=10;break;
     case 3:
     fontname="/font12";fontwidth=12;fontheight=12;break;
     case 5:
     fontname="/font70";fontwidth=70;fontheight=70;break;
     case 6:
     fontname="/font12num";fontwidth=12;fontheight=12;break;
     case 7:
     fontname="/font24";fontwidth=24;fontheight=24;break;
     case 8:
     fontname="/font8";fontwidth=8;fontheight=8;break;
     case 9:
     fontname="/font100num";fontwidth=100;fontheight=100;break;
     case 11:
     fontname="/weathericon";fontwidth=32;fontheight=32;break;
     case 12:
     fontname="/weathericon80";fontwidth=80;fontheight=80;break;
      case 13:
     fontname="/weathericon32";fontwidth=32;fontheight=32;break;
     case 14:
     fontname="/weathericon50";fontwidth=50;fontheight=50;break;
     }
  }
  
void WaveShare_EPD::DrawUTF(int16_t x,int16_t y,String code)
{
  char buffer[200];
  code.toCharArray(buffer,200);
  DrawUTF(x,y,fontwidth,fontheight,(unsigned char *)buffer);
  }
void WaveShare_EPD::DrawUTF(int16_t x,int16_t y,byte width,byte height,unsigned char *code)
{
  int charcount;
  charcount=UTFtoUNICODE((unsigned char*)code);
  DrawUnicodeStr(x,y,width,height,charcount,(unsigned char *)UNICODEbuffer);
  
  }
int WaveShare_EPD::UTFtoUNICODE(unsigned char *code)
{ 
  int i=0;int charcount=0;
  while(code[i]!='\0')
  { 
    //Serial.println("current codei");
      //Serial.println(code[i],HEX);
      //  Serial.println(code[i]&0xf0,HEX);
    if(code[i]<=0x7f)  //ascii
    {
      
      UNICODEbuffer[charcount*2]=0x00;
      UNICODEbuffer[charcount*2+1]=code[i];
        // Serial.println("english or number");
      // Serial.println(UNICODEbuffer[charcount*2],HEX);
       //  Serial.println(UNICODEbuffer[charcount*2+1],HEX);
      i++;charcount++;
   
      }
    else if((code[i]&0xe0)==0xc0)
    {
    
      UNICODEbuffer[charcount*2+1]=(code[i]<<6)+(code[i+1]&0x3f);
      UNICODEbuffer[charcount*2]=(code[i]>>2)&0x07;
      i+=2;charcount++;
     // Serial.println("two bits utf-8");
      }
     else if((code[i]&0xf0)==0xe0)
    {
      
      UNICODEbuffer[charcount*2+1]=(code[i+1]<<6)+(code[i+2]&0x7f);
      UNICODEbuffer[charcount*2]=(code[i]<<4)+((code[i+1]>>2)&0x0f);
      
       //Serial.println("three bits utf-8");
       // Serial.println(UNICODEbuffer[charcount*2],HEX);
        // Serial.println(UNICODEbuffer[charcount*2+1],HEX);
        i+=3;charcount++;
      }
     else
     {
      return 0;}
  }
  UNICODEbuffer[charcount*2]='\0';
  return charcount;
  }
void WaveShare_EPD::DrawUnicodeChar(int16_t x,int16_t y,byte width,byte height,unsigned char *code)
 { 
 
  int offset;
  int sizeofsinglechar;
  if (height%8==0) sizeofsinglechar=(height/8)*width;
  else sizeofsinglechar=(height/8+1)*width;
 offset=(code[0]*0x100+code[1])*sizeofsinglechar;
  // Serial.println("code[1]");
  // Serial.println(code[1]);
   // Serial.println("sizeofsinglechar");
  // Serial.println(sizeofsinglechar);
  File f=SPIFFS.open(fontname,"r");
  f.seek(offset,SeekSet);
  char zi[sizeofsinglechar];
  f.readBytes(zi,sizeofsinglechar);
 /*for(int i=0;i<32;i++)
  {
     
   Serial.println(zi[i],HEX);
    }*/
  // Serial.println("offset");
   //Serial.println(offset);
   if (offset<0xff*sizeofsinglechar && FontIndex<10) 
   {drawXbm(x,y,width,height,(unsigned char *)zi); }
   else  {drawXbm(x,y,width,height,(unsigned char *)zi);}
  
//SPIFFS.end();
}

void WaveShare_EPD::DrawUnicodeStr(int16_t x,int16_t y,byte width,byte height,byte strlength,unsigned char *code)
{
  int ymax=yDot;
 
  CurrentCursor=0;
   byte sizeofsinglechar;
  if (height%8==0) sizeofsinglechar=(height/8)*width;
  else sizeofsinglechar=(height/8+1)*width;
  int ymove=0;
  int xmove=0;
  strlength*=2;
  int i=0;
  while(i<strlength)
  {
      int offset;
      offset=(code[i]*0x100+code[i+1])*sizeofsinglechar;
      if (offset<0xff*sizeofsinglechar&&fontscale==1) 
      {
      
      DrawUnicodeChar(x+xmove,y+ymove,width,height,(unsigned char *)code+i);
      ymove+=CurrentCursor+1;
      if((y+ymove+width/2)>=ymax-1) {xmove+=height+1;ymove=0;CurrentCursor=0;}
      }
     else if(offset<0xff*sizeofsinglechar&&fontscale==2) 
     {
      DrawUnicodeChar(x+xmove,y+ymove,width,height,(unsigned char *)code+i);
      ymove+=CurrentCursor+2;
      if((y+ymove+width)>=ymax-1) {xmove+=height+1;ymove=0;CurrentCursor=0;}
      }
    else if(fontscale==2)
    {
      DrawUnicodeChar(x+xmove,y+ymove,width,height,(unsigned char *)code+i);
      ymove+=width*2;
      if((y+ymove+width*2)>=ymax-1) {xmove+=height*2+2;ymove=0;CurrentCursor=0;}
      }
    else
    {
      DrawUnicodeChar(x+xmove,y+ymove,width,height,(unsigned char *)code+i);
      ymove+=width;
      if((y+ymove+width)>=ymax-1) {xmove+=height+1;ymove=0;CurrentCursor=0;}
      }
    i++;i++;
    }
  
  }
void WaveShare_EPD::drawXbm(int16_t xMove, int16_t yMove, int16_t width, int16_t height,unsigned char *xbm) {
  int16_t heightInXbm = (height + 7) / 8;
  uint8_t Data;
  for(int16_t x = 0; x < width; x++) {
    for(int16_t y = 0; y < height; y++ ) {
      if (y & 7) {
        Data <<= 1; // Move a bit
      } else {  // Read new Data every 8 bit
        Data = xbm[(y / 8) + x * heightInXbm];
      }
      // if there is a bit draw it
      if (((Data & 0x80)>>7)) {
       if(fontscale==1) {SetPixel(xMove + y, yMove + x);CurrentCursor=x;}
       if(fontscale==2) {SetPixel(xMove + y*2, yMove + x*2);SetPixel(xMove + y*2+1, yMove + x*2);SetPixel(xMove + y*2, yMove + x*2+1);SetPixel(xMove + y*2+1, yMove + x*2+1);CurrentCursor=x*2;}
       //if(fontscale==2) {SetPixel(xMove + y*2, yMove + x*2);CurrentCursor=x*2;}
      }
    }
  }
}

void WaveShare_EPD::DrawXbm_P(int16_t xMove, int16_t yMove, int16_t width, int16_t height,const unsigned char *xbm) {
 int16_t heightInXbm = (height + 7) / 8;
 uint8_t Data;
 //unsigned char temp[heightInXbm*width];
 //memcpy_P(temp, xbm, heightInXbm*width);
  
  for(int16_t x = 0; x < width; x++) {
    for(int16_t y = 0; y < height; y++ ) {
      if (y & 7) {
        Data <<= 1; // Move a bit
      } else {  // Read new Data every 8 bit
        Data = pgm_read_byte(xbm+(y / 8) + x * heightInXbm);
      }
      // if there is a bit draw it
      if (((Data & 0x80)>>7)) {
       if(fontscale==1) {SetPixel(xMove + y, yMove + x);CurrentCursor=x;}
       if(fontscale==2) {SetPixel(xMove + y*2, yMove + x*2);SetPixel(xMove + y*2+1, yMove + x*2);SetPixel(xMove + y*2, yMove + x*2+1);SetPixel(xMove + y*2+1, yMove + x*2+1);}
      }
    }
  }
}
void WaveShare_EPD::SetPixel(int16_t x, int16_t y)
{ 
  
    
      if(x<(xDot*2)&&y<(yDot*2))  EPDbuffer[x/4+y*xDot/4]&=~(0xc0>>((x%4)*2)) ;
  
   
  }

void WaveShare_EPD::clearbuffer()
{
  
  if(EPD_Type==WF29BZ03)
   {
    for(int i=0;i<(xDot*yDot/4);i++) EPDbuffer[i]=0xff;
   }
  else for(int i=0;i<(xDot*yDot/8);i++) EPDbuffer[i]=0xff;
  }
void WaveShare_EPD::EPD_Set_Model(byte model)
{
  EPD_Type=epd_type(model);
  
  switch (model)
     {
     case 4:
     xDot=128;yDot=296;break;//WF29BZ03
     }
  
  
  }
unsigned char WaveShare_EPD::ReadBusy(void)
{
  unsigned long i=0;
  for(i=0;i<2000;i++){
	
if(isEPD_BUSY!=EPD_BUSY_LEVEL) {
				Serial.println("Busy is H \r\n");
      	return 1;
      }
 
	  delay(2);
   //Serial.println("epd is Busy");
  }
  return 0;
}

void WaveShare_EPD::EPD_WriteCMD(unsigned char command)
{ 	
	EPD_CS_0;	
	EPD_DC_0;    // command write
	SPI_Write(command);
	EPD_CS_1;
}
void WaveShare_EPD::EPD_WriteData (unsigned char Data)
{
 
  EPD_CS_0; 
  EPD_DC_1;     
  SPI_Write(Data);  
  EPD_CS_1;
}

void WaveShare_EPD::EPD_WriteCMD_p1(unsigned char command,unsigned char para)
{

  EPD_CS_0; 
  EPD_DC_0;    // command write		
	SPI_Write(command);
	EPD_DC_1;		// command write
	SPI_Write(para);
  EPD_CS_1;	
}



void WaveShare_EPD::EPD_Write(unsigned char *value, unsigned char Datalen)
{
  
	unsigned char i = 0;
	unsigned char *ptemp;
	ptemp = value;

	EPD_CS_0;
	EPD_DC_0;		// When DC is 0, write command 
	SPI_Write(*ptemp);	//The first byte is written with the command value
	ptemp++;
	EPD_DC_1;		// When DC is 1, write Data
	for(i= 0;i<Datalen-1;i++){	// sub the Data
		SPI_Write(*ptemp);
		ptemp++;
	}
	EPD_CS_1;
}

void WaveShare_EPD::EPD_WriteDispRam(unsigned int XSize,unsigned int YSize,unsigned char *Dispbuff,unsigned int offset,byte label)
{
	
	int i = 0,j = 0;
	
      EPD_WriteCMD(0x10);
      EPD_CS_0; 
      EPD_DC_1;   
  for(i=0;i<(YSize*2);i++){
    for(j=0;j<(XSize*2);j++){
      SPI_Write(~*Dispbuff);
      Dispbuff++;
    }  
  } 
    EPD_CS_1; 
    return;

}





void WaveShare_EPD::EPD_Init(void)
{
  EPD_RST_0;   
  driver_delay_xms(5);  
  EPD_RST_1;
  //driver_delay_xms(100);
	
  if(EPD_Type==WF29BZ03)
  {
    EPD_WriteCMD(0x01); 
     EPD_WriteData(0x07);      
     EPD_WriteData(0x00);
     EPD_WriteData(0x0B);      
     EPD_WriteData(0x00);
    
     EPD_WriteCMD(0x06);         
     EPD_WriteData(0x07);
     EPD_WriteData(0x07);
     EPD_WriteData(0x07);
    
     EPD_WriteCMD(0x04);         
     ReadBusy();          
    
     EPD_WriteCMD(0X00);
     EPD_WriteData(0xb7);    
    
     EPD_WriteCMD(0X50);
     EPD_WriteData(0x37);
    
        EPD_WriteCMD(0x30);     
        EPD_WriteData(0x39);    
        
        EPD_WriteCMD(0x61);     
        EPD_WriteData(0x80);    
        EPD_WriteData(0x01);    
        EPD_WriteData(0x28);
        
        EPD_WriteCMD(0x82);     
        EPD_WriteData(0x0E);  
    
    }
	
}


void WaveShare_EPD::EPD_Update(void)
{
   
  	EPD_WriteCMD(0x12);
  	
}


void WaveShare_EPD::EPD_init_Full(void)
{	
  EPD_Init();

	
  EPD_Write((unsigned char *)lut_vcomDC2,sizeof(lut_vcomDC2));
  EPD_Write((unsigned char *)lut_ww2,sizeof(lut_ww2));
  EPD_Write((unsigned char *)lut_bw2,sizeof(lut_bw2));
  EPD_Write((unsigned char *)lut_wb2,sizeof(lut_wb2));
  EPD_Write((unsigned char *)lut_bb2,sizeof(lut_bb2));
  
}

void WaveShare_EPD::EPD_Dis_Full(unsigned char *DisBuffer,unsigned char Label)
{
	
	

	if(Label == 2){
    EPD_WriteDispRam(xDot/8, yDot,(unsigned char *)DisBuffer,0, 0xff); // white  
  }
  else if(Label==3)
  {
    EPD_WriteDispRam(xDot/8, yDot, (unsigned char *)DisBuffer,0,0x00);  // black
    }else{
		EPD_WriteDispRam(xDot/8, yDot, (unsigned char *)DisBuffer,0,1);	// white
	}	
	EPD_Update();	

	  
}
