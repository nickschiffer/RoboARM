/*
  RoboARM driver code:
  Pin Mappings:
    Joystick:
      (Port) : (Function) : (Signal)
       PB5   :    X1      : Analog
       PE4   :    Y1      : Analog
       PB1   :    B2      : Digital
       PB4   :    B3      : Digital
       PB3   :    B1      : Digital
       PB2   :    B4      : Digital
    Arm:
      (Port)   : (Function)   (Color)  : (Signal)
        PE0    : Up/Down      (Black)  :   PWM
        PE5    : Forward/Back (Green)  :   PWM
        PC5    : Claw         (Yellow) :   PWM
        PC4    : Rotation     (Blue)   :   PWM

    Movement:
      Button 1: Close Claw
      Button 2: Up
      Button 3: Down
      Button 4: Open Claw

      x-axis: Rotation
      y-axis: Foward/Back

    Deadzone:
      x-avg = 1900 +/- 600
      y-avg = 1832 +/- 500

 The circuit for LCD:
 * LCD RS pin to digital pin 28 (PE2)
 * LCD Enable pin to digital pin 27  (PE1)
 * LCD D4 pin to digital pin 23 (PD0)
 * LCD D5 pin to digital pin 24 (PD1)
 * LCD D6 pin to digital pin 25 (PD2)
 * LCD D7 pin to digital pin 26 (PD3)
 * LCD R/W pin to ground
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 
 */
#include <stdio.h>
#include <Servo.h>
#include <LiquidCrystal.h>


// #define DEBUG /* uncomment for Serial Debugging Output (causes severe lag with actual movements...Prints are expensive) */

//Servo Bounds
#define CLAW_OPEN_MAX 1
#define CLAW_CLOSED_MAX 150
#define CLAW_MOVEMENT_SPEED 5

#define ROTATE_LEFT_MAX 0
#define ROTATE_RIGHT_MAX 179
#define ROTATION_SPEED 2


#define UP_MAX 120
#define DOWN_MAX 74
#define UPDOWN_MOVEMENT_SPEED 2

#define DOWN_MAX_BACK 74
#define DOWN_MAX_FORWARD 0
int   downmax_height = 0;
int   downmax_width  = 0;
float downmax_slope  = 0;

#define FORWARD_MAX 179
#define BACK_MAX 89
#define FORWARD_BACK_SPEED 1
#define SENSITIVE_OFFSET 20

//Joystick Calibration
#define X_START 1900 //X Middle Point
#define Y_START 1832 //Y Middle Point
#define X_RANGE 600  //X Deadzone Threshold
#define Y_RANGE 500  //Y Deadzone Threshold

//recording
//#define RECORD_LENGTH (BUFSIZ + 500)
//#define RECORD_LENGTH (3*BUFSIZ + 500)
#define RECORD_LENGTH (6*BUFSIZ + 600)
int record_length =  RECORD_LENGTH;
//int record[RECORD_LENGTH][4];
//unsigned short int record[RECORD_LENGTH][4];
unsigned char record[RECORD_LENGTH][4];
int count = 0; //current recording position
bool recording = false;
bool playing   = false;
int cursor = 0;//current playback position

int progress_cursor = 0; // used for progress bar for recording and playback

//Inputs (used to store input values)
int x_val = 0;
int y_val = 0;

int B1_val = 0;
int B2_val = 0;
int B3_val = 0;
int B4_val = 0;


/* Analog */
int X1 = PB_5;
int Y1 = PE_4;
/* Digital */
int B2_pin = PB_1;
int B3_pin = PB_4;
int B1_pin = PB_3;
int B4_pin = PB_2; 

/* Servos */
int UpDown_pin      = PE_0;
int ForwardBack_pin = PE_5;
int Claw_pin        = PC_5;
int Rotate_pin      = PC_4;

/* Positional Variables */
int ClawPosition        = 0;
int RotationPosition    = 0;
int UpDownPosition      = 0;
int ForwardBackPosition = 0;

char str[200];

int current_down_max = 0;
bool adjusting_back  = false;

//Set up Servos
Servo UpDown;
Servo ForwardBack;
Servo Claw;
Servo Rotate;

// initialize the LCD library with the numbers of the interface pins (see mapping at top of file)
LiquidCrystal lcd(28, 27, 23, 24, 25, 26);

