#include "CytronMotorDriver.h"

/********************** Functions **********************/
double MapCommand(double x, double in_max, double out_max)
{
  if (x > in_max)
    x = in_max; 
  if (x < -in_max)
    x = -in_max;
  return (x/abs(in_max))*abs(out_max);
}


// MOTOR CLASS
class Motor
{
  public:   
    virtual void set_speed(double pwm);
  
};

class Motor_cytron : public Motor
{
  public:
    Motor_cytron(int pin_pwm, int pin_dir);
    virtual void set_speed(double pwm);
  private: 
    CytronMD _motor;
};


Motor_cytron::Motor_cytron(int pin_pwm, int pin_dir)
  : _motor(PWM_DIR, pin_pwm, pin_dir)
{   
  pinMode(pin_pwm, OUTPUT);
  pinMode(pin_dir, OUTPUT);
}


void Motor_cytron::set_speed(double pwm)
{
  //Scale the setpoint in -255 to 255
  double cmd = MapCommand(pwm, 1., 255.0); 
  
  //_motor.setSpeed(cmd);
}
