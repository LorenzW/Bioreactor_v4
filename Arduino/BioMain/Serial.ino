#include "WString.h"

#ifdef THR_SERIAL

#define SERIAL_BUFFER_LENGTH 32
char serialBuffer[SERIAL_BUFFER_LENGTH];
byte serialBufferPosition=0;

NIL_WORKING_AREA(waThreadSerial, 128); // minimum 128
NIL_THREAD(ThreadSerial, arg) {

  Serial.begin(115200);
  while(true) {
    while (Serial.available()) {
      // get the new byte:
      char inChar = (char)Serial.read(); 

      if (inChar==13 || inChar==10) {
        // this is a carriage return;
        if (serialBufferPosition>0) {
          printResult(serialBuffer, &Serial);
        } 
        serialBufferPosition=0;
        serialBuffer[0]='\0';
      } 
      else {
        if (serialBufferPosition<SERIAL_BUFFER_LENGTH) {
          serialBuffer[serialBufferPosition]=inChar;
          serialBufferPosition++;
          if (serialBufferPosition<SERIAL_BUFFER_LENGTH) {
            serialBuffer[serialBufferPosition]='\0';
          }
        }
      }  
    }
    nilThdSleepMilliseconds(1);
  }
}

#endif

#define MAX_MULTI_LOG 10

void printGeneralParameters(Print* output){
  output->print(F("EPOCH:"));
  output->println(now());
  output->print(F("millis:"));
  output->println(millis());
#ifdef I2C_RELAY_FOOD
  output->print(F("I2C relay food:"));
  output->println(I2C_RELAY_FOOD); 
#endif
#ifdef FLUX
  output->print(F("I2C Flux:"));
  output->println(I2C_FLUX); 
#endif
#ifdef THR_LINEAR_LOGS
  output->print(F("Next log index:"));
  output->println(nextEntryID);
#endif
#ifdef THR_LINEAR_LOGS
  output->print(F("FlashID:"));
  sst.printFlashID(output);
#endif
}

/* SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 
 This method will mainly set/read the parameters:
 Uppercase + number + CR ((-) and 1 to 5 digit) store a parameter (0 to 25 depending the letter)
 example: A100, A-1
 -> Many parameters may be set at once
 example: C10,20,30,40,50
 Uppercase + CR read the parameter
 example: A
 -> Many parameters may be read at once
 example: A,B,C,D
 s : read all the parameters
 h : help
 l : show the log file
 */

