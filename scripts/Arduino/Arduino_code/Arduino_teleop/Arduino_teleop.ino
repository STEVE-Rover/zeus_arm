
/********************** Includes **********************/
#include <ros.h>
#include <math.h>
#include <std_msgs/Float64MultiArray.h>
#include <std_msgs/Int16.h>
#include <std_msgs/Bool.h>

/*
#include "CytronMotorDriver.h"
#include <ams_as5048b.h>
#include <Servo.h>
#include <Bounce2.h>
#include <digitalWriteFast.h>
*/

#include "Encoder.h"
#include "Limitswitch.h"
#include "Motor.h"
/********************** Includes **********************/


/********************** Constants **********************/
#define U_RAW 1
#define U_DEG 3
#define U_RAD 4

#define N_MOTORS 6
#define N_ENCODERS 4

const int counts_per_revolution       = 6533;
const unsigned long TIME_PERIOD_LOW   = 100;      // 10 ms publishing rate loop
const unsigned long TIME_PERIOD_COM   = 1000;    //1000 ms after that it sends 0
unsigned long       time_last_low     = 0;
unsigned long       time_now          = 0;

// Calib all joints
bool calib_all_joints = false;
bool in_calib = false;

std_msgs::Int16 calib_value;
std_msgs::Float64MultiArray joint_positions;

ros::NodeHandle nh;
ros::Publisher calib_state_pub("/zeus_arm/calibration_state", &calib_value);
ros::Publisher pos_pub("/zeus_arm/joint_positions", &joint_positions);

// Create encoder object :
Encoder* enc_arr[N_ENCODERS] = {
    new Encoder_oth(19, 18, counts_per_revolution*2), //Dont forget the ratio from motor to base
    new Encoder_ams(0x40, U_RAD),
    new Encoder_ams(0x41, U_RAD),
    new Encoder_ams(0x42, U_RAD)
};

// Create limitswitch object 
Limitswitch* switch_arr[N_ENCODERS] = {
    new Limitswitch(23, false),
    new Limitswitch(24, false),
    new Limitswitch(25, false),
    new Limitswitch(26, false)
};

// Create motor object : 
// Motor::Motor(int pin_pwm, int pin_dir, double max_speed, 
//    double min_speed_threshold, unsigned int time_period_com = 1000)

Motor motor_arr[N_MOTORS] = {
    Motor(11, 12, 0.3, 0.1, TIME_PERIOD_COM), //J1
    Motor(9, 10, 0.5, 0.1, TIME_PERIOD_COM), //J2
    Motor(7, 8, 1, 0.1, TIME_PERIOD_COM), //J3
    Motor(5, 22, 1, 0.1, TIME_PERIOD_COM), //J4
    Motor(3, 4, 0.5, 0.1, TIME_PERIOD_COM), //J5
    Motor(2, 13, 1, 0.1, TIME_PERIOD_COM) //J6
};
        

/********************** CALLBACKS **********************/
void MessageCallback( const std_msgs::Float64MultiArray& cmd_msg)
{
  for (int i=0; i<N_MOTORS; i++)
  {
    motor_arr[i].vel_setpoint = cmd_msg.data[i];
    motor_arr[i].UpdateLastComm();
  }
}

void CalibCallback(const std_msgs::Int16 & calib_cmd)
{
  calib_value.data = calib_cmd.data;
}

// ROS subscribers
ros::Subscriber<std_msgs::Float64MultiArray> cmd_sub("/zeus_arm/joint_commands", &MessageCallback );
ros::Subscriber<std_msgs::Int16> calib_sub("/zeus_arm/calib_cmd", &CalibCallback );


void Calib_loop()
{
  if (calib_value.data >= 0)
  {
    if (calib_value.data == 0)
    {
      calib_all_joints = true;
      calib_value.data = N_ENCODERS;
    }
    else if (calib_value.data > 0 && calib_value.data <= N_ENCODERS)
    {
      if (!in_calib)
      {
        motor_arr[calib_value.data].start_calib = true;
        in_calib = true;
      }
      else
      { // If calib of motor is done
        if (!motor_arr[calib_value.data].start_calib)
        {
          in_calib = false;
          if (calib_all_joints)
          {
            if (calib_value.data == 1)
            {
              calib_value.data = -1;
              calib_all_joints = false;
            }
            else
              calib_value.data -= 1;
          }
          else
            calib_value.data = -1;
        }
      }
    }
  }
  
}

/********************** Arduino Loop **********************/

void setup() {
  // Use same baud as rosserial_arduino
  Serial.begin(57600);

  calib_value.data = -1;
  
  // Init ROS stuff
  nh.initNode();
  nh.subscribe(cmd_sub);
  
}

void loop() {
  time_now = millis();

  // Calib loop
  Calib_loop();

  // Motor loop
  for (int i=0; i<N_MOTORS; i++)
  {
    motor_arr[i].motor_loop();
  }
  
  // Low level loop
  if ((time_now - time_last_low) > TIME_PERIOD_LOW )
  {
    float data_arr[N_ENCODERS];
    for (int i=0; i<N_ENCODERS; i++)
    {
      data_arr[i] = enc_arr[i]->get();
    }
    joint_positions.data = data_arr;
    pos_pub.publish(&joint_positions);

    // Send calib state
    calib_state_pub.publish(&calib_value);
    
    
    time_last_low = time_now; 
  }
  // Process ROS Events
  nh.spinOnce();
  
}
