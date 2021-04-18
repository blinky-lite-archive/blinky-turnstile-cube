#include <OneWire.h>
#define BAUD_RATE 57600
#define CHECKSUM 64

struct TransmitData
{
  float tempA = 0.0;
  float tempB = 0.0;
  byte extraInfo[44];
};
struct ReceiveData
{
  int loopDelay = 100;
  byte extraInfo[52];
};

struct DS18B20
{
  int signalPin;
  int powerPin;
  byte chipType;
  byte address[8];
  OneWire oneWire;
  float temp = 0.0;
};
DS18B20 dS18B20_A;
DS18B20 dS18B20_B;

byte initDS18B20(byte* addr, OneWire* ow)
{
  byte type_s = 0;
  if ( !ow->search(addr)) 
  {
    ow->reset_search();
    delay(250);
    return 0;
  }
   
  // the first ROM byte indicates which chip
  switch (addr[0]) 
  {
    case 0x10:
      type_s = 1;
      break;
    case 0x28:
      type_s = 0;
      break;
    case 0x22:
      type_s = 0;
      break;
    default:
      return 0;
  } 
  return type_s;
}

float getDS18B20Temperature(OneWire* ow, byte* addr, byte chipType)
{
  byte i;
  byte data[12];
  float celsius;
  ow->reset();
  ow->select(addr);
  ow->write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(750);     // maybe 750ms is enough, maybe not
  
  ow->reset();
  ow->select(addr);    
  ow->write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) data[i] = ow->read();
  int16_t raw = (data[1] << 8) | data[0];
  if (chipType) 
  {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10)  raw = (raw & 0xFFF0) + 12 - data[6];
  }
  else 
  {
    byte cfg = (data[4] & 0x60);
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
  }
  celsius = (float)raw / 16.0;
  return celsius;
  
}

void setupPins(TransmitData* tData, ReceiveData* rData)
{
  dS18B20_A.signalPin = 5;
  dS18B20_A.powerPin = 3;
  dS18B20_B.signalPin = 9;
  dS18B20_B.powerPin = 7;
  pinMode(dS18B20_A.powerPin, OUTPUT);
  digitalWrite(dS18B20_A.powerPin, HIGH);    
  pinMode(dS18B20_B.powerPin, OUTPUT);
  digitalWrite(dS18B20_B.powerPin, HIGH);    
  dS18B20_A.oneWire = OneWire(dS18B20_A.signalPin);
  dS18B20_A.chipType = initDS18B20(dS18B20_A.address, &dS18B20_A.oneWire);
  dS18B20_B.oneWire = OneWire(dS18B20_B.signalPin);
  dS18B20_B.chipType = initDS18B20(dS18B20_B.address, &dS18B20_B.oneWire);
//  Serial.begin(9600);
}
void processNewSetting(TransmitData* tData, ReceiveData* rData, ReceiveData* newData)
{
  rData->loopDelay = newData->loopDelay;
}
boolean processData(TransmitData* tData, ReceiveData* rData)
{
  digitalWrite(dS18B20_A.powerPin, LOW); 
  digitalWrite(dS18B20_B.powerPin, LOW); 
  delay(500);
  digitalWrite(dS18B20_A.powerPin, HIGH); 
  digitalWrite(dS18B20_B.powerPin, HIGH); 
  delay(500);  

  dS18B20_A.temp = getDS18B20Temperature(&dS18B20_A.oneWire, dS18B20_A.address, dS18B20_A.chipType);
  dS18B20_B.temp = getDS18B20Temperature(&dS18B20_B.oneWire, dS18B20_B.address, dS18B20_B.chipType);
//  Serial.print(dS18B20_A.temp);
//  Serial.print(",");
//  Serial.println(dS18B20_B.temp);

  tData->tempA = dS18B20_A.temp;
  tData->tempB = dS18B20_B.temp;
  delay(rData->loopDelay);
  return true;
}


const int commLEDPin = 13;
boolean commLED = true;

struct TXinfo
{
  int cubeInit = 1;
  int newSettingDone = 0;
  int checkSum = CHECKSUM;
};
struct RXinfo
{
  int newSetting = 0;
  int checkSum = CHECKSUM;
};

struct TX
{
  TXinfo txInfo;
  TransmitData txData;
};
struct RX
{
  RXinfo rxInfo;
  ReceiveData rxData;
};
TX tx;
RX rx;
ReceiveData settingsStorage;

int sizeOfTx = 0;
int sizeOfRx = 0;

void setup()
{
  setupPins(&(tx.txData), &settingsStorage);
  pinMode(commLEDPin, OUTPUT);  
  digitalWrite(commLEDPin, commLED);

  sizeOfTx = sizeof(tx);
  sizeOfRx = sizeof(rx);
  Serial1.begin(BAUD_RATE);
  delay(1000);  
  int sizeOfextraInfo = sizeof(tx.txData.extraInfo);
  for (int ii = 0; ii < sizeOfextraInfo; ++ii) tx.txData.extraInfo[ii] = 0;

}
void loop()
{
  boolean goodData = false;
  goodData = processData(&(tx.txData), &settingsStorage);
  if (goodData)
  {
    tx.txInfo.newSettingDone = 0;
    if(Serial1.available() > 0)
    { 
      commLED = !commLED;
      digitalWrite(commLEDPin, commLED);
      Serial1.readBytes((uint8_t*)&rx, sizeOfRx);
      
      if (rx.rxInfo.checkSum == CHECKSUM)
      {
        if (rx.rxInfo.newSetting > 0)
        {
          processNewSetting(&(tx.txData), &settingsStorage, &(rx.rxData));
          tx.txInfo.newSettingDone = 1;
          tx.txInfo.cubeInit = 0;
        }
      }
      else
      {
        Serial1.end();
        for (int ii = 0; ii < 50; ++ii)
        {
          commLED = !commLED;
          digitalWrite(commLEDPin, commLED);
          delay(100);
        }

        Serial1.begin(BAUD_RATE);
        tx.txInfo.newSettingDone = 0;
        tx.txInfo.cubeInit = -1;
      }
    }
    Serial1.write((uint8_t*)&tx, sizeOfTx);
    Serial1.flush();
  }
}
