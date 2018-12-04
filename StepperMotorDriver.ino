/*
	Stepper motor driver for Arduino Uno
	Tomasz Jaworski, 2018
*/

#include "digitalWriteFast.h"
#include <avr/wdt.h>
#include <ctype.h>


#include <SPI.h>
#include <Ethernet.h>

#define LED						13

#define PIN_MOTOR_ENABLE		4
#define PIN_MOTOR_DIRECTION		5
#define PIN_MOTOR_PULSE			6

#define PIN_LIMIT_LEFT			2
#define PIN_LIMIT_RIGHT			3



enum class MotorDirection {
	Forward,
	Backward
};

enum class MotorPower {
	Disabled,
	Enabled
};

struct {
	int start_delay;
	int stop_delay;
	int step_delay;
	int n_delay_steps;
	int divider;
	
	struct {
		int speed;
		int speed2;
	} homing;
	
		
} config;


struct {
	long current_position;
	int delay;
	
	MotorDirection direction;
	MotorPower power;
} state;

struct {
  long target;	

  long topspeed_steps;
  long delta;

  long acc_steps;
  long decc_steps;

  int step_delay;
} task;

volatile struct {
	bool switch_left;
	bool switch_right;
	bool switch_any;
	
	bool soft_limit_left_active;
	bool soft_limit_right_active;
	long soft_limit_left;
	long soft_limit_right;
	
	
} limit = { 0 };

char sprintf_buffer[100];


