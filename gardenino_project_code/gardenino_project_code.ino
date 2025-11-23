#include <LiquidCrystal_I2C.h>
#include <Wire.h>              
#include <IRremote.hpp>       
#include <string.h>
#include <DHT.h>
#include<Servo.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); 
Servo Servo1;

#define echoPin1 2
#define trigPin1 3
#define echoPin2 4
#define trigPin2 5

int DC1 = 6;  
int DC2 = 7;   
int DC3 = 8;  
int DC4 = 9;
int RemotePin = 10; 
int servoPin = 11;
int relay_pin = 12; 

int MOISTURE_PIN = A2;
#define DHT_PIN A3

#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);
int temperature=0;
int humidity=0;
unsigned long lastDHT = 0;
const unsigned long DHT_INTERVAL = 2000; 

int moisture_percent=0;
int soil_moisture = 0; 
int dry_value = 940;   
int wet_value = 300;   

long lastCommand = 0;
unsigned long lastSignalTime = 0;
const unsigned long RELEASE_TIMEOUT = 100; 

int distance1=0;
int distance2=0;

String lcd_message;  

enum BUTTON_PRESSED{
  NUM_ONE = 0xC,
  NUM_TWO = 0x18,
  NUM_THREE = 0x5E,
  NUM_FOUR = 0x8,
  NUM_FIVE = 0x1C,
  NUM_SIX = 0x5A,
  NUM_SEVEN = 0x42,
  NUM_EIGHT = 0x52,
  NUM_NINE=0x4a
};

void setup() {
  Serial.begin(9600);
  Servo1.attach(servoPin);
  IrReceiver.begin(RemotePin); 
  dht.begin();
  lcd.init();
  lcd.backlight();
  

  pinMode(relay_pin,OUTPUT);
  digitalWrite(relay_pin, HIGH);

  pinMode(DC1, OUTPUT);
  pinMode(DC2, OUTPUT);
  pinMode(DC3, OUTPUT);
  pinMode(DC4, OUTPUT);

  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);

  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);

  lcd_message = "Welcome farmer";
  print_message(lcd_message);
}

long lastLcdCommand = -1; 

void loop() {
    input();
    detect_obstacle();

    unsigned long now = millis();
    if (now - lastDHT >= DHT_INTERVAL) {
        lastDHT = now;
        SOIL_MOISTURE_READ();
        DHT_READ();

        if (lastCommand != lastLcdCommand) {
            lastLcdCommand = lastCommand;

            lcd.clear(); 
            if (lastCommand==NUM_ONE){
                DISPLAY_READINGS("Temperature: ", temperature, 0);
                DISPLAY_READINGS("Humidity: ", humidity, 1);
            } else if(lastCommand==NUM_THREE){
                DISPLAY_READINGS("Moisture: ",moisture_percent,0);
            } else if(lastCommand==NUM_SEVEN){
                lcd_message = "Watering";
                print_message(lcd_message);
                digitalWrite(relay_pin, LOW);
            } else{
                lcd_message = "Welcome farmer";
                print_message(lcd_message);
                digitalWrite(relay_pin, HIGH);
            }
        }
    }
}


void SOIL_MOISTURE_READ(){
  soil_moisture = analogRead(MOISTURE_PIN);
  moisture_percent = map(soil_moisture, dry_value, wet_value, 0, 100);
  if (moisture_percent < 0) moisture_percent = 0;
  if (moisture_percent > 100) moisture_percent = 100;
}

void DHT_READ(){
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
}

void DISPLAY_READINGS(String text, int reading,int cursor){
  lcd.setCursor(0, cursor);
  lcd.print(text);
  lcd.print(reading);
}

void print_message(String message) {
  lcd.clear();
  if (message.length() <= 16) {
    lcd.print(message);
  } else {
    delay(50);
    for (int i = 0; i <= message.length() - 16; i++) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(message.substring(i, i + 16));
      delay(300); 
    }
    delay(700);
  }
}

void detect_obstacle(){
  distance1 = measureDistance(trigPin1, echoPin1); 
  distance2 = measureDistance(trigPin2, echoPin2); 

  if ((lastCommand == NUM_TWO || lastCommand == 0xFFFFFFFF) && distance2 < 30) {
    move("stop");
  }

  if ((lastCommand == NUM_EIGHT || lastCommand == 0xFFFFFFFF) && distance1 < 30) {
    move("stop");
  }
}

void input() {
  if (IrReceiver.decode()) { 
    long value = IrReceiver.decodedIRData.command;
    Serial.println(value, HEX);
    
    if (value == 0xFFFFFFFF) {
      value = lastCommand; 
    } else if (value != 0) {
      lastCommand = value; 
    }
    
    if (value != 0) { 
      switch(value){
        case NUM_ONE:
          Serial.print("TEST");
          break;

        case NUM_TWO: 
          move("forward");
          break;

        case NUM_EIGHT:
          move("backward");
          break;

        case NUM_FOUR: 
          move("left"); 
          break;

        case NUM_SIX: 
          move("right"); 
          break;

        case NUM_NINE: 
          seed();
          break;

        default: 
          move("stop"); 
          break;
      }
    }
    
    lastSignalTime = millis(); 
    IrReceiver.resume();
  }
  
  if (millis() - lastSignalTime > RELEASE_TIMEOUT) {
    move("stop");
    lastCommand = 0;
  }
  
}

void move(String dir) {
  if (dir == "forward") {
    digitalWrite(DC1, HIGH);  digitalWrite(DC2, LOW);
    digitalWrite(DC3, HIGH);  digitalWrite(DC4, LOW);
  } 
  else if (dir == "backward") {
    digitalWrite(DC1, LOW);  digitalWrite(DC2, HIGH);
    digitalWrite(DC3, LOW);  digitalWrite(DC4, HIGH);
  } 
  else if (dir == "left") {
    digitalWrite(DC1, LOW);  digitalWrite(DC2, HIGH);  
    digitalWrite(DC3, HIGH); digitalWrite(DC4, LOW);   
  } 
  else if (dir == "right") {
    digitalWrite(DC1, HIGH); digitalWrite(DC2, LOW);   
    digitalWrite(DC3, LOW);  digitalWrite(DC4, HIGH);  
  }
  else{
    digitalWrite(DC1, LOW); digitalWrite(DC2, LOW);
    digitalWrite(DC3, LOW); digitalWrite(DC4, LOW);
  }
}

void seed(){
  Servo1.write(170);  
  delay(200);         
  Servo1.write(100);   
  delay(1000);  
  Servo1.write(170);  

}


long measureDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  return duration * 0.034 / 2;
}
