#ifdef LCD_SELECT

/************************************
       LCD Control Functions
************************************/
void setupLCD(){
  pinMode(LCD_SELECT,OUTPUT); digitalWrite(LCD_SELECT, HIGH);
  SPI.setClockDivider(SPI_CLOCK_DIV8);
  #ifndef FLASH_SELECT
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  #endif
}

void Last_Params_To_SPI_buff(byte* buff) { 
  for(byte i = 0; i < MAX_PARAM; i++) {
    buff[2*i]=(byte)((getParameter(i)>>8)&(0x00FF));
    buff[2*i+1]=(byte)((getParameter(i))&(0x00FF));
  }
}

byte buff[2*MAX_PARAM];
/***********************************************
        SPI Slave thread for SPI
************************************************/
NIL_WORKING_AREA(waThreadLCD, 24);
NIL_THREAD(ThreadLCD, arg) {

  nilThdSleepMilliseconds(1000);
  while(true) {
    //Last_Log_To_SPI_buff(buff);        //to display the last log in flash instead of the last Parameters
  Last_Params_To_SPI_buff(buff);       //Load Last Parameters Into SPI buffer
  digitalWrite(LCD_SELECT, LOW);       //enable Slave Select
  for(int i=0; i<sizeof(buff); i++){ 
    SPI.transfer(char(buff[i]));
    delayMicroseconds(5);              //correct transmission errors
  }
  SPI.transfer('\n');                  //double chariot return at end of communication  
  SPI.transfer('\n');
  digitalWrite(LCD_SELECT, HIGH);      // disable Slave Select
  nilThdSleepMilliseconds(1000);
  }
}


#endif