// -- hardware stuff
#if defined(Arduino_h)
	bool __pulse_state = 0;

	#define DEBUG(...)
	#define MOTOR_SET_DIRECTION(__x)							\
		do {													\
			if ((__x) == 1) {									\
				state.direction = MotorDirection::Forward;		\
				digitalWriteFast(PIN_MOTOR_DIRECTION, false);	\
			}													\
			else {												\
				state.direction = MotorDirection::Backward;		\
				digitalWriteFast(PIN_MOTOR_DIRECTION, true);	\
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
			state.power = __const_value ? MotorPower::Enabled : MotorPower::Disabled; \
		} while (0);											\

		
		
#else
	// test code
	long __step = 0;
	#define MOTOR_SET_DIRECTION(__x) do { __step = __x; } while(0)
	#define MOTOR_PULSE state.current += __step;
	#define MOTOR_ENABLE(__const_value)

	#define DEBUG(...) printf(__VA_ARGS__)
#endif

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
EthernetServer server(23);
EthernetClient client;



void print(const char* str) {
	Serial.print("> ");
	client.print("> ");
}

void println(const char* str) {
	Serial.println("> ");
	client.println("> ");
}

void println(long int& value) {
	Serial.println(value);
	client.println(value);
}

void print(const String& str) {
	Serial.print(str);
	client.print(str);
}

void println(const String& str) {
	Serial.println(str);
	client.println(str);
}

void print(const __FlashStringHelper* str) {
	Serial.print(str);
	client.print(str);
}

void println(const __FlashStringHelper* str) {
	Serial.println(str);
	client.println(str);
}




bool ensure_motor_power(void) {
	if (state.power != MotorPower::Enabled) {
		println(F("Error: motor driver stage is not enabled"));
		return false;
	}
	return true;
}

bool ensure_soft_limits(void) {
	if (!limit.soft_limit_left_active) {
		println(F("Error: unknown home position; use 'findhome' command"));
		return false;
	}

	if (!limit.soft_limit_right_active) {
		println(F("Error: unknown maximum position; use 'findmax' command"));
		return false;
	}
	
	return true;
}

void do_accelerate(void)
{
	for (int i = 0; i < task.acc_steps; i++)
	{
		DEBUG("ACC-F CP=%ld; delay=%d\n", state.current_position, state.delay);

		// move
		MOTOR_PULSE;
		state.delay -= task.step_delay;
		delayMicroseconds(state.delay);
	}
  
	if (state.direction == MotorDirection::Forward)
		state.current_position += task.acc_steps;
	else
		state.current_position -= task.acc_steps;
}

void do_move(void)
{
	for (int i = 0; i < task.topspeed_steps; i++) {
		DEBUG("mov-F CP=%ld; delay=%d\n", state.current_position, state.delay);

		// move
		MOTOR_PULSE;
		delayMicroseconds(state.delay);
	}
	
	if (state.direction == MotorDirection::Forward)
		state.current_position += task.topspeed_steps;
	else
		state.current_position -= task.topspeed_steps;

}

void do_deccelerate(void) 
{
	for (int i = 0; i < task.decc_steps; i++) {
		DEBUG("DEC-F CP=%ld; delay=%d\n", state.current_position, state.delay);

		// move
		MOTOR_PULSE;
		state.delay += task.step_delay;
		delayMicroseconds(state.delay);
	}

	if (state.direction == MotorDirection::Forward)
		state.current_position += task.decc_steps;
	else
		state.current_position -= task.decc_steps;

}


void do_goto(long target)
{
	if (limit.soft_limit_left_active)
		target = max(limit.soft_limit_left, target);
	if (limit.soft_limit_right_active)
		target = min(limit.soft_limit_right, target);
	
	task.target = target;

	task.delta = task.target - state.current_position;
	task.acc_steps = min(abs(task.delta) / 2, config.n_delay_steps);
	task.decc_steps = min(abs(task.delta) / 2, config.n_delay_steps);
	task.step_delay = config.step_delay;


	DEBUG("acceleration_steps=%ld\n", task.acc_steps);
	DEBUG("decceleration_steps=%ld\n", task.decc_steps);

	task.topspeed_steps = abs(task.delta) - task.acc_steps - task.decc_steps;

	//sprintf(sprintf_buffer, "current=%ld; target=%ld\n", state.current_position, task.target);
	//Serial.print(sprintf_buffer);
	
	if (task.delta > 0)
		MOTOR_SET_DIRECTION(1)
	else
		MOTOR_SET_DIRECTION(-1);

	do_accelerate();
	do_move();
	do_deccelerate();

	DEBUG("FINAL STATE: position=%ld; delay=%d\n\n", state.current_position, state.delay);  
}



void do_goto_home(void) {

	limit.switch_any = limit.switch_left = limit.switch_right = false;
	
	// STAGE 1
	// Go for the left limit switch
	MOTOR_SET_DIRECTION(-1);
	delay(10);

	while (true) {
		// Did we hit it?
		if (limit.switch_left)
			break;
		
		// Nope, make next step
		delayMicroseconds(config.homing.speed);
		MOTOR_PULSE;
	}

	// Stabilize the chassis
	delay(1000);
	
	
	// STAGE 2
	// Get of the left limit switch.
	// It can take a several steps since most limit switches have switching hysteresis.
	MOTOR_SET_DIRECTION(+1);
	delay(10);
	
	while (true) {
		if (!digitalReadFast(PIN_LIMIT_LEFT))
		{
			delay(100);
			if (!digitalReadFast(PIN_LIMIT_LEFT))
				break;
		}
		
		delay(config.homing.speed2);
		MOTOR_PULSE;
	}

	// Stabilize the chassis, just to be sure
	delay(1000);
	limit.switch_any = limit.switch_left = limit.switch_right = false;
	
	state.current_position = 0;

	// set limiters
	limit.soft_limit_left = 0;
	limit.soft_limit_left_active = true;
}


void do_find_right_limit(void) {

	limit.switch_any = limit.switch_left = limit.switch_right = false;

	// STAGE 1
	// Go for the right limit switch
	MOTOR_SET_DIRECTION(+1);
	delay(10);	
	
	while (true) {
		if (limit.switch_right)
			break;
		
		MOTOR_PULSE;
		delayMicroseconds(config.homing.speed);
		state.current_position++;
	}
	
	// Stabilize
	delay(1000);
	
	// STAGE 2
	// Get of the right limit switch
	MOTOR_SET_DIRECTION(-1);
	delay(10);

	while (true) {
		if (!digitalReadFast(PIN_LIMIT_RIGHT))
			break;
		
		MOTOR_PULSE;

		delay(config.homing.speed2);
		state.current_position--;
	}	

		// set limiters
	limit.soft_limit_right = state.current_position;
	limit.soft_limit_right_active = true;

}
	

void cmd_test_limit_switches(void) {
	
	strcpy(sprintf_buffer, "\nL?R?"); // 2, 4
	
	while (Serial.available() == 0 && client.available() == 0)
	{
		sprintf_buffer[2] = '0' + !!digitalReadFast(PIN_LIMIT_LEFT);
		sprintf_buffer[4] = '0' + !!digitalReadFast(PIN_LIMIT_RIGHT);
		
		print(sprintf_buffer);
	}
	
	if (Serial.available())
		while (Serial.available())
			Serial.read();	
	if (client.available())
		while (client.available())
			client.read();	
}

void cmd_sweep(void) {
	if (!ensure_motor_power())
		return;
	if (!ensure_soft_limits())
		return;

	println("INFO: To quit sweeping type '++++++'...");
	
	int stage = 0;
	int plus_counter = 0;
	bool finish = false;
	while (!finish) {
		
		if (stage % 2 == 0)
			do_goto(0);
		if (stage % 2 == 1)
			do_goto(limit.soft_limit_right);
		
		stage++;
		
		while ((Serial.available() || client.available()) && !finish) {
			
			byte b;
			if (Serial.available())
				b = Serial.read();
			else
				if (client.available())
					b = client.read();
				
			if (b == '+') {
				plus_counter++;
				if (plus_counter >= 6)
					finish = true;
			} else
				plus_counter = 0;
		}
	}

	// clean serial rx buffer
	if (Serial.available())
		while (Serial.available())
			Serial.read();	
	if (client.available())
		while (client.available())
			client.read();		
	println(F("Done"));
}

void show_position(void)
{
	print("state.current_position=");
	println(state.current_position);
}

void isr_limit_left(void) {
	//Serial.print("l");
	limit.switch_left = true;
	limit.switch_any = true;
}

void isr_limit_right(void) {
	//Serial.print("r");
	limit.switch_right = true;
	limit.switch_any = true;
}

void setup() {
	wdt_disable();
	noInterrupts();
	
	Serial.begin(9600);
	pinModeFast(LED, OUTPUT);


	// setup driver pins
	pinModeFast(PIN_MOTOR_ENABLE, OUTPUT);
	pinModeFast(PIN_MOTOR_DIRECTION, OUTPUT);
	pinModeFast(PIN_MOTOR_PULSE, OUTPUT);

	digitalWriteFast(PIN_MOTOR_ENABLE, true);
	digitalWriteFast(PIN_MOTOR_DIRECTION, false);
	digitalWriteFast(PIN_MOTOR_PULSE, false);
	
	MOTOR_SET_DIRECTION(1);
	MOTOR_ENABLE(false);


	// setup inputs for limit switches
	pinModeFast(PIN_LIMIT_LEFT, INPUT);
	pinModeFast(PIN_LIMIT_RIGHT, INPUT);
	
	attachInterrupt(digitalPinToInterrupt(PIN_LIMIT_LEFT), isr_limit_left, RISING);
	attachInterrupt(digitalPinToInterrupt(PIN_LIMIT_RIGHT), isr_limit_right, RISING);
	

	// basic motor parameters setup
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
	
	config.homing.speed = 3000;
	config.homing.speed2 = 25;


	// initial conditions
	state.current_position = 0;
	state.delay = config.start_delay;
	
	// engage interrupts
	interrupts();
	
	
	// I'm alive!
	for (int i = 0; i < 10; i++)
	{
		Serial.print((char)('+' + (i & 1) * 2));
		delay(200);
	}
	
	
	// initialize the ethernet device
	Ethernet.begin(mac, ip, myDns, gateway, subnet);

	
	Serial.println();
	Serial.println(F("================================================================================="));
	Serial.println(F("# Stepper motor controller (motor 57HS22-A, driver HY-DIV-268N-5A)              #"));
	Serial.println(F("# v1.0 Tomasz Jaworski, 2018                                                    #"));
	Serial.println(F("================================================================================="));

	Serial.print(F("DHCP: Hardware address: "));
	for (byte octet = 0; octet < 6; octet++) {
		Serial.print(mac[octet], HEX);
		Serial.print((octet < 5) ? '-' : '\n');
	}
	
	// Talk to a DHCP server
	if (Ethernet.begin(mac)) {
		Serial.print(F("DHCP: device ip = "));
		Serial.println(Ethernet.localIP());	
	} else
		Serial.println(F("DHCP: Unable to acquire an IP address"));


	// Hardware error?
	if (Ethernet.hardwareStatus() == EthernetNoHardware) {
		Serial.println(F("ETHERNET: shield was not found. Aborted."));
		while (true)
			delay(10);
	}


	
	// start listening for clients
	server.begin();
	
	Serial.println(F("Commands should end only with the '\\n' character."));
	Serial.println(F("ARDUINO IDE: Change 'No line ending' to 'Newline' at the bottom part of your console"));
	Serial.println(F("INFO: Empty command repeats last one."));
	

}

template <typename T>
bool get_value(String& str, T& output) {
	str.trim();

	// is there anything interesting?
	if (str.length() == 0)
		return false;
	if (!isdigit(str.charAt(0)) && str.charAt(0) != '-')
		return false;
	
	char* endptr;
	output = strtol(str.c_str(), &endptr, 10);
	if (*endptr == '\0')
	{
		// There is no more to read; clear the input string
		str = "";
		return true;
	}
	if (isspace(*endptr))
	{
		// eat everything up until white-space
		str.remove(0, endptr - str.c_str() + 1);
		return true;
	}

	return false;
}

void loop() {

	String s = "";
	String old_command = "";
	
	while(1)
	{
		s = "";

		print("> ");

		while(true)
		{
			// if there is no data than check if there is a connection to be accepted
			EthernetClient new_client = server.accept();
	
			if (new_client)
			{
				client.stop();
				client = new_client;
				Serial.print(F("New remote client: "));
				Serial.print(client.remoteIP());
				Serial.print(F(":"));
				Serial.println(client.remotePort());
				
				client.println("Connected.");
			}
			
			if (client && !client.connected())
			{
				client.stop();
				Serial.println(F("Remote connection closed"));
			}
		
			// still no data?
			if (Serial.available() == 0 && client.available() == 0)
				continue; // wait some more
			
			if (Serial.available()) // Data on serial line?
			{
				int ch = Serial.read();
				if (ch == '\n')
					break;
				s += (char)ch;
			}
			
			if (client.available()) // Data from TCP client?
			{
				int ch = client.read();
				if (ch == '\n')
					break;
				s += (char)ch;
			}
			
		}
		
		s.trim();
		s.toLowerCase();
		
		// repeat last command if current is empty
		if (s == "")
			s = old_command;
		else
			old_command = s;
		
		println(s); // local echo

		
		if (s == "help")
		{
			println(F("Available commands:"));
			println(F("  reset       - reset the controller"));
			println(F("  switches    - test limit switches only"));
            
			println(F("  enable/en   - enables power output"));
			println(F("  disable/di  - disables power output"));
            
			println(F("  forward/f   - set forward direction (incremental)"));
			println(F("  backward/b  - set backward direction (decremental)"));
			println(F("  step/s      - make a step in set direction"));
			
			println(F("  position    - show current position"));
			println(F("  go P        - go to position P"));
			println(F("  sethome [P] - set home at position P (if given) or at current position (if not)"));
			println(F("  findhome    - home left until limit switch is found; then zero the position"));
			println(F("  findmax     - find max position (most to the right)"));
			
			
			continue;
		}
		
		
		if (s == "reset") {
			println(F("Resetting..."));
			
			MOTOR_ENABLE(false);
			
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
			println(F("Power enabled"));
			continue;
		}

		if (s == "disable" || s == "di") {
			MOTOR_ENABLE(false);
			println(F("Power disabled"));
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
			if (!ensure_motor_power())
				continue;
			
			MOTOR_PULSE;
			continue;
		}
		
		if (s.startsWith("go")) {
			if (!ensure_motor_power())
				continue;
			
			long position;
			s.remove(0, 2);
			if (!get_value<long>(s, position)) {
				println(F("Command 'go': Expected distance in steps"));
				continue;
			}
				
			do_goto(position);
			show_position();
			continue;
		}
		
		if (s.startsWith("sethome")) {
			long new_home;
			s.remove(0, 7);
			if (!get_value<long>(s, new_home)) {
				println(F("Command 'sethome': Expected distance in steps"));
				continue;
			}
				
			state.current_position -= new_home;
			show_position();
			continue;
		}
		
		if (s == "position") {
			show_position();
			continue;
		}

		if (s == "findhome") {
			if (!ensure_motor_power())
				continue;
			
			do_goto_home();
			
			sprintf(sprintf_buffer, "state.current_position=%ld\n", state.current_position);
			print(sprintf_buffer);
			continue;
		}
		
		if (s == "findmax") {
			if (!ensure_motor_power())
				continue;

			do_find_right_limit();
			sprintf(sprintf_buffer, "position.max=%ld\n", limit.soft_limit_right);
			print(sprintf_buffer);
			continue;
		}
		
		if (s == "sweep") {
			cmd_sweep();
			continue;
		}
		
		
		print(F(" Command '"));
		print(s);
		print(F("' unknown; Maybe 'help'?\n"));
	}
}