void printResult(char* data, Print* output) {
  boolean theEnd=false;
  byte paramCurrent=0; // Which parameter are we defining
  char paramValue[SERIAL_MAX_PARAM_VALUE_LENGTH];
  paramValue[0]='\0';
  byte paramValuePosition=0;
  byte i=0;
  boolean inValue=false;

  while (!theEnd) {
    byte inChar=data[i];
    i++;
    if (i==SERIAL_BUFFER_LENGTH) theEnd=true;
    if (inChar=='\0') {
      theEnd=true;
    }
    else if ((inChar>47 && inChar<58) || inChar=='-' || inValue) { // a number (could be negative)
      if (paramValuePosition<SERIAL_MAX_PARAM_VALUE_LENGTH) {
        paramValue[paramValuePosition]=inChar;
        paramValuePosition++;
        if (paramValuePosition<SERIAL_MAX_PARAM_VALUE_LENGTH) {
          paramValue[paramValuePosition]='\0';
        }
      }
    } 
    else if (inChar>64 && inChar<92) { // an UPPERCASE character so we define the field
      // we extend however the code to allow 2 letters fields !!!
      if (paramCurrent>0) {
        paramCurrent*=26;
      }
      paramCurrent+=inChar-64;
      if (paramCurrent>MAX_PARAM) {
        paramCurrent=0; 
      }
    }
    if (inChar==',' || theEnd) { // store value and increment
      if (paramCurrent>0) {
        if (paramValuePosition>0) {
          setAndSaveParameter(paramCurrent-1,atoi(paramValue));
          output->println(parameters[paramCurrent-1]);
        } 
        else {
          output->println(parameters[paramCurrent-1]);
        }
        if (paramCurrent<=MAX_PARAM) {
          paramCurrent++;
          paramValuePosition=0;
          paramValue[0]='\0';
        } 
      }
    }
    if (data[0]>96 && data[0]<123 && (i>1 || data[1]<97 || data[1]>122)) { // we may have one or 2 lowercasee
      inValue=true;
    }
  }

  // we will process the commands, it means it starts with lowercase
  switch (data[0]) {
    
    #ifdef THR_LORA
    case 'a':
      processLoraCommand(data[1], paramValue, output);
    break;
    #endif
      
    case 'c':
      if (paramValuePosition>0) 
        printCompactParameters(output, atoi(paramValue));
      else   
        printCompactParameters(output); 
      break; 
    #ifdef THR_LINEAR_LOGS
    case 'd':
      if (paramValuePosition>0) {
        if (atol(paramValue)==1234)
          formatFlash(output);
      } 
      else 
        output->println(F("To format flash enter d1234"));
      break;
    #endif    
    case 'e':
      if (paramValuePosition>0){ 
        setTime(atol(paramValue));
        output->println("");
      }
      else 
        output->println(now());
      break;
    case 'f':
      printFreeMemory(output);
      break;
    case 'h':
      printHelp(output);
      break;
    case 'i':
      #if defined(GAS_CTRL) || defined(PH_CTRL)
        wireInfo(output); 
      #else  //not elsif !!
        noThread(output);
      #endif
      break;
    case 'l':
    #ifdef THR_LINEAR_LOGS
      if (paramValuePosition>0) 
        printLogN(output,atol(paramValue));
      else 
        printLastLog(output);
   #else
    noThread(output);
#endif
    break;
  case 'm':
#ifdef THR_LINEAR_LOGS
    if (paramValuePosition>0) {
      long currentValueLong=atol(paramValue);
      if (( currentValueLong - nextEntryID ) < 0) {
        printLogN(output,currentValueLong);
      } 
      else {
        byte endValue=MAX_MULTI_LOG;
        if (currentValueLong > nextEntryID) 
          endValue=0;
        else if (( nextEntryID - currentValueLong ) < MAX_MULTI_LOG) 
          endValue= nextEntryID - currentValueLong;           
        for (byte i=0; i<endValue; i++) {
          currentValueLong=printLogN(output,currentValueLong)+1;
          nilThdSleepMilliseconds(25);
        }
      }
    } 
    else
      output->println(nextEntryID-1);// we will get the first and the last log ID
#else
    noThread(output);
#endif
    break;
  case 'o':
#if defined(TEMP_LIQ) || defined(TEMP_PCB)
    oneWireInfo(output);
#else
    noThread(output);
#endif
    break;
  case 'p':
    printGeneralParameters(output);
    break;
  case 'q':
    if (paramValuePosition>0) {
      setQualifier(atoi(paramValue)); 
    } 
    else {
      uint16_t a=getQualifier();
      output->println(a);
    }
    break;
  case 'r':
    if (paramValuePosition>0) {
      if (atol(paramValue)==1234) {
        resetParameters();
        output->println(F("Reset done"));
      }
    } 
    else {
      output->println(F("To reset enter r1234"));
    }
    break;
  case 's':
    printParameters(output);
    break;
  case 'w':
	processWeightCommand(data[1], paramValue, output);
    break;
  case 'z':
    getStatusEEPROM(output);
    break;
  }
  output->println("");
}

void printHelp(Print* output) {
  //return the menu
  output->println(F("(c)ompact settings"));
#ifdef THR_LINEAR_LOGS
  output->println(F("(d)elete flash"));
#endif
  output->println(F("(e)poch"));
  output->println(F("(f)ree"));
  output->println(F("(h)elp"));
  output->println(F("(i)2c"));
  output->println(F("(l)og"));
  output->println(F("(m)ultiple log"));
  output->println(F("(o)ne-wire"));
  output->println(F("(p)aram"));
  output->println(F("(q)ualifier"));
  output->println(F("(r)eset"));
  output->println(F("(s)ettings"));
  output->println(F("(w)eight"));
  output->println(F("(z) eeprom"));
}


static void printFreeMemory(Print* output)
{
  nilPrintUnusedStack(output);
}


void noThread(Print* output){
  output->println(F("No Thread"));
}


/* Fucntions to convert a number to hexadeciaml */

const char hex[] = "0123456789ABCDEF";

uint8_t toHex(Print* output, byte value) {
  output->print(hex[value>>4&15]);
  output->print(hex[value>>0&15]);
  return value;
}

uint8_t toHex(Print* output, int value) {
  byte checkDigit=toHex(output, (byte)(value>>8&255));
  checkDigit^=toHex(output, (byte)(value>>0&255));
  return checkDigit;
}

uint8_t toHex(Print* output, long value) {
  byte checkDigit=toHex(output, (int)(value>>16&65535));
  checkDigit^=toHex(output, (int)(value>>0&65535));
  return checkDigit;
}



