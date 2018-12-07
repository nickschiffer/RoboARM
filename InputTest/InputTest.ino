/*
  Analog Input
 Used to Calibration/Statitics for analog inputs from joystick.
 
 */
#include <stdio.h>

 
int x_val = 0;
int y_val = 0;

int B1_val = 0;
int B2_val = 0;
int B3_val = 0;
int B4_val = 0;

//For delta tuning
bool firstpass = true;
int firstpass_x = 0;
int firstpass_y = 0;
int x_delta_high_mid = 0;
int x_delta_low_mid = 0;
int y_delta_high_mid = 0;
int y_delta_low_mid = 0;


/* Analog */
int X1 = PB_5;
int Y1 = PD_2;
/* Digital */
int B2_pin = PB_1;
int B3_pin = PB_4;
int B1_pin = PB_3;
int B4_pin = PB_2;

 //Average
 long long sum_x = 0;
 long long sum_y = 0;
 long count = 0;
 int avg_x, avg_y;

char str[2048];

void setup() {
  pinMode(B1_pin,INPUT);
  pinMode(B2_pin,INPUT);
  pinMode(B3_pin,INPUT);
  pinMode(B4_pin,INPUT);  

  Serial.begin(9600);
  
  
}

void loop() {

  // get middle values for x and y (for tuning)
  if (firstpass){
    firstpass_x = analogRead(X1);
    firstpass_y = analogRead(Y1);
    firstpass = false;
  }

  
  
  
  // read the value from the sensor:
  x_val = analogRead(X1);
  y_val = analogRead(Y1);   
  // turn the ledPin on
  B1_val = digitalRead(B1_pin);
  B2_val = digitalRead(B2_pin);  
  B3_val = digitalRead(B3_pin);  
  B4_val = digitalRead(B4_pin); 

  //Average Calculation
  sum_x += x_val;
  sum_y += y_val;
  count++;
  avg_x = sum_x / count;
  avg_y = sum_y / count;
  



  // Calculate Deltas (for tuning)
  x_delta_high_mid = x_val - firstpass_x;
  x_delta_low_mid  = firstpass_x - x_val;

  y_delta_high_mid = y_val - firstpass_y;
  y_delta_low_mid  = firstpass_y - y_val;
  

  //sprintf(str, "X_val: %d, Y_val: %d, B1: %d, B2: %d, B3: %d, B4: %d\n", x_val, y_val, B1_val, B2_val, B3_val, B4_val);
  sprintf(str, "X_val: %5d, Y_val: %5d, X_avg: %5d, Y_avg: %5d, x_delta_high_mid: %5d, x_delta_low_mid: %5d, y_delta_high_mid: %5d, y_delta_low_mid: %5d\n", x_val, y_val, avg_x, avg_y, x_delta_high_mid, x_delta_low_mid, y_delta_high_mid, y_delta_low_mid);

  Serial.print(str); 
    
  delay(20);
}
