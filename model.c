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
long __step = 0;
#define MOTOR_SET_DIRECTION(__x) do { __step = __x; } while(0)
#define MOTOR_PULSE position.current += __step;

//
//
// ##############################################################################
//
//

void do_accelerate(void)
{
  for (int i = 0; i < task.acc_steps; i++)
  {
    printf("ACC-F CP=%ld; delay=%d\n", position.current, position.delay);
    
    // move
    position.delay -= task.step_delay;
    MOTOR_PULSE;
  }
}

void do_move(void)
{
  for (int i = 0; i < task.fullspeed_steps; i++) {
    printf("mov-F CP=%ld; delay=%d\n", position.current, position.delay);

    // move
    MOTOR_PULSE;
  }
}

void do_deccelerate(void) 
{
  for (int i = 0; i < task.decc_steps; i++) {
    printf("DEC-F CP=%ld; delay=%d\n", position.current, position.delay);
    
    // move
    position.delay += task.step_delay;
    MOTOR_PULSE;
  }

}



int main(void) {

  position.current = -50;
  position.target = 50;

  int start_delay = 100;
  int stop_delay = 10;
  int step_delay = 2;
  int nsteps = (start_delay - stop_delay) / step_delay;


  position.delay = start_delay;

  task.delta = position.target - position.current;
  task.acc_steps = min(abs(task.delta) / 2, nsteps);
  task.decc_steps = min(abs(task.delta) / 2, nsteps);
  task.step_delay = step_delay;


  printf("acceleration_steps=%ld\n", task.acc_steps);
  printf("decceleration_steps=%ld\n", task.decc_steps);

  task.fullspeed_steps = abs(task.delta) - task.acc_steps - task.decc_steps;

  if (task.delta > 0)
    MOTOR_SET_DIRECTION(1);
  else
    MOTOR_SET_DIRECTION(-1);

  do_accelerate();
  do_move();
  do_deccelerate();

  printf("FINAL STATE:");
  printf("position=%ld; delay=%d", position.current, position.delay);
  return 0;
}
*/