void moveClaw(char OpenOrClose){
  
  switch (OpenOrClose){
    case 'o' :
        if (ClawPosition > CLAW_OPEN_MAX + CLAW_MOVEMENT_SPEED){
            ClawPosition -= CLAW_MOVEMENT_SPEED;
            Claw.write(ClawPosition);
            #ifdef DEBUG
            sprintf(str, "Opening Claw. Claw Position: %d\n", ClawPosition);
            Serial.print(str);
            #endif
            
        }
        break;
    case 'c' :
        if (ClawPosition < CLAW_CLOSED_MAX - CLAW_MOVEMENT_SPEED){
            ClawPosition += CLAW_MOVEMENT_SPEED;
            Claw.write(ClawPosition);
            #ifdef DEBUG
            sprintf(str, "Closing Claw. Claw Position: %d\n", ClawPosition);
            Serial.print(str);
            #endif
        }
        break;
    case 'i':
        int default_val;
        default_val = (CLAW_CLOSED_MAX - CLAW_OPEN_MAX)/2;
        ClawPosition = default_val;
        Claw.write(default_val);
        #ifdef DEBUG
        sprintf(str, "Initializing CLaw. Claw Position: %d\n", ClawPosition);
        Serial.print(str);
        #endif
        break;
    default:
        break;
  }
}

void moveUpDown(char UpOrDown){
  
  switch (UpOrDown){
    case 'u' : //move up
        if (UpDownPosition + UPDOWN_MOVEMENT_SPEED < UP_MAX ){
            UpDownPosition += UPDOWN_MOVEMENT_SPEED;
            UpDown.write(UpDownPosition);
            #ifdef DEBUG
            sprintf(str, "Moving Up. UpDown Position: %d\n", UpDownPosition);
            Serial.print(str);
            #endif
            
        }
        break;
    case 'd' : //move down
            if (!adjusting_back){
              calcDownMax();
            }
        if (UpDownPosition - UPDOWN_MOVEMENT_SPEED > current_down_max){
            UpDownPosition -= UPDOWN_MOVEMENT_SPEED;
            UpDown.write(UpDownPosition);
            #ifdef DEBUG
            sprintf(str, "Moving Down. UpDown Position: %d, Current DownMax: %d\n", UpDownPosition, current_down_max);
            Serial.print(str);
            #endif
        }
        break;
    case 'i': //initialize
        int default_val;
        //default_val = abs(UP_MAX - DOWN_MAX)/2;
        default_val = 100;
        UpDown.write(default_val);
        UpDownPosition = default_val;
        #ifdef DEBUG
        sprintf(str, "Initializing UpDown. UpDown Position: %d\n", UpDownPosition);
        Serial.print(str);
        #endif
        break;
    default:
        break;
  }
}

void moveArmRotate(){
    //Rotate
    if (x_val > X_START + X_RANGE){ //Move if input value is outside of middle deadzone
      //Rotate Left
      if (ForwardBackPosition >= (FORWARD_MAX - SENSITIVE_OFFSET)){ //If close to the max front position, move more slowly
        if (RotationPosition - (ROTATION_SPEED - 1) >= ROTATE_LEFT_MAX){
          RotationPosition -= (ROTATION_SPEED - 1);
          Serial.println("Sensitive Left");
          Rotate.write(RotationPosition);
          #ifdef DEBUG
          sprintf(str, "Rotating Left. Rotation Position: %d\n", RotationPosition);
          Serial.print(str);
          #endif
        }
      }
      
      else if (RotationPosition - ROTATION_SPEED >= ROTATE_LEFT_MAX){ //check to see if movement will go out of bounds
        RotationPosition -= ROTATION_SPEED;
        Rotate.write(RotationPosition);
        #ifdef DEBUG
        sprintf(str, "Rotating Left. Rotation Position: %d\n", RotationPosition);
        Serial.print(str);
        #endif
      }
    }
    if (x_val < X_START - X_RANGE){ //Move if input value is outside of middle deadzone
      //Rotate Right
      if (ForwardBackPosition >= (FORWARD_MAX - SENSITIVE_OFFSET)){ //If close to the max front position, move more slowly
        if (RotationPosition + (ROTATION_SPEED - 1) <= ROTATE_RIGHT_MAX){ //check to see if movement will go out of bounds
          RotationPosition += (ROTATION_SPEED - 1);
          Serial.println("Sensitive Right");
          Rotate.write(RotationPosition);
          #ifdef DEBUG
          sprintf(str, "Rotating Right. Rotation Position: %d\n", RotationPosition);
          Serial.print(str);
          #endif
        }
      }
      else if (RotationPosition + ROTATION_SPEED <= ROTATE_RIGHT_MAX){ //check to see if movement will go out of bounds
        RotationPosition += ROTATION_SPEED;
        Rotate.write(RotationPosition);
        #ifdef DEBUG
        sprintf(str, "Rotating Right. Rotation Position: %d\n", RotationPosition);
        Serial.print(str);
        #endif
      }
    }
}

