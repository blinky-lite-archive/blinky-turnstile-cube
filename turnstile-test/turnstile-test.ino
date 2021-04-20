const int bar1Pin = 3;
const int bar2Pin = 5;
const int bar3Pin = 7;
const int bar4Pin = 9;
int bar1Val = 0;
int bar2Val = 0;
int bar3Val = 0;
int bar4Val = 0;

void setup() 
{
  pinMode(bar1Pin, INPUT);
  pinMode(bar2Pin, INPUT);
  pinMode(bar3Pin, INPUT);
  pinMode(bar4Pin, INPUT);
  Serial.begin(9600);
}

void loop() 
{
  bar1Val = digitalRead(bar1Pin);
  bar2Val = digitalRead(bar2Pin);
  bar3Val = digitalRead(bar3Pin);
  bar4Val = digitalRead(bar4Pin);
  Serial.print(bar1Val);
  Serial.print(", ");
  Serial.print(bar2Val);
  Serial.print(", ");
  Serial.print(bar3Val);
  Serial.print(", ");
  Serial.println(bar4Val);
  delay(1000);
}
