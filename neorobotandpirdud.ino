
// I used the Arduino UNO R2. 
// Arduino prototyping shield (optional)
// Three TG9e micro servos,  two of them modified for continuous rotation. 
// Sharp IR sensor GP2Y0A2

#include <Servo.h> 
#include <Adafruit_NeoPixel.h>

#define PIN 3
Adafruit_NeoPixel strip = Adafruit_NeoPixel(3, PIN, NEO_RGB + NEO_KHZ800);

// ============ OPERATIONAL CONSTANTS =================

int safe_distance = 40; // safe distance to obstacle before robot takes action (eg stop, turn) - in inches

// This robot moves by differential steering. To make it turn, one wheel must move
// faster than the other in same or opposite direction for a a number of seconds. 
// Use trial and error to find this time value for your own robot
//  These values depend on many factors such as wheel size, surface qualities, traction, etc. 
// It's best test your robot on surfaces resembling your robot's playground (eg sumo disc, marble)

// Times are expressed in milliseconds. 1000 = 1 sec. 

int rturn_time = 850;          // time needed for wheels to make a 90 Right turn
int lturn_time = 850;          // time needed for wheels to make a 90 Left turn
int reverse_time = 2000;     // time needed for wheels while reversing
int backturn_time = 2000;   // time needed for wheels to spin to turn 180
int rr;
int br;
int gr;

// Create Servo objects
Servo IR_servo;           // Servo to rotate Sharp IR sensor acting as radar 
Servo l_servo;             // Servo to drive Left wheel
Servo r_servo;            // Servo to d rive Right wheel

// Assign servos/sensor I/O to Arduino  pins
int IR_read_pin = 5;       // analog pin to read IR sensor
int IR_pin = 9;                // pin to control servo to spin IR sensor
int IR_value_debug =0;
int l_pin = 10;              // pin to direct Left wheel servo
int r_pin = 11;             // pin to direct Right wheel servo
int PIR = 4;
int PIRVal = 0;
int PIRState = LOW;
// Assign Leds to pins

int Led_L = 6;
int Led_R = 7;
int Led_O = 8;
volatile int state = LOW;


// Assign Button Pin


// Trackers to control servo positions. 
int IR_pos = 90;
int l_pos = 0;
int r_pos = 0;

// Values used to stop wheel servos. In this program, I don't use them 
// because I just "detach" servo objects which stops the servos

int r_stop = 78;  // this value is specific to a given servo 
int l_stop = 90;  //   this value is specific to a given servo 

// I use these values to drive servos at maximum speed. Note how one speed is negative 
// while other is positive. 
// That's because when you position servos on both sides of the bot, they will 
// spin opposite each other causing the robot to spin around itself. 
// so to drive a robot with two servos in one direction, one wheel must 
// move clockwise and the other counterclockwise for robot to move ahead or back 

int r_go = +200;  // this value is specific to a given servo 
int l_go = -200;  // this value is specific to a given servo 

// Same as above but reversed. 
int r_reverse = -200;  // this value is specific to a given servo 
int l_reverse = +200;  // this value is specific to a given servo 

char go_direction = 'F' ;  // holds current robot direction
// char last_direction = 'F';
// char dir = 'F';

void setup() 
{   
  Serial.begin(9600);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  pinMode(IR_pin, INPUT);    // set IR Sonar pin for input 
  pinMode(Led_R, OUTPUT);
  pinMode(Led_L, OUTPUT);
  pinMode(Led_O, OUTPUT);
  attachInterrupt(0, Stops, CHANGE); 
  IR_servo.attach(IR_pin);   // activate IR scanning servo  

  // find which direction to start moving.   
  go_direction = look_around('A');            // scan in (A)ll directions to decide where to go first
  go_direction = move_to(go_direction);  // Go in direction with more space
} 

