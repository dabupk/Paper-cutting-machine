#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);

//declaring pins
const int right_limit_pin = 2;
const int left_limit_pin = 3;
const int clk_pin = 4;
const int dir_pin = 5;
const int set_pin = 6;
const int en_pin = 7;
const int paper_pin = 8;
const int rpwm_pin = 9;
const int lpwm_pin = 10;
const int plus_pin = 11;
const int minus_pin = 12;
const int fan_pin = 13;

//declaring variables
short slider_speed;
short roll_speed;
short factor;
short refresh_rate;
short value_increment_speed;
short screen;
long value;
long inch;
long repeat;
long previous_inch;
long previous_repeat;
long steps;
byte dot;
byte inch_confirmed;
byte repeat_confirm;
byte minus_pin_active;
byte plus_pin_active;
byte set_state;
byte screen_changed;
volatile byte slider_position;
const short total_screens = 4;
const short button_speed = 3;
const short inter_delay = 1000;

void setup() {
  // put your setup code here, to run once:
  //configuring gpios
  pinMode(left_limit_pin,INPUT_PULLUP);
  pinMode(right_limit_pin,INPUT_PULLUP);
  pinMode(clk_pin,OUTPUT);
  pinMode(dir_pin,OUTPUT);
  pinMode(set_pin,INPUT_PULLUP);
  pinMode(en_pin,OUTPUT);
  pinMode(paper_pin,INPUT_PULLUP);
  pinMode(lpwm_pin,OUTPUT);
  pinMode(rpwm_pin,OUTPUT);
  pinMode(plus_pin,INPUT_PULLUP);
  pinMode(minus_pin,INPUT_PULLUP);
  pinMode(fan_pin,OUTPUT);

  //preparing board
  digitalWrite(fan_pin,LOW);
  digitalWrite(dir_pin,LOW);
  digitalWrite(en_pin,HIGH);
  Serial.begin(9600);
  lcd.begin();
  lcd.backlight();
  attachInterrupt(digitalPinToInterrupt(left_limit_pin), left, FALLING);
  attachInterrupt(digitalPinToInterrupt(right_limit_pin), right, FALLING);

  //intializing machine
  factory_restore();
  get_values();
  intialize();   
  check_paper();
}


void loop() {
  // put your main code here, to run repeatedly:
  data(); // get data from user
  for (int j=0;j<repeat;j++){
  if(digitalRead(paper_pin) == LOW){  //protocol for when machine is out of paper
 previous_inch = inch;    
 previous_repeat =  repeat+1;  //add one more loop to compensate for wasted paper
 check_paper();
 inch = previous_inch;
 repeat = previous_repeat;
    }
  roll();
  slide();
    }  
}

void intialize(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Intializing");
  slider_position = false;
  if (digitalRead(right_limit_pin) == LOW){
    slider_position = true;
  }
    while (slider_position == false){
      analogWrite(lpwm_pin,slider_speed);
      analogWrite(rpwm_pin,0);
      }
      analogWrite(lpwm_pin,0);
      analogWrite(rpwm_pin,0);
}

// get values from eemprom memory
void get_values(){
  slider_speed = EEPROMReadlong(0);
  roll_speed   = EEPROMReadlong(10);
  factor       = EEPROMReadlong(20);
}

void factory_restore(){
  EEPROMWritelong(0,100);
  EEPROMWritelong(10,800);
  EEPROMWritelong(20,306);
}

void check_paper(){ 
     // wait until paper is feeded
  while (digitalRead(paper_pin)== LOW){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Feed paper !");
  delay(500);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Feed paper ");
  delay(500);
    }
    
    //cut a little amount of paper
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Preparing ");
  delay(inter_delay);
  inch =1;
  repeat = 0;
  roll();
  slide();
  inch = previous_inch;
  repeat = previous_repeat;
  }

void data(){
  //setting all values to their intial state
   inch =1;
   repeat = 1;
   screen=1;
   inch_confirmed=false;
   repeat_confirm=false;
   plus_pin_active = false;
   minus_pin_active = false;
   set_state = false;
   screen_changed = false;
   value=0;

   //dont run machine until value for reapeat(loop) is set
   while(repeat_confirm == false){
    screen_changed = false;
    while (digitalRead(set_pin) == LOW){  //if set button if pressed take coresponding action
    set_state = true;
  if(screen_changed == true){
    set_state = false;
  }
      if(digitalRead(plus_pin) == LOW){
      plus_pin_active = true;
    }
    else if(digitalRead(minus_pin) == LOW){
      minus_pin_active = true;
    }
     
    if(plus_pin_active == true && digitalRead(plus_pin) == HIGH){
      plus_pin_active = false;
      screen++;
      screen_changed =true;
             if(screen > total_screens){
               screen = 1;
            }
    }
  
   else if(minus_pin_active == true && digitalRead(minus_pin) == HIGH){
      minus_pin_active = false;
      screen--;
      screen_changed =true;
             if(screen < 1){
               screen = total_screens;
            }
       }    
   }
    if(digitalRead(plus_pin) == LOW){
    value++;
    value_increment_speed -= button_speed;
   } 
    else if (digitalRead(minus_pin) == LOW){
  value--;
  value_increment_speed -= button_speed;
      }
    else {
      value_increment_speed = 100;
    }
    value = constrain(value,0,2000000000);
    value_increment_speed = constrain(value_increment_speed,0,5000);
    screens();
    delay(value_increment_speed);
   }
}

//fucntion for selecting differnet screens
void screens(){
   if(screen == 1){ //input data screen
  screen_1(value);
   }
   else if(screen == 2){ //blade speed screen
            screen_2(value);
    }
    else if(screen == 3){ //slider speed screen
            screen_3(value);
    }
    else if(screen == 4){ //factor screen
            screen_4(value);
    }
}

