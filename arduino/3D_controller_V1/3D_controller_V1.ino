#include <Subscriber.h>
#include <SubscriberManager.h>

#include <Publisher.h>
#include <PublisherManager.h>

#include <MPU6050_tockn.h>
#include <Wire.h>


// ==================================== MOTOR CLASS
class Motor {
  public:
    Motor(int dir1, int dir2, int speedp);
    void setPower(double power);
    
  private:
    int dir1Pin;
    int dir2Pin;
    int speedPin;
};

Motor::Motor(int dir1, int dir2, int speedp){
  dir1Pin = dir1;
  dir2Pin = dir2;
  speedPin = speedp;

  pinMode(dir1, OUTPUT);
  pinMode(dir2, OUTPUT);
  pinMode(speedp, OUTPUT);
}

void Motor::setPower(double power){
  int pwm_power = constrain(int(abs(255*power)), 0, 255);

  if(power > 0){
    digitalWrite(dir1Pin, HIGH);
    digitalWrite(dir2Pin, LOW);
    analogWrite(speedPin, pwm_power);
  }else{
    digitalWrite(dir1Pin, LOW);
    digitalWrite(dir2Pin, HIGH);
    analogWrite(speedPin, pwm_power);
  }
}

// ==================================== HARDWARE
// For accelerometer
MPU6050 mpu6050(Wire);

// Motors
Motor motorA = Motor(42, 43, 3);
Motor motorB = Motor(40, 41, 4);
Motor motorC = Motor(38, 39, 5);

// ================================== PUBLISHERS AND SUBSCRIBERS
PublisherManager pm;

Publisher time_pub = Publisher("time");
Publisher roll = Publisher("roll");
Publisher pitch = Publisher("pitch");
Publisher yaw = Publisher("yaw");
Publisher wheelA = Publisher("wheelA");
Publisher wheelB = Publisher("wheelB");
Publisher wheelC = Publisher("wheelC");

SubscriberManager sm;

// Set up subscribers
Subscriber onoff_sub = Subscriber("on");
Subscriber kP_sub = Subscriber("kP");
Subscriber kD_sub = Subscriber("kD");
Subscriber kOffset_sub = Subscriber("kOffset");
Subscriber cutoffAngle_sub = Subscriber("cutoffAngle");
Subscriber roll_setpoint_sub = Subscriber("rollSP");
Subscriber pitch_setpoint_sub = Subscriber("pitchSP");

// ================================== SETUP
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // init gyro
  Wire.begin();
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);


  // Assign publishers and subscribers
  pm.addPublisher(time_pub);
  pm.addPublisher(roll);
  pm.addPublisher(pitch);
  pm.addPublisher(yaw);
  pm.addPublisher(wheelA);  // Once the algorithm is known functional, can calculate on computer side
  pm.addPublisher(wheelB);
  pm.addPublisher(wheelC);


  sm.addSubscriber(onoff_sub);
  sm.addSubscriber(kP_sub);
  sm.addSubscriber(kD_sub);
  sm.addSubscriber(kOffset_sub);
  sm.addSubscriber(cutoffAngle_sub);
  sm.addSubscriber(roll_setpoint_sub);
  sm.addSubscriber(pitch_setpoint_sub);
}

// ================================== OTHER VARS
unsigned long loop_time_ms = 50; // ms
double loop_time_s = loop_time_ms / 1000.0; // seconds!
unsigned long start_time_ms;

// Pre-computed sin cos values
double s = 0.866025403784; // sin(120)
double c = -0.5; // cos(120)

// Pre-computed deg -> rad
double degtorad = 0.01745329;

double rollCmd = 0;
double pitchCmd = 0;

double rollOffset = 0;
double pitchOffset = 0;

const double MAX_POWER = 0.5;

// Roll Pitch Yaw values
double prev_roll_val = 0;
double roll_dot = 0;
double prev_pitch_val = 0;
double pitch_dot = 0;
double prev_yaw_val = 0;
double yaw_dot = 0;

// ================================== LOOOOOOP
void loop() {
  // Loop start time
  start_time_ms = millis();
  
  // Save prev values
  prev_roll_val = roll.value;
  prev_pitch_val = pitch.value;
  prev_yaw_val = yaw.value;

  // Update gyro values
  mpu6050.update();
  roll.value = mpu6050.getAngleX() * degtorad; // Roll
  pitch.value = mpu6050.getAngleY() * degtorad; // Pitch
  yaw.value = mpu6050.getAngleZ() * degtorad;

  // Update derivatives
  roll_dot = (roll.value - prev_roll_val) / loop_time_s;
  pitch_dot = (pitch.value - prev_pitch_val) / loop_time_s;
  yaw_dot = (yaw.value - prev_yaw_val) / loop_time_s;

  // PD on roll and pitch
  rollCmd = kP_sub.value * (roll.value - roll_setpoint_sub.value + rollOffset) + kD_sub.value * (roll_dot);
  pitchCmd = kP_sub.value * (pitch.value - pitch_setpoint_sub.value + pitchOffset) + kD_sub.value * (pitch_dot);

  // Offset
  rollOffset = rollOffset * onoff_sub.value;
  rollOffset += kOffset_sub.value * rollCmd * loop_time_s;

  pitchOffset = pitchOffset * onoff_sub.value;
  pitchOffset += kOffset_sub.value * pitchCmd * loop_time_s;


  // Calculate wheel powers
  wheelA.value = rollCmd * c + pitchCmd * s; // Wheel A power
  wheelB.value = -rollCmd; // Wheel B power
  wheelC.value = rollCmd * c - pitchCmd * s; // Wheel C power


  // Set constrained values if on = 1
  if(onoff_sub.value == 1){
    motorA.setPower(constrain(wheelA.value, -MAX_POWER, MAX_POWER));
    motorB.setPower(constrain(wheelB.value, -MAX_POWER, MAX_POWER));
    motorC.setPower(constrain(wheelC.value, -MAX_POWER, MAX_POWER));
  }else{
    motorA.setPower(0);
    motorB.setPower(0);
    motorC.setPower(0);
  }

  // Update publishers and subscribers
  pm.update();
  sm.update();

  // Grab execution time for testing
  time_pub.value = millis() - start_time_ms;

  // Enforce constant loop time
  while(millis() - start_time_ms <= loop_time_ms);
}