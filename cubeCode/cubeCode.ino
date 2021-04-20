#define BAUD_RATE 57600
#define CHECKSUM 64
const int bar1Pin = 3;
const int bar2Pin = 5;
const int bar3Pin = 7;
const int bar4Pin = 9;

struct TransmitData
{
  int bar1Val = 0;
  int bar2Val = 0;
  int bar3Val = 0;
  int bar4Val = 0;
  byte extraInfo[36];
};
struct ReceiveData
{
  int loopDelay = 1000;
  byte extraInfo[52];
};

void setupPins(TransmitData* tData, ReceiveData* rData)
{
  pinMode(bar1Pin, INPUT);
  pinMode(bar2Pin, INPUT);
  pinMode(bar3Pin, INPUT);
  pinMode(bar4Pin, INPUT);
//  Serial.begin(9600);
}
void processNewSetting(TransmitData* tData, ReceiveData* rData, ReceiveData* newData)
{
  rData->loopDelay = newData->loopDelay;
}
boolean processData(TransmitData* tData, ReceiveData* rData)
{
  tData->bar1Val = digitalRead(bar1Pin);
  tData->bar2Val = digitalRead(bar2Pin);
  tData->bar3Val = digitalRead(bar3Pin);
  tData->bar4Val = digitalRead(bar4Pin);
/*
  Serial.print(tData->bar1Val);
  Serial.print(", ");
  Serial.print(tData->bar2Val);
  Serial.print(", ");
  Serial.print(tData->bar3Val);
  Serial.print(", ");
  Serial.println(tData->bar4Val);
*/
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