//main screen for getting size in inches and number of loops to repeat the cycle
void screen_1(long my_value){
  if(inch_confirmed == false){
    if(inch != my_value  || refresh_rate  > 5){
      refresh_rate =0;
      inch = my_value;
            lcd.clear();
            lcd.setCursor(0, 1);
            lcd.print("Repeat :");
            lcd.setCursor(9, 1);
            lcd.print(repeat);
            lcd.setCursor(0, 0);
              if(dot == true){
                dot = false;
                lcd.print("Inch :");
              }
              else{
              dot = true;
              lcd.print("Inch ");
              }
                lcd.setCursor(9, 0);
                lcd.print(inch);
                }
        if(set_state == true){
          set_state = false;
           inch_confirmed = true;
            value =1;
            delay(500);
        }
   }
    
  else {
    if(repeat != my_value  || refresh_rate  > 5){
      refresh_rate =0;
      repeat = my_value;
            lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Inch :");
                lcd.setCursor(9, 0);
                lcd.print(inch);
                lcd.setCursor(0, 1);
              if(dot == true){
                dot = false;
                lcd.print("Repeat :");
              }
              else{
              dot = true;
              lcd.print("Repeat ");
              }
                lcd.setCursor(9, 1);
                lcd.print(repeat);
                } 
                if(set_state == true){
                  set_state = false;
                    repeat_confirm = true;
                        }
       }
       refresh_rate++;      
}

//screen for setting blade speed
void screen_2(long my_value){
   if(slider_speed != my_value  || refresh_rate  > 5){
      refresh_rate =0;
      slider_speed = my_value;
            lcd.clear();
            lcd.setCursor(0, 0);
              if(dot == true){
                dot = false;
                lcd.print("Blade speed :");
              }
              else{
              dot = true;
              lcd.print("Blade speed  ");
              }
            lcd.setCursor(0, 1);
            lcd.print(slider_speed);
                }
                if(set_state == true){
           EEPROMWritelong(0,slider_speed);
           get_values();
          set_state = false;
           screen =1;
            value =1;
            delay(500);
        }
        refresh_rate++; 
}

//screen for roller speed
void screen_3(long my_value){
  if(roll_speed != my_value  || refresh_rate  > 5){
      refresh_rate =0;
      roll_speed = my_value;
            lcd.clear();
            lcd.setCursor(0, 0);
              if(dot == true){
                dot = false;
                lcd.print("Roller speed :");
              }
              else{
              dot = true;
              lcd.print("Roller speed  ");
              }
            lcd.setCursor(0, 1);
            lcd.print(roll_speed);
                }
                if(set_state == true){
                  EEPROMWritelong(10,roll_speed);
                  get_values();
          set_state = false;
           screen =1;
            value =1;
            delay(500);
        }
        refresh_rate++; 
}

//screen for multiplying factor for converting steps into inch
void screen_4(long my_value){
   if(factor != my_value  || refresh_rate  > 5){
      refresh_rate =0;
      factor = my_value;
            lcd.clear();
            lcd.setCursor(0, 0);
              if(dot == true){
                dot = false;
                lcd.print("Factor :");
              }
              else{
              dot = true;
              lcd.print("Factor ");
              }
            lcd.setCursor(0, 1);
            lcd.print(factor);
                }
                if(set_state == true){
                  EEPROMWritelong(30,roll_speed);
                  get_values();
          set_state = false;
           screen =1;
            value =1;
            delay(500);
        }
        refresh_rate++; 
}

//runs the roller
void roll(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Rolling ");
  delay(inter_delay);
  steps=inch*factor;
  digitalWrite(en_pin,LOW);
  for(int i=0;i<steps;i++){
 digitalWrite(clk_pin,HIGH);
 delayMicroseconds(roll_speed);
 digitalWrite(clk_pin,LOW);
 delayMicroseconds(roll_speed);
    }
    digitalWrite(en_pin,HIGH); 
}

// runs the blade
void slide(){
  digitalWrite(fan_pin,HIGH);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Cutting ");
  delay(inter_delay);
  if(digitalRead(right_limit_pin) == LOW){
    slider_position = true;
    }
    else if(digitalRead(left_limit_pin) == LOW){
     slider_position = false;
    }
  while (slider_position == true){
    analogWrite(lpwm_pin,0);
    analogWrite(rpwm_pin,slider_speed);
  }
  analogWrite(lpwm_pin,0);
  analogWrite(rpwm_pin,0);
  delay(inter_delay);
  while (slider_position == false){
    analogWrite(lpwm_pin,slider_speed);
    analogWrite(rpwm_pin,0);
  }
  analogWrite(lpwm_pin,0);
  analogWrite(rpwm_pin,0);
  delay(inter_delay);
  digitalWrite(fan_pin,LOW);
}

//ISR for left limit switch
void left(){
    analogWrite(lpwm_pin,0);
    analogWrite(rpwm_pin,0);
    slider_position =false;
}

//ISR for right limit switch
void right(){
     analogWrite(lpwm_pin,0);
     analogWrite(rpwm_pin,0);
     slider_position = true;
}

//writes long variable consisting of 4 bytes to EEPROM
void EEPROMWritelong(int address, long value){

  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);
  
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}

//reads long variable consisting of 4 bytes from EEPROM
long EEPROMReadlong(long address){
    long four = EEPROM.read(address);
    long three = EEPROM.read(address + 1);
    long two = EEPROM.read(address + 2);
    long one = EEPROM.read(address + 3);

    return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}
