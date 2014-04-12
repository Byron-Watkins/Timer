////////////////////////////////////////////////////////////////////////
// Blink three LEDs using hardware interrupt timers for timing.
// This leaves the processor free to perform other functions than
// executing wait loops.  To demonstrate that the timers are
// independent, the three LEDs blink at different rates.
//
// by Byron Watkins	<ByronWatkins@comcast.net>
//
// If we encourage them to do so, the keepers of the Arduino project
// might consider replacing or supplementing delay() and
// delayMicroseconds() with this library running on Timer/Counter 0
// thus freeing up Timer/Counter 2 for PWM, critical timing, or
// counting external events.
////////////////////////////////////////////////////////////////////////

#include <Timer.h> // This also creates the "timer" object.
Timer timer;

// A LED class of objects passed to callBack.
class LED {
public:
  LED (int pin) : blinkPin (pin) {}
  int getPinNumber () {return blinkPin;}
  void blinkMe () {
    ledState ^= true;
    digitalWrite (blinkPin, ledState);
  }
protected:
private:
  bool	ledState;
  int   blinkPin;
};

LED led[3] = {13, 12, 11};  // LEDs on pins 11-13.

// The callback function called at regular intervals.
// The argument is a pointer to the particular LED
// that needs blinked presently.
void callBack (void *pArg){
  static_cast<LED *>(pArg)->blinkMe ();
}

// The timer's data objects.  Assign the timers' periods.
// 0x100 = 256 decimal results in 1.049 second period with the
// default timer prescaler (256).  Use configTimers to change
// the prescaler.  Use setRepeats to to cause the timer to
// execute up to 65535 more alarms and to stop.  These three
// initializers result in pin 13 on/off in 0.9994 seconds
// each, pin 12 on/off in 0.49997 seconds each and pin 11
// on/off in 0.3318 seconds each by default.
//
//             256 * p * x
//         T = -----------
//               F_CPU
//
// The values (x) are assigned to timePeriod in the constructor.
timeElement timerData [3] = {0xF400, 0x7A00, 0x5100};

void setup()
{
  // Change the timer prescaler (p) to 1.  This will make the
  // timer resolution 16 microseconds.
  timer.configTimers (1);
  for (uint8_t i=0; i<3; i++) {
    pinMode (led [i].getPinNumber (), OUTPUT);
    timerData [i].setArg (&led [i]);
    timerData [i].setCallBack (callBack);
    timer.startTimer (&timerData [i]);
  }
}

void loop() // The processor can be doing anything here while the timers run.
{

}