void moveArmForwardBack(){
  //Forward or Back
    if (y_val > Y_START + Y_RANGE){ //Move if input value is outside of middle deadzone
      //Move Forward
      if (ForwardBackPosition + FORWARD_BACK_SPEED <= FORWARD_MAX){ //check to see if movement will go out of bounds
        ForwardBackPosition += FORWARD_BACK_SPEED;
        ForwardBack.write(ForwardBackPosition);
        #ifdef DEBUG
        sprintf(str, "Moving Forward. ForwardBack Position: %d\n", ForwardBackPosition);
        Serial.print(str);
        #endif
      }
    }
    if (y_val < Y_START - Y_RANGE){ //Move if input value is outside of middle deadzone
      //Move Back
      if (ForwardBackPosition - FORWARD_BACK_SPEED >= BACK_MAX){ //check to see if movement will go out of bounds
        
        ForwardBackPosition -= FORWARD_BACK_SPEED;
        ForwardBack.write(ForwardBackPosition);
        #ifdef DEBUG
        sprintf(str, "Moving Back. ForwardBack Position: %d\n", ForwardBackPosition);
        Serial.print(str);
        #endif
        calcDownMax();
        /* If the arm is moved back and is lower than the lowest allowed position for that ForwardBackPosition, we need to move the arm up.*/
        if (UpDownPosition < current_down_max){
          #ifdef DEBUG
          Serial.println("adjusting\n");
          #endif
          adjusting_back = true;
          moveUpDown('u');
          adjusting_back = false;
        }
      }
    }
  
}

/* Used to calculate the lowest point the arm can go depending on its current ForwardBackPosition (Think Triangle, y=mx+b) */
void calcDownMax(){
  current_down_max = (int)((float)downmax_slope*(float)abs(FORWARD_MAX - ForwardBackPosition));
  #ifdef DEBUG
  sprintf(str, "height: %d, width: %d, slope: %g, currentdownmax: %d\n", downmax_height, downmax_width, downmax_slope, current_down_max);
  Serial.print(str);
  #endif
  
}

/* Prints the Welcome Message when the bot is powered on or reset */
void printWelcome(){
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("   Welcome to");
  delay(2000);
  lcd.setCursor(0, 1);
  lcd.print("    RoboARM!");
  delay(2000);
  lcd.setCursor(0,0);
  lcd.print("                ");
  lcd.setCursor(0,1);
  lcd.print("   Welcome to");
  delay(750);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Initializing");
  int lcd_cursor = 12;
  for (int i = 0; i < 3; i++){
    for (int j = 0; j < 3; j++){
      lcd.setCursor(lcd_cursor++, 0);
      lcd.print(".");
      delay(600);
    }
    lcd.setCursor(12,0);
    lcd.print("   ");
    lcd_cursor = 12;
    delay(500);
  }
  byte block[8] = {
    0b00000,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b00000  
  };
  lcd.createChar(0, block);
  byte play[8] = {
    0b01000,
    0b01100,
    0b01110,
    0b01111,
    0b01110,
    0b01100,
    0b01000,
    0b00000
  };
  lcd.createChar(1, play);
  byte rec[8] = {
    0b00000,
    0b01110,
    0b11111,
    0b11111,
    0b11111,
    0b01110,
    0b00000,
    0b00000
  };
  lcd.createChar(2, rec);
}

void setup() {
  
  
  
  Serial.begin(9600);
  Serial.print("Starting\n");
  printWelcome();
  lcd.setCursor(0,0);
  lcd.print("Mode: Manual");
  //Setup digital inputs
  pinMode(B1_pin,INPUT);
  pinMode(B2_pin,INPUT);
  pinMode(B3_pin,INPUT);
  pinMode(B4_pin,INPUT);
  pinMode(PUSH1, INPUT_PULLUP);
  pinMode(PUSH2, INPUT_PULLUP);
  
  //recording and playback Interrupt Initialization
  attachInterrupt(digitalPinToInterrupt(PUSH1), toggleRecord, FALLING);
  attachInterrupt(digitalPinToInterrupt(PUSH2), togglePlayback, FALLING);

  //Attach Servos
  UpDown.attach(UpDown_pin);
  ForwardBack.attach(ForwardBack_pin);
  Claw.attach(Claw_pin);
  Rotate.attach(Rotate_pin);

  //Set Servos to Default
  moveClaw('i');
  moveUpDown('i');

  //intitialize Rotation to middle point of bounds
  RotationPosition = abs(ROTATE_LEFT_MAX - ROTATE_RIGHT_MAX)/2;
  Rotate.write(RotationPosition);
  sprintf(str, "Initializing Rotate. RotationPosition: %d\n", RotationPosition);
  Serial.print(str);

  //Initialize ForwardBack
  //ForwardBackPosition = abs(FORWARD_MAX - BACK_MAX)/2;
  ForwardBackPosition = 100;
  ForwardBack.write(ForwardBackPosition);
  sprintf(str, "Initializing ForwardBack. ForwardBackPosition: %d\n", ForwardBackPosition);
  Serial.print(str);


  downmax_height = abs(DOWN_MAX_BACK - DOWN_MAX_FORWARD);
  downmax_width  = abs(FORWARD_MAX - BACK_MAX);
  downmax_slope  = (float)downmax_height/(float)downmax_width;
  calcDownMax();
  
}


