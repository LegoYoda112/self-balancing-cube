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

// Set up subscribers
SubscriberManager sm;

Subscriber onoff_sub = Subscriber("on");
Subscriber kP_sub = Subscriber("kP");
Subscriber kD_sub = Subscriber("kD");
Subscriber kOffset_sub = Subscriber("kOffset");
Subscriber cutoffAngle_sub = Subscriber("cutoffAngle");
Subscriber roll_setpoint_sub = Subscriber("rollSP");
Subscriber pitch_setpoint_sub = Subscriber("pitchSP");
Subscriber yaw_setpoint_sub = Subscriber("yawSP");

// ================================== SETUP
void setup() {
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
  sm.addSubscriber(yaw_setpoint_sub);
}

// ================================== OTHER VARS / CONSTANTS

// Loop timing
unsigned long loop_time_ms = 30; // ms
double loop_time_s = loop_time_ms / 1000.0; // seconds!
unsigned long start_time_ms;

// Motor value limit
const double MAX_POWER = 1;

// Pre-computed sin cos values
const double s = 0.866025403784; // sin(120)
const double c = -0.5; // cos(120)

// Pre-computed deg -> rad
const double degtorad = 0.01745329;

// Roll-Pitch-Yaw commanded forces/torques
double rollCmd = 0;
double pitchCmd = 0;
double yawCmd = 0;

// Setpoint offsets
double rollOffset = 0;
double pitchOffset = 0;

// Roll Pitch Yaw values
double prev_roll_val = 0;
double roll_dot = 0;
double roll_dot_filtered = 0;

double prev_pitch_val = 0;
double pitch_dot = 0;
double pitch_dot_filtered = 0;

double prev_yaw_val = 0;
double yaw_dot = 0;
double yaw_dot_filtered = 0;

// IIR filter alpha
const double alpha = 0.2;

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
  yaw.value = mpu6050.getAngleZ() * degtorad - yaw_setpoint_sub.value; // Yaw

  // Update derivatives
  roll_dot = (roll.value - prev_roll_val) / loop_time_s;
  pitch_dot = (pitch.value - prev_pitch_val) / loop_time_s;
  yaw_dot = (yaw.value - prev_yaw_val) / loop_time_s;

  // Use an IIR filter to perform a basic lowpass
  // Attempts to cutout noise due to wheel vibrations
  roll_dot_filtered = alpha * roll_dot + (1 - alpha) * roll_dot_filtered;
  pitch_dot_filtered = alpha * pitch_dot + (1 - alpha) * pitch_dot_filtered;
  yaw_dot_filtered = alpha * yaw_dot + (1 - alpha) * yaw_dot_filtered;

  // PD on roll, pitch and yaw
  rollCmd = constrain(kP_sub.value * (roll.value + rollOffset - roll_setpoint_sub.value) + kD_sub.value * (roll_dot_filtered), -1.5, 1.5);
  pitchCmd = constrain(kP_sub.value * (pitch.value + pitchOffset - pitch_setpoint_sub.value) + kD_sub.value * (pitch_dot_filtered), -1.5, 1.5);
  //yawCmd = kP_sub.value * (yaw.value) + kD_sub.value * yaw_dot_filtered;

  // Offsets
  rollOffset = rollOffset * onoff_sub.value;
  rollOffset += kOffset_sub.value * rollCmd * loop_time_s;

  pitchOffset = pitchOffset * onoff_sub.value;
  pitchOffset += kOffset_sub.value * pitchCmd * loop_time_s;


  // Calculate wheel powers
  wheelA.value = - rollCmd * c + pitchCmd * s; // Wheel A power
  wheelB.value = - rollCmd; // Wheel B power
  wheelC.value = - rollCmd * c - pitchCmd * s; // Wheel C power

  // Safety cutoff
  if(abs(roll.value - roll_setpoint_sub.value) > 0.3 || abs(pitch.value - pitch_setpoint_sub.value) > 0.3){
    onoff_sub.value = 0;
  }

  // Set constrained motor values if on = 1
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