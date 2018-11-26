/*
	Stepper motor driver for Arduino Uno
	Tomasz Jaworski, 2018
*/

#include <stdio.h>

#define min(a, b) ((a < b) ? (a) : (b))

int main(void) {

  long current_position = 50;
  long target_position = -50;	
  int start_delay = 100;
  int stop_delay = 10;
  int step_delay = 2;
  int nsteps = (start_delay - stop_delay) / step_delay;

  long delta = target_position - current_position;

  int current_delay = start_delay;
  int acceleration_steps = min(delta / 2, nsteps);
  int decceleration_steps = min(delta / 2, nsteps);

  printf("acceleration_steps=%d\n", acceleration_steps);
  printf("decceleration_steps=%d\n", decceleration_steps);

  int fullspeed_steps = delta - acceleration_steps - decceleration_steps;

  for (int i = 0; i < acceleration_steps; i++)
  {
    printf("ACC CP=%ld; delay=%d\n", current_position++, current_delay);
    current_delay -= step_delay;
  }

  for (int i = 0; i < fullspeed_steps; i++)
    printf("--- CP=%ld; delay=%d\n", current_position++, current_delay);

  for (int i = 0; i < decceleration_steps; i++) {
    printf("DEC CP=%ld; delay=%d\n", current_position++, current_delay);
    current_delay += step_delay;
  }

  return 0;
}