/* Recording is toggled via hardware interrupt if record button is pressed */
void toggleRecord(){
  
  if (recording){
    #ifdef DEBUG
    sprintf(str, "Stopping Recording... Length: %d\n", count);
    Serial.print(str);
    #endif
    lcd.setCursor(6,0);
    lcd.print("Manual    ");
    recording = false;
    lcd.setCursor(0,1);
    lcd.print("                ");
    progress_cursor = 0;
  }
  else{
    #ifdef DEBUG
    Serial.println("Starting Recording...");
    #endif
    lcd.setCursor(6,0);
    lcd.print("Record ");
    lcd.write((byte) 2);
    recording = true;
    count = 0;
  }
  
  
  
}

/* Playback is toggled via hardware interrupt if playback button is pressed */
void togglePlayback(){
  
  if (recording) //If recording, the recording needs to be concluded before playback can occur
      recording = false;
      lcd.setCursor(0,1);
      lcd.print("                ");
      progress_cursor = 0;
  if (!playing){
    if (count != 0){
      #ifdef DEBUG
      Serial.println("Playing");
      #endif
      lcd.setCursor(6,0);
      lcd.print("Playback ");
      lcd.write((byte) 1);
      playing = true;
      cursor = 0;
      }
      else {
        #ifdef DEBUG
        Serial.println("No recording to play back");
        #endif
      }
    }
    else{
      playing = false;
      #ifdef DEBUG
      Serial.println("Playback Finished");
      #endif
      lcd.setCursor(0,1);
      lcd.print("    Finished");
      delay(1000);
      lcd.setCursor(0,1);
      lcd.print("                ");
      lcd.setCursor(6,0);
      lcd.print("        ");
      lcd.setCursor(6,0);
      
      lcd.print("Manual    ");
      cursor = 0;
    }
  
}


void loop() {
   
  // read the values from the sensor if playback is not active:
  if (!playing){
    x_val = analogRead(X1);
    y_val = analogRead(Y1);   

    B1_val = digitalRead(B1_pin);
    B2_val = digitalRead(B2_pin);  
    B3_val = digitalRead(B3_pin);  
    B4_val = digitalRead(B4_pin);

    /* Claw */
   if (B1_val == HIGH){
      moveClaw('c'); //close claw
   }
   else if (B4_val == HIGH){
      moveClaw('o'); //open claw
   }

    if (B2_val == HIGH){
      moveUpDown('u'); //move up
    }
    else if (B3_val == HIGH){
      moveUpDown('d'); //move down
    }

    moveArmRotate();
    moveArmForwardBack();
    
    /* If recording and still have room to record, store current positions into record array */
    if (recording){
        if (count != record_length - 1){
          #ifdef DEBUG
          Serial.print("count:");
          Serial.println(count);
          #endif 
          record[count][0] = ClawPosition;
          record[count][1] = RotationPosition;
          record[count][2] = UpDownPosition;
          record[count][3] = ForwardBackPosition;
          count++;

          /* Map recording position to [0 : 16] and print status bar accordingly */
          int progress_block = map(count, 0, record_length, 0, 17);
          if (progress_block > progress_cursor){
            lcd.setCursor(progress_cursor, 1);
            lcd.write((byte) 0);
            progress_cursor = progress_block; 
          }
      }
      else {
        toggleRecord();
      }
    }
  }
  else {
    /* If Playing Back, write recorded position values directly to Servos */
    Claw.write(record[cursor][0]);
    Rotate.write(record[cursor][1]);
    UpDown.write(record[cursor][2]);
    ForwardBack.write(record[cursor][3]);

    /* Map current playback position out of record length into 0 : 16. Then write to status bar */
    int progress_block = map(cursor, 0, count, 0, 17);
    if (progress_block > progress_cursor){
      lcd.setCursor(progress_cursor, 1);
      lcd.write((byte) 0);
      progress_cursor = progress_block; 
    }
    if (cursor < count - 1){
      cursor++;
    }
    else {
      togglePlayback(); //stop playback once it has reached the end
    }
  }
   delay(20);
}