void loop()
{ 
  // F = Forward  R = Right  L = Left  B = Backturn 180  T = Reverse S = Stop D= Deadend O = Obstacle

  while (go_direction == 'F'){  // keep going forward while no Obstacle is reached and direction returned is 'F'orward
    // Serial.print("Loop/Global IR value: ");   Serial.println(IR_value_debug);
    go_direction = look_around(go_direction);
  }
  go_still(); // stop
  strip.setPixelColor(1, 250,20,5);
  strip.setPixelColor(2, 250,20,5);
  strip.setPixelColor (0,100,100,100);
  strip.show();

  // Serial.print("NO LONGER MOVING FORWARD BECAUSE OF CONDITION: "); Serial.println(go_direction);

  if (go_direction == 'O'){            // if Forward movement reaches Obstacle
    go_direction = look_around('A');   // look around
    if (go_direction == 'D') {         // if Dead end
      move_to('B');                    // turn 180
      go_direction = look_around('A'); // look around
      if (go_direction == 'D'){        // if another Dead end
        move_to('S');                  // Stop and send help
        // Serial.println("DEAD END! I GIVE UP.");
        while (1==1) {
        }                
      } 
    }
  }
  go_direction = move_to(go_direction);
}

// ====================== RADAR FUNCTION ============================

char look_around(char whereto){
  char dirx = '/'; 
  int last_distance = 0;
  int distance = 0;

  switch (whereto) { // scan where?

  case 'F':  // scan if Forward is clear, return same F if true
    // Serial.println("look_around>> Scanning forward");
    digitalWrite (Led_O, HIGH);
    digitalWrite (Led_L, LOW);
    digitalWrite (Led_R, LOW);
    IR_servo.write(90);
    delay(100);
    last_distance = IR_scan();
    delay(10);
    if (last_distance > safe_distance) {
      // Serial.print("look_around>> FORWARD DISTANCE: ");  Serial.println(last_distance);
      dirx = 'F';
      return 'F';    
    } 
    else
    {
      dirx = 'O';   // Obstacle reached
      return 'O';   // Obstacle reached
      digitalWrite (Led_O, HIGH);
      digitalWrite (Led_L, HIGH);
      digitalWrite (Led_R, HIGH);
    }
    break;

  case 'A':  // Scan L,F,R if clear. return direction of direction with more space. 
    dirx = 'D';    // Assume Dead end unless changed in following tests 
    digitalWrite (Led_O, HIGH);
    digitalWrite (Led_L, LOW);
    digitalWrite (Led_R, HIGH);

    // SCAN LEFT
    IR_servo.write(180); // turn IR servo Left (or Right depending on your setting)
    delay(1000);
    distance = IR_scan(); // get distance to obstacle NOTE: be mindful of out of range values for sensor

    // Serial.print("look_around>> Left scan distance: "); Serial.println(distance);
    if (distance > safe_distance ){
      dirx = 'R';
      digitalWrite (Led_O, LOW);
      digitalWrite (Led_L, LOW);
      digitalWrite (Led_R, HIGH);
    }

    // SCAN FORWARD
    IR_servo.write(90); // turn IR servo Forward
    delay(1000);
    last_distance = IR_scan(); // get distance to obstacle NOTE: be mindful of out of range values for sensor
    delay(500);
    // Serial.print("look_around>> Forward scan distance: ");  Serial.println(last_distance);
    if (last_distance > distance && last_distance > safe_distance ){
      distance = last_distance; // forward space is greater than previous space
      dirx = 'F'; //go forward
      digitalWrite (Led_O, HIGH);
      digitalWrite (Led_L, LOW);
      digitalWrite (Led_R, LOW);
    }

    // SCAN RIGHT
    IR_servo.write(0); // turn Right (or Left depending on servo orientation)
    delay(1000);
    last_distance = IR_scan(); // get distance to obstacle NOTE: be mindful of out of range values for sensor
    delay(500);
    // Serial.print("look_around>> Right scan distance: "); Serial.println(last_distance);    
    if (last_distance > distance && last_distance > safe_distance ){
      distance = last_distance; // forward space is greater than previous space
      dirx = 'L'; //go Left (or Right depending on your servo)
      digitalWrite (Led_O, LOW);
      digitalWrite (Led_L, HIGH);
      digitalWrite (Led_R, LOW);
    }
    return dirx;  // retun resulting direction. 
    break;
  }
  // Serial.print("look_around>> Go direction: "); Serial.print(dirx); Serial.print("   @distance: "); Serial.println 
  (last_distance);
}

