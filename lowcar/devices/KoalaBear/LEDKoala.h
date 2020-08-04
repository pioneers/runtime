#ifndef LEDKOALA_H
#define LEDKOALA_H

#include "pindefs_koala.h"

//class for controlling the LED's on the KoalaBear
class LEDKoala {
public:
	//Constructor
	LEDKoala();

	void ctrl_LEDs (float vel, float deadband, bool enabled);
	void test_LEDs ();
	void setup_LEDs ();
private:
	bool red_state;
	bool yellow_state;
	bool green_state;

	void ctrl_red (float vel, float deadband, bool enabled);
	void ctrl_yellow (float vel, float deadband, bool enabled);
	void ctrl_green (float vel, float deadband, bool enabled);
};

#endif /* LEDKOALA_H */
