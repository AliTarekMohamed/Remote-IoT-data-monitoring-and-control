#include <avr/io.h>
#include <avr/interrupt.h>
#include <SoftwareSerial.h> // Required for communicating with ESP-01S
#include <LiquidCrystal.h>

#define LED1 2
#define LED2 3
#define MOTION_SENSOR 4
#define SMOKE_SENSOR A0
#define TEMP_SENSOR A1

LiquidCrystal lcd(12, 13, 5, 6, 7, 8); // RS, E, D4, D5, D6, D7 pins for LCD
SoftwareSerial espSerial(11,10);

int motionValue=0;
bool smokeDetected = false;
float tempValue = 0, smokeValue = 0;

void readSensors();
void sendSensorDataToLCD();
void setupWiFi();
void controlLEDs();
void delay2Seconds();
void INIT_ADC();
int Analog_Read(uint8_t pin_num);

void setup() {
  Serial.begin(115200);
  setupWiFi();
  lcd.begin(16, 2);
  pinMode(TEMP_SENSOR, INPUT);
  pinMode(SMOKE_SENSOR, INPUT);
  pinMode(MOTION_SENSOR, INPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  INIT_ADC();
  initTimer1(); 
  sei();
}

void loop() {
  Wifi_Send();   
}

ISR(TIMER1_COMPA_vect) {
  readSensors();
  sendSensorDataToLCD();
}

void Wifi_Send()
{
  if (espSerial.available())
  {
    if (espSerial.find("+IPD,"))
    {
      delay(500);

      String res = "";
      while(espSerial.available())
      {
        char c = espSerial.read();
        res += c;
      }

      String webPage = "HTTP/1.1 200 OK \r\n";
      webPage += "Content-Type: text/html\r\n\r\n";
      webPage += "<html><head><title>LED Control</title></head><body><h1>Control the LEDs</h1><button onclick=\"sendCommand('led1_on')\">LED 1 On</button><button onclick=\"sendCommand('led1_off')\">LED 1 Off</button><br><br><button onclick=\"sendCommand('led2_on')\">LED 2 On</button><button onclick=\"sendCommand('led2_off')\">LED 2 Off</button><script>function sendCommand(command) {var xhttp = new XMLHttpRequest();xhttp.open('GET', '/'+command, true);xhttp.send();}</script></body></html>";

      int id;
      String cmd;
      if(espSerial.find(">"))
      {
        if (tempValue > 30)
        {
          webPage += "<script>alert(Temperature exceeded 30 degree<br>Temperature: (${tempValue}));</script>";
        }
        if (smokeDetected)
        {
          webPage += "<script>alert(\"Smoke Detected\");</script>";
        }

        id = res[0] - 48;
        int s = webPage.length();

        cmd = "AT+CIPSEND=";
        cmd += id;
        cmd += ",";
        cmd += s;
        cmd += "\r\n";
        control(res);
        espSerial.print(cmd);
        espSerial.print(webPage);
      }
      if(espSerial.find("SEND OK"))
      {
        cmd = "AT+CIPCLOSE=";
        cmd += id;
        cmd += "\r\n"; 
        espSerial.print(cmd);
      }
    }
  }
}
void readSensors()
{
  tempValue= analogRead(TEMP_SENSOR);
  tempValue = (tempValue * 5000 / 1024) / 10;

  smokeValue = analogRead(SMOKE_SENSOR);
  if (smokeValue > 100)
  {
    smokeDetected = true;
  }
  else
  {
    smokeDetected = false;
  }

  motionValue = digitalRead(MOTION_SENSOR);
}

void sendSensorDataToLCD()
{
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print((int)tempValue);
  lcd.print("C");

  lcd.setCursor(7, 0);
  lcd.print("Smoke:");
  lcd.print((int)smokeValue);

  lcd.setCursor(0, 1);
  lcd.print("Motion:");
  lcd.print(motionValue == HIGH ? "Detected" : "None");
}
void send_cmd(String cmd)
{
  String res = "";
  espSerial.print(cmd);
  while(espSerial.available())
  {
    char c = espSerial.read();
    res += c;
  }
  Serial.println(res);
  delay(2500);
}

void control(String x) {
  if (x.indexOf("led1_on") != -1)
  {
    digitalWrite(LED1, HIGH);
  }
  else if (x.indexOf("led1_off") != -1)
  {
    digitalWrite(LED1, LOW);
  }
  else if (x.indexOf("led2_on") != -1)
  {
    digitalWrite(LED2, HIGH);
  }
  else if (x.indexOf("led2_off") != -1)
  {
    digitalWrite(LED2, LOW);
  }
}

void setupWiFi() {
  espSerial.begin(115200);
  
  //send_cmd("AT+RST\r\n");
  send_cmd("AT+UART_CUR=115200,8,1,0,0");
  send_cmd("AT+CWJAP=\"Orange-Tarek\",\"GaPhFCTh\"");
  send_cmd("AT+CIPMUX=1");
  send_cmd("AT+CWMODE=1");
  send_cmd("AT+CIFSR");
  send_cmd("AT+CIPMUX=1");
  send_cmd("AT+CIPSERVER=1,80");
}

void initTimer1() {
  TCCR1A = 0;
  TCCR1B = 0;
  OCR1A = 31250;
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS12) | (1 << CS10);
  TIMSK1 |= (1 << OCIE1A);
}

void INIT_ADC() {
    ADMUX |= (1 << REFS0);
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0) | (1 << ADEN);
}

int Analog_Read(uint8_t pin_num) {
    ADMUX |= pin_num;
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC) == 0);
    return ADC;
}