// ==================MOVEMENT FUCTION =====================================
// F = forward  R = right  L = left  B = backturn 180  T = reverse S = stop 
char move_to(char dir){
  switch (dir) { // scan ahead
  case 'F':  
    // Serial.println("move_to>> Turn  forward");
    go_forward();
    return 'F';
    break;
  case 'L': 
    // Serial.println("move_to>> Turn  Left");
    go_left(lturn_time);
    go_forward();
    return 'F';
    break;
  case 'R': 
    // Serial.println("move_to>> Turn  Right");
    go_right(rturn_time);
    go_forward();    
    return 'F';
    break; 
  case 'B': 
    // Serial.println("move_to>> Backturn 180");
    go_back(backturn_time);
    go_still();
    return 'S';
    break;
  case 'T': 
    // Serial.println("move_to>> Reverse in straight line");
    go_reverse(reverse_time);
    go_still();
    return 'S';
    break;
  case 'S': 
    // Serial.println("move_to>> Stop");
    go_still();
    return 'S';
    break;
  }
}

// =========================== IR LOGIC ===========================
int IR_scan()  // scan for obstacles then return distance
{
  float IR_distance =0;
  float TOT_IR_distance =0;
  float volts = 0;
  int IR_readings = 10;
  //  Serial.println("==============START REAL TIME IR READING ===========");
  for (int i =0; i < IR_readings ; i++) { 
    volts += analogRead(IR_read_pin) *0.0048828125;  
    IR_distance += 65*pow(volts, -1.10);    // worked out from graph 65 = theoretical distance / (1/Volts)S - luckylarry.co.uk
    //   Serial.println(IR_distance);
    delay(50);   
    //Serial.print("IR_scan>> each IR distance: "); Serial.println(IR_distance);
    //TOT_IR_distance += IR_distance;
  }
  IR_distance  /= IR_readings;
  IR_value_debug = IR_distance;
  // Serial.print("IR_scan>> AVG TOTAL distance: "); Serial.println(IR_distance);
  return IR_distance;
}

// ================== PHYSICAL MOVEMENT ========================

void go_forward()
{
  attach_servos();
  l_servo.write(l_go);
  r_servo.write(r_go);
  strip.setPixelColor(1, 0,230,0);
  strip.setPixelColor(2, 0,230,0);
  strip.setPixelColor (0, 0,230,0);
  strip.show();
}

void go_still()
{
  delay(1000);
  detach_servos();
  strip.setPixelColor(1, 1,12,20);
  strip.setPixelColor(2, 1,12,20);
  strip.setPixelColor(0, 100,50,50);
  strip.show();
  PIRVal = digitalRead(4);  // read input value
  if (PIRVal == HIGH) {            // check if the input is HIGH
  pir_dance();
  PIRVal == LOW;
  }
   else{
   rr=random(255);
   gr=random(255);
   br=random(255);
  strip.setPixelColor(1, rr,gr, br);
  strip.setPixelColor(2, br, rr , gr);
  strip.setPixelColor (0,gr, br, rr);
  strip.show();}
  delay(1000);
  PIRVal == LOW;
}

