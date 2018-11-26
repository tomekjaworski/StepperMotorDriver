#include "digitalWriteFast.h"

#define LED				13

#define MOTOR_ENABLE		2
#define MOTOR_DIRECTION		3
#define MOTOR_PULSE			4

#define LEFT				7
#define RIGHT				8


void setup() {
	// put your setup code here, to run once:
	Serial.begin(9600);
	Serial.println("Sterownik silnika krokowego");

	pinModeFast(LED, OUTPUT);

	pinModeFast(MOTOR_ENABLE, OUTPUT);
	pinModeFast(MOTOR_DIRECTION, OUTPUT);
	pinModeFast(MOTOR_PULSE, OUTPUT);

	pinModeFast(LEFT, INPUT);
	pinModeFast(RIGHT, INPUT);
}

long current_position = 0;
long target_position = 1000;	

int start_delay = 100;
int stop_delay = 10;
int step_delay = 2;
int nsteps = (start_delay - stop_delay) / step_delay;

void loop() {
	// put your main code here, to run repeatedly:

	digitalWriteFast(MOTOR_ENABLE, false);
	digitalWriteFast(MOTOR_DIRECTION, false);
	digitalWriteFast(MOTOR_PULSE, false);



  
  
  


	while(1)
	{
		bool left = digitalReadFast(LEFT);
		bool right = digitalReadFast(RIGHT);

		char tab[10];
		sprintf(tab, "L%dR%d", left, right);
		Serial.println(tab);
	}


	while (1)
	{
		digitalWriteFast(MOTOR_PULSE, true);
		digitalWriteFast(LED, true);
		delay(5);
		digitalWriteFast(MOTOR_PULSE, false);
		digitalWriteFast(LED, false);
		delay(5);
	}

}
