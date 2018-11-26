/*
	Stepper motor driver for Arduino Uno
	Tomasz Jaworski, 2018
*/
/*

#include <stdio.h>

#define min(__a, __b) ((__a) < (__b) ? (__a) : (__b))
#define abs(__a) ((__a) < 0 ? -(__a) : (__a))


struct {
  long current;
  long target;	

  int delay;
} position;

struct {
  long fullspeed_steps;
  long delta;

  long acc_steps;
  long decc_steps;

  int step_delay;
} task;

// -- hardware stuff

#if defined(Arduino)

  #define DEBUG(...)
  #define MOTOR_SET_DIRECTION(__x)
  #define MOTOR_PULSE

#else
  // test code
  long __step = 0;
  #define MOTOR_SET_DIRECTION(__x) do { __step = __x; } while(0)
  #define MOTOR_PULSE position.current += __step;

  #define DEBUG(...) printf(__VA_ARGS__)
#endif
//
//
// ##############################################################################
//
//

void do_accelerate(void)
{
  for (int i = 0; i < task.acc_steps; i++)
  {
    DEBUG("ACC-F CP=%ld; delay=%d\n", position.current, position.delay);
    
    // move
    position.delay -= task.step_delay;
    MOTOR_PULSE;
  }
}

void do_move(void)
{
  for (int i = 0; i < task.fullspeed_steps; i++) {
    DEBUG("mov-F CP=%ld; delay=%d\n", position.current, position.delay);

    // move
    MOTOR_PULSE;
  }
}

void do_deccelerate(void) 
{
  for (int i = 0; i < task.decc_steps; i++) {
    DEBUG("DEC-F CP=%ld; delay=%d\n", position.current, position.delay);
    
    // move
    position.delay += task.step_delay;
    MOTOR_PULSE;
  }

}

struct {
  int start_delay;
  int stop_delay;
  int step_delay;
  int n_delay_steps;
} config;

void do_goto(long target)
{
  position.target = target;

  task.delta = position.target - position.current;
  task.acc_steps = min(abs(task.delta) / 2, config.n_delay_steps);
  task.decc_steps = min(abs(task.delta) / 2, config.n_delay_steps);
  task.step_delay = config.step_delay;


  DEBUG("acceleration_steps=%ld\n", task.acc_steps);
  DEBUG("decceleration_steps=%ld\n", task.decc_steps);

  task.fullspeed_steps = abs(task.delta) - task.acc_steps - task.decc_steps;

  if (task.delta > 0)
    MOTOR_SET_DIRECTION(1);
  else
    MOTOR_SET_DIRECTION(-1);

  do_accelerate();
  do_move();
  do_deccelerate();

  DEBUG("FINAL STATE: position=%ld; delay=%d\n\n", position.current, position.delay);  
}


int main(void) {

  // konfiguracja
  config.start_delay = 100;
  config.stop_delay = 10;
  config.step_delay = 2;
  config.n_delay_steps = (config.start_delay - config.stop_delay) / config.step_delay;

  // warunki początkowe
  position.current = 0;
  position.delay = config.start_delay;


  do_goto(50);

  do_goto(150);

  do_goto(-100);

  return 0;
}

*/