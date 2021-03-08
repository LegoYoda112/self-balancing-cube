
#include <MPU6050_tockn.h>
#include <Wire.h>
#include <Encoder.h>

// ==================================== HARDWARE
Encoder myEnc(18, 19);

MPU6050 mpu6050(Wire);

const int dir1PinA = 42;
const int dir2PinA = 43;
const int speedPinA = 3;

// ==================================== SETUP
void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);

  pinMode(dir1PinA, OUTPUT);
  pinMode(dir2PinA, OUTPUT);
  pinMode(speedPinA, OUTPUT);
}

// ==================================== VARS
// (will be populated by link software when tuning)
double setpoint_angle = 0;

double angle = 0;
double prev_angle = 0;
double angle_dot = 0;

double cutoff_angle = 0.3;

// Base PID values
double kP = 0;
double kD = 0;

// p and d values for debug
double p = 0;
double d = 0;
double u = 0;

// Offset integrator
double offset_gain = 0;
double setpoint_adjustment = 0;

// Wheel
long wheel_pos = 0;
long previous_wheel_pos = 0;
double wheel_speed = 0;

// ON vs OFF
int on = 1;

// Filter stuff -- REPLACE? YES REPLACE
const int num_readings = 5;
int rolling_d_index = 0;
double rolling_d [num_readings];
double total_d = 0;
double avg_d = 0;

// Loop timing
int loop_time_ms = 100; // ms
double loop_time_s = loop_time_ms / 1000.0; // seconds!
long start_time_ms = 0;

// ==================================== LOOP
void loop() {
  start_time_ms = millis(); // loop timing

  // Update sensors
  previous_wheel_pos = wheel_pos;
  
  wheel_pos = myEnc.read();
  wheel_speed = (wheel_pos - previous_wheel_pos);
  
  mpu6050.update();


  //TODO - Convert to radians, makes MATLAB -> real world easier
  // Calculate angular velocity
  prev_angle = angle;
  angle = (-mpu6050.getAngleX() * (2*3.1415 / 360)) - setpoint_adjustment;
  angle_dot = angle - prev_angle;
  
  //Serial.print("angleX : ");
  //Serial.print(angle);
  //Serial.print(" angleX_dot : ");
  //Serial.print(angle_dot);

  // If cutoff_angle is exceeded, turn the thing off so nothing blows up when it falls over
  if(abs(angle) < cutoff_angle){
    p = -kP * angle * on;
    d = - kD * angle_dot * on;
  }else{
    p = 0;
    d = 0;
  }

  u = p + d;
  
  //Setpoint offset stuff 
  setpoint_adjustment = setpoint_adjustment * on + setpoint_angle * (on - 1);
  setpoint_adjustment += offset_gain * u;
  

  // Set motor speed (actualy power [actually some highly non-linear thing but whatever, we are engineers])
  setSpeed(u);

  // Telementary
  Serial.print(" P: ");
  Serial.print(p);
  Serial.print(" D: ");
  Serial.print(d);
  Serial.print(" angle: ");
  Serial.print(angle);
  Serial.print(" set: ");
  Serial.print(setpoint_adjustment);
  Serial.println();

  // Parse any incoming serial commands
  parseSerial();

  while(millis() - start_time_ms <= loop_time_ms); // enforce loop time
}

// ==================================== SETSPEED
// Control the driver board (-1 to 1 power)
void setSpeed(double power){
  int pwm_power = constrain(int(abs(255*power)), 0, 255);

  if(power > 0){
    digitalWrite(dir1PinA, HIGH);
    digitalWrite(dir2PinA, LOW);
    analogWrite(speedPinA, pwm_power);
  }else{
    digitalWrite(dir1PinA, LOW);
    digitalWrite(dir2PinA, HIGH);
    analogWrite(speedPinA, pwm_power);
  }
}


// ==================================== PARSESERIAL
// Parse any incoming serial commands, accepts things in the form:
// cmd: value
// Credit: Me, like last year when I was a mega-brain and somehow wrote this
void parseSerial(){
    if(Serial.available() > 0){
      //Serial.println("Started parsing");
      char c = 'a';
      int inputIndex = 0;
      String inputString = "";
      String inputArgs[5];
      //Serial.print("Receiving string: ");
      while(c != '\n'){
        delay(1);
        if(Serial.available() > 0) {
          c = Serial.read();
          inputString += c;
          //Serial.print(c);
        }
        
        if(c == ' '){
          inputString[inputString.length()-1] = '\0';
          inputArgs[inputIndex] = inputString;
          inputIndex += 1;
          inputString = "";
        }
      }
      inputString[inputString.length()-1] = '\0';
      inputArgs[inputIndex] = inputString;
      
      //Serial.println("Finished parsing");
      //Serial.print("Command: ");
      //Serial.println(inputArgs[0]);
      //Serial.println(inputArgs[1]);

      String cmd = inputArgs[0];
      double val = inputArgs[1].toDouble();

      Serial.print(cmd);
      
      if(cmd == "kP"){
        kP = val;
      }else if(cmd == "kD"){
        kD = val;
      }else if(cmd == "setpoint"){
        setpoint_angle = val;
      }else if(cmd == "on"){
        on = val;
      }else if(cmd == "cutoffAngle"){
        cutoff_angle = 0.3;
      }else if(cmd == "offset_gain"){
        offset_gain = val;
      }
    }
}
