/*
	Stepper motor driver for Arduino Uno
	Tomasz Jaworski, 2018
*/

#include "digitalWriteFast.h"
#include <avr/wdt.h>

#define LED						13

#define PIN_MOTOR_ENABLE		2
#define PIN_MOTOR_DIRECTION		3
#define PIN_MOTOR_PULSE			4

#define PIN_LIMIT_LEFT			7
#define PIN_LIMIT_RIGHT			8



enum class MotorDirection {
	Forward,
	Backward
};


struct {
	int start_delay;
	int stop_delay;
	int step_delay;
	int n_delay_steps;
	int divider;
} config;


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
	#define MOTOR_SET_DIRECTION(__x)							\
		do {													\
			if ((__x) == 1) {									\
				position.direction = MotorDirection::Forward;	\
				digitalWriteFast(PIN_MOTOR_DIRECTION, true)		\
				digitalWriteFast(LED, true);					\
			}													\
			else {												\
				position.direction = MotorDirection::Backward;	\
				digitalWriteFast(PIN_MOTOR_DIRECTION, false);	\
				digitalWriteFast(LED, false);					\
			}													\
		} while(0);
			
// -------------------------
			
	#define MOTOR_PULSE											\
		do {													\
	  		if (__pulse_state = !__pulse_state)					\
			{													\
				digitalWriteFast(PIN_MOTOR_PULSE, true);		\
			} else {											\
				digitalWriteFast(PIN_MOTOR_PULSE, false);		\
			}													\
		} while (0);
	
	#define MOTOR_ENABLE(__const_value)							\
		do {													\
			digitalWriteFast(PIN_MOTOR_ENABLE, !__const_value)	\
		} while (0);											\

		
		
#else
	// test code
	long __step = 0;
	#define MOTOR_SET_DIRECTION(__x) do { __step = __x; } while(0)
	#define MOTOR_PULSE position.current += __step;
	#define MOTOR_ENABLE(__const_value)

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


void cmd_test_limit_switches(void)
{
	char buffer[] = {'\n', 'L', '-','R','-','\x0'}; // 2, 4
	
	while (Serial.available() == 0)
	{
		buffer[2] = '0' + !!digitalReadFast(PIN_LIMIT_LEFT);
		buffer[4] = '0' + !!digitalReadFast(PIN_LIMIT_RIGHT);
		
		Serial.print(buffer);
	}
	
	while (Serial.available())
		Serial.read();	
}

void setup() {
	wdt_disable();
	Serial.begin(9600);
	pinModeFast(LED, OUTPUT);


	// setup driver pins
	pinModeFast(PIN_MOTOR_ENABLE, OUTPUT);
	pinModeFast(PIN_MOTOR_DIRECTION, OUTPUT);
	pinModeFast(PIN_MOTOR_PULSE, OUTPUT);

	digitalWriteFast(PIN_MOTOR_ENABLE, true);
	digitalWriteFast(PIN_MOTOR_DIRECTION, false);
	digitalWriteFast(PIN_MOTOR_PULSE, false);
	
	// setup inputs for limit switches
	pinModeFast(PIN_LIMIT_LEFT, INPUT);
	pinModeFast(PIN_LIMIT_RIGHT, INPUT);

	MOTOR_SET_DIRECTION(1);
	MOTOR_ENABLE(false);
	

	// basic motro parameters setup
	config.start_delay = 1600;	// Interval [us] between steps at ZERO velocity
	config.stop_delay = 250;	// Interval [us] between steps at TOP velocity
	config.step_delay = 4;		// Acceleration/decceleration interval
								// Amount of time [us] that is added or subtracted from common step interval during ramp execution
								// This parameter can be also interpreted as the slope of the velocity ramp
	
	config.divider = 1;			// Step divider set with the Driver's Switches
								// 1	=	1/1
								// 2	=	1/2
								// 4	=	1/4
								// 8	=	1/8
								// 16	=	1/16

	config.n_delay_steps = (config.start_delay - config.stop_delay) / config.step_delay;

	// initial conditions
	position.current = 0;
	position.delay = config.start_delay;
	
	
	
	// I'm alive!
	delay(1000);
	for (int i = 0; i < 10; i++)
	{
		Serial.print((char)(43+2*(i&1)));
		delay(200);
	}
	
	Serial.println();
	Serial.println(F("================================================================================="));
	Serial.println(F("# Stepper motor controller (motor 57HS22-A, driver HY-DIV-268N-5A               #"));
	Serial.println(F("# v1.0 Tomasz Jaworski, 2018                                                    #"));
	Serial.println(F("================================================================================="));
	Serial.println(F("Commands should end only with the '\\n' character."));
	Serial.println(F("ARDUINO IDE: Change 'No line ending' to 'Newline' in the bottom part of your console"));
}



void loop() {


	String s = "";
	String old_command = "";
	
	while(1)
	{
		Serial.print("> ");
		s = "";

		while(true)
		{
			while(Serial.available() == 0);
			int ch = Serial.read();
			if (ch == '\n')
				break;
			s += (char)ch;
		}
		
		s.trim();
		s.toLowerCase();
		
		// repeat last command if current is empty
		if (s == "")
			s = old_command;
		else
			old_command = s;
		
		Serial.println(s);

		
		if (s == "help")
		{
			Serial.println(F("Available commands:"));
			Serial.println(F("  reset      - reset the controller"));
			Serial.println(F("  switches   - test limit switches only"));

			Serial.println(F("  enable/en  - enables power output"));
			Serial.println(F("  disable/di - disables power output"));

			Serial.println(F("  forward/f  - set forward direction (incremental)"));
			Serial.println(F("  backward/b - set backward direction (decremental)"));
			Serial.println(F("  step/s     - make a step in set direction"));
			
			

			continue;
		}
		
		
		if (s == "reset") {
			Serial.println("Resetting...");
			delay(1000);
			wdt_enable(WDTO_15MS);
			while(1);
		}
		
		if (s == "switches") {
			cmd_test_limit_switches();
			continue;
		}
		
		if (s == "enable" || s == "en") {
			MOTOR_ENABLE(true);
			Serial.println("Power enabled");
			continue;
		}

		if (s == "disable" || s == "di") {
			MOTOR_ENABLE(false);
			Serial.println("Power disabled");
			continue;
		}

		if (s == "forward" || s == "f") {
			MOTOR_SET_DIRECTION(+1);
			continue;
		}

		if (s == "backward" || s == "b") {
			MOTOR_SET_DIRECTION(-1);
			continue;
		}

		if (s == "step" || s == "s") {
			MOTOR_PULSE;
			continue;
		}
		
		
		Serial.print(F(" Command '"));
		Serial.print(s);
		Serial.print(F("' unknown; Maybe 'help'?\n"));
	}

	

	while(0)
	{
		bool left = digitalReadFast(PIN_LIMIT_LEFT);
		bool right = digitalReadFast(PIN_LIMIT_RIGHT);

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
