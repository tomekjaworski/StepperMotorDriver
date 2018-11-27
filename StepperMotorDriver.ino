/*
	Stepper motor driver for Arduino Uno
	Tomasz Jaworski, 2018
*/

#include "digitalWriteFast.h"

#define LED				13

#define PIN_MOTOR_ENABLE		2
#define PIN_MOTOR_DIRECTION		3
#define PIN_MOTOR_PULSE			4

#define LEFT				7
#define RIGHT				8


#define min(__a, __b) ((__a) < (__b) ? (__a) : (__b))
#define abs(__a) ((__a) < 0 ? -(__a) : (__a))

enum class MotorDirection {
	Forward,
	Backward
};

struct {
  long current;
  int delay;
  MotorDirection direction;
} position;

struct {
  long target;	

  long topspeed_steps;
  long delta;

  long acc_steps;
  long decc_steps;

  int step_delay;
} task;

// -- hardware stuff


#if defined(Arduino_h)

  bool __pulse_state = 0;
  #define DEBUG(...)
  #define MOTOR_SET_DIRECTION(__x)									\
		do {														\
			if ((__x) == 1) {										\
				position.direction = MotorDirection::Forward;		\
				digitalWriteFast(PIN_MOTOR_DIRECTION, true)			\
				digitalWriteFast(LED, true);						\
			}														\
			else {													\
				position.direction = MotorDirection::Backward;		\
				digitalWriteFast(PIN_MOTOR_DIRECTION, false);		\
				digitalWriteFast(LED, false);						\
			}														\
		} while(0);
			
// -------------------------
			
  #define MOTOR_PULSE												\
		do {														\
	  		if (__pulse_state = !__pulse_state)						\
			{														\
				digitalWriteFast(PIN_MOTOR_PULSE, true);			\
			} else {												\
				digitalWriteFast(PIN_MOTOR_PULSE, false);			\
			}														\
		} while (0);
		
#else
  // test code
  long __step = 0;
  #define MOTOR_SET_DIRECTION(__x) do { __step = __x; } while(0)
  #define MOTOR_PULSE position.current += __step;

  #define DEBUG(...) printf(__VA_ARGS__)
#endif



void do_accelerate(void)
{
	for (int i = 0; i < task.acc_steps; i++)
	{
		DEBUG("ACC-F CP=%ld; delay=%d\n", position.current, position.delay);

		// move
		MOTOR_PULSE;
		position.delay -= task.step_delay;
		delayMicroseconds(position.delay);
	}
  
	if (position.direction == MotorDirection::Forward)
		position.current += task.acc_steps;
	else
		position.current -= task.acc_steps;
}

void do_move(void)
{
	for (int i = 0; i < task.topspeed_steps; i++) {
		DEBUG("mov-F CP=%ld; delay=%d\n", position.current, position.delay);

		// move
		MOTOR_PULSE;
		delayMicroseconds(position.delay);
	}
	
	if (position.direction == MotorDirection::Forward)
		position.current += task.topspeed_steps;
	else
		position.current -= task.topspeed_steps;

}

void do_deccelerate(void) 
{
	for (int i = 0; i < task.decc_steps; i++) {
		DEBUG("DEC-F CP=%ld; delay=%d\n", position.current, position.delay);

		// move
		MOTOR_PULSE;
		position.delay += task.step_delay;
		delayMicroseconds(position.delay);
	}

	if (position.direction == MotorDirection::Forward)
		position.current += task.decc_steps;
	else
		position.current -= task.decc_steps;

}


void setup() {
	// put your setup code here, to run once:
	Serial.begin(9600);
	Serial.println("Sterownik silnika krokowego");

	pinModeFast(LED, OUTPUT);

	pinModeFast(PIN_MOTOR_ENABLE, OUTPUT);
	pinModeFast(PIN_MOTOR_DIRECTION, OUTPUT);
	pinModeFast(PIN_MOTOR_PULSE, OUTPUT);

	pinModeFast(LEFT, INPUT);
	pinModeFast(RIGHT, INPUT);
}

//int start_delay = 16000;
//int stop_delay = 1000;
//int step_delay = 2;
//int nsteps = (start_delay - stop_delay) / step_delay;


struct {
	int start_delay;
	int stop_delay;
	int step_delay;
	int n_delay_steps;
} config;

void do_goto(long target)
{
	task.target = target;

	task.delta = task.target - position.current;
	task.acc_steps = min(abs(task.delta) / 2, config.n_delay_steps);
	task.decc_steps = min(abs(task.delta) / 2, config.n_delay_steps);
	task.step_delay = config.step_delay;


	DEBUG("acceleration_steps=%ld\n", task.acc_steps);
	DEBUG("decceleration_steps=%ld\n", task.decc_steps);

	task.topspeed_steps = abs(task.delta) - task.acc_steps - task.decc_steps;

	char buffer[100];
	sprintf(buffer, "current=%ld; target=%ld\n", position.current, task.target);
	Serial.print(buffer);
	
	if (task.delta > 0) {
		Serial.println("a");
		MOTOR_SET_DIRECTION(1);
	} else {
		Serial.println("b");
		MOTOR_SET_DIRECTION(-1);
	}

	do_accelerate();
	do_move();
	do_deccelerate();

	DEBUG("FINAL STATE: position=%ld; delay=%d\n\n", position.current, position.delay);  
}



void loop() {
	// put your main code here, to run repeatedly:

	digitalWriteFast(PIN_MOTOR_ENABLE, false);
	digitalWriteFast(PIN_MOTOR_DIRECTION, true);
	digitalWriteFast(PIN_MOTOR_PULSE, false);


	// konfiguracja
	config.start_delay = 1600;
	config.stop_delay = 250;
	config.step_delay = 4;
	config.n_delay_steps = (config.start_delay - config.stop_delay) / config.step_delay;

	// warunki początkowe
	position.current = 0;
	position.delay = config.start_delay;


	

	while(0)
	{
		bool left = digitalReadFast(LEFT);
		bool right = digitalReadFast(RIGHT);

		char tab[10];
		sprintf(tab, "L%dR%d", left, right);
		Serial.println(tab);
	}


	while(0) {
		MOTOR_PULSE;
		delayMicroseconds(200);
	}

	
	while (1)
	{

		do_goto(10 * 200 * 4 * 2);
		delay(100);

//		do_goto(1500);
//		do_goto(-1000);
		do_goto(0);
		delay(300);
	}

}