void go_reverse(int move_time){ // move straight  but opposite direction
  delay(50);
  attach_servos();
  l_servo.write(l_reverse);
  r_servo.write(r_reverse);
  strip.setPixelColor(1, 220, 2, 220);
  strip.setPixelColor(2, 220, 0, 220);
  strip.setPixelColor (0,69, 89, 200);
  strip.show();
  delay(move_time); // keep moving for seconds before stopping
  detach_servos();  // stop

  delay(50);
}

void  go_back(int move_time){  // turn 180 in place
  delay(50);
  attach_servos();
  l_servo.write(l_reverse);
  r_servo.write(r_go);
  strip.setPixelColor(1, 0,0, 200);
  strip.setPixelColor(2, 0, 0, 200);
  strip.setPixelColor (0, 220,220,220);
  strip.show();
  delay(move_time); // keep moving for seconds before stopping
  detach_servos();  // stop
  delay(50);
}

void  go_left(int move_time){
  delay(50);
  //attach_servos();
  attach_servos();
  l_servo.write(l_go);
  r_servo.write(r_reverse);
  delay(move_time); // keep moving for seconds before stopping
  strip.setPixelColor(1, 0, 200, 0);
  strip.setPixelColor(2, 200,0, 0);
  strip.setPixelColor (0,69, 89, 200);
  strip.show();
  detach_servos();  // stop
  delay(50);
}

void  go_right(int move_time){
  delay(100);
  attach_servos();
  l_servo.write(l_reverse);
  r_servo.write(r_go);
  delay(move_time); // keep moving for seconds before stopping
  strip.setPixelColor(1, 200,0,0);
  strip.setPixelColor(2, 0, 200, 0);
  strip.setPixelColor (0,128,128,128);
  strip.show();
  detach_servos(); // stop
  delay(50);
}

void detach_servos(){
  delay(50);
  l_servo.detach();  // sure way to stop servo
  r_servo.detach(); // sure way to stop servo
  delay(50);
}

void attach_servos(){
  delay(100);
  l_servo.attach(l_pin);
  r_servo.attach(r_pin);
  delay(50);
  strip.setPixelColor(1, 200,0,200);
  strip.setPixelColor(2, 200,0 ,200);
  strip.setPixelColor (0,200,0,200);
  strip.show();
}

void Stops()
{
  delay(100);
  l_servo.detach();  // sure way to stop servo
  r_servo.detach(); // sure way to stop servo
  state = LOW;
  strip.setPixelColor(1, 200,0,0);
  strip.setPixelColor(2, 200,0 ,0);
  strip.setPixelColor (0,200,0, 0);
  strip.show();
  while(1) { 
  }
} 

void pir_dance(){
  rr=random(255);
   gr=random(255);
   br=random(255);
  strip.setPixelColor(1, rr,gr, br);
  strip.setPixelColor(2, br, rr , gr);
  strip.setPixelColor (0,gr, br, rr);
  strip.show();
  go_reverse(1300);
   rr=random(255);
   gr=random(255);
   br=random(255);
  strip.setPixelColor(1, rr,gr, br);
  strip.setPixelColor(2, br, rr , gr);
  strip.setPixelColor (0,gr, br, rr);
  strip.show();
  go_back(1300);
  rr=random(255);
   gr=random(255);
   br=random(255);
  strip.setPixelColor(1, rr,gr, br);
  strip.setPixelColor(2, br, rr , gr);
  strip.setPixelColor (0,gr, br, rr);
  strip.show();
  go_left(500);
  rr=random(255);
   gr=random(255);
   br=random(255);
  strip.setPixelColor(1, rr,gr, br);
  strip.setPixelColor(2, br, rr , gr);
  strip.setPixelColor (0,gr, br, rr);
  strip.show();
  go_right(rr+200);
  rr=random(255);
   gr=random(255);
   br=random(255);
  strip.setPixelColor(1, rr,gr, br);
  strip.setPixelColor(2, br, rr , gr);
  strip.setPixelColor (0,gr, br, rr);
  strip.show();
}
  
//================ END ====================
