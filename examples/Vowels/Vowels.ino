//////////////////////////////////////////////////////////////////////
// Five timers are configured; each timer will send one of the five
// vowels over the serial port when it expires.  The timers will have
// different timeout intervals.  Once each second loop() will send
// a '\n' to prevent one infinitely long line of text.
//
// by Byron Watkins	<ByronWatkins@comcast.net>
//
// If we encourage them to do so, the keepers of the Arduino project
// might consider replacing or supplementing delay() and
// delayMicroseconds() with this library running on Timer/Counter 0
// thus freeing up Timer/Counter 2 for PWM, critical timing, or
// counting external events.
//////////////////////////////////////////////////////////////////////

#include <Timer.h>
Timer timer;

// Define a 'Character' class of objects to pass to callBack.
class Character {
public:
  Character (const char c) : _x (c) {}
  Character () : _x ('a') {}
  void changeCharacter (const char c) {_x = c;}
  void sendCharacter () const {Serial.write (_x);}
protected:
private:
  char _x;
};

// Create five Character objects and assign one vowel to each.
Character v [20] = {'a', 'e', 'i', 'o', 'u', '2', '8', '7', '5', '9',
                    'x', 'd', 'b', 'w', 'r', '?', '4', '6', 'g', '3'};

// The WriteIt function will be the timer CallBack function,
// will accept a (void *) pointer to one of our Character objects,
// and will send the contents of the object to the serial port.
void WriteIt (void * pArg) {
  static_cast<Character *>(pArg)->sendCharacter ();
}
// An alternate CallBack function to test modifyCallBack ()
void WriteItToo (void * pArg) {
  static_cast<Character *>(pArg)->sendCharacter ();
}

// Create five timeElement objects to keep up with the timer
// alarms and to call the sendCharacter () function.  The
// timeOut periods are prime numbers.
timeElement te [5] = {11, 13, 17, 23, 29};

void setup() {
  Serial.begin (9600);
  
  // Change the timer prescaler (p) to 1024.
  timer.configTimers (0b101);
  
  // Initialize and initiate the timers.
  for (uint8_t i=0; i<5; i++) {
    te [i].setArg (&v [i]);
    te [i].setCallBack (WriteIt);
    timer.startTimer (&te [i]);
  }
  // Wait for the user to set the random variable seed to some
  // semi-random value by sending something over the serial port.
  // The time it takes him to press "Send" will be different each
  // time and the string he sends will typically vary also.
  while (!Serial.available ()) ;
  
  // Read the string from the serial port.
  String rx;
  while (Serial.available ())
    rx += String (Serial.read ());
    
  randomSeed (timer.getPresentTime () - rx.length ());
}

void loop() {
  delay (1000);         // Wait for one second.
  
  // Keep things interesting by allowing the user to send something
  // over the serial port.  We ignore what is specifically sent;
  // however, the send itself causes the Character timers to be
  // modified in some random fashion.
  if (Serial.available ()) {
    // Read the data so only one change will occur per send.
    while (Serial.available ())
      Serial.read ();
    
    switch (random (13)) {
    case 0:  // startTimer:
      if (!timer.isFull ())
        Serial.println ("\nstartTimer");
      break;
    case 1:  // cancelTimer:
      Serial.println ("\ncancelTimer");
      if (timer.getCount ())
        timer.cancelTimer (&te [random (5)]);
      break;
    case 2:  // getPresentTime:
      Serial.print ("\ngetPresentTime: ");
      Serial.println (timer.getPresentTime (), HEX);
      break;
    case 3:  // getCount:
      Serial.print ("\ngetCount: ");
      Serial.println (timer.getCount (), DEC);
      break;
    case 4:  // getTimeOut:
      Serial.print ("\ngetTimeOut: ");
      Serial.println (te [0].getTimeOut (), HEX);
      break;
    case 5:  // modifyPeriod:
      Serial.println ("\nmodifyPeriod");
      te [random (5)].modifyPeriod (random (30, 50));
      break;
    case 6:  // modifyRepeats:
      Serial.println ("\nmodifyRepeats");
      te [random (5)].modifyRepeats (random (5, 20));
      break;
    case 7:  // modifyCallBack:
      Serial.println ("\nmodifyCallBack");
      if (random (2))
        te [0].modifyCallBack (WriteIt);
      else
        te [0].modifyCallBack (WriteItToo);
      break;
    case 8:  // modifyArg:
      Serial.println ("\nmodifyArg");
      te [random (5)].modifyArg (&v [random (20)]);
      break;
    case 9:  // modifyTimeOut:
      Serial.println ("\nmodifyTimeOut");
      te [random (5)].modifyTimeOut (random (0xFFFF));
      break;
    case 10:  // getTimePeriod:
      Serial.print ("\ngetTimePeriod: ");
      Serial.println (te [0].getTimePeriod ());
      break;
    case 11:  // getRemaining:
      Serial.print ("\ngetRemaining: ");
      Serial.println (te [0].getRemaining ());
      break;
    case 12:  // isFull:
      if (timer.isFull ())
        Serial.println ("\nIs Full");
      else
        Serial.println ("\nIs NOT Full");
      break;
    }
  } else
    Serial.print ("\n");  // End the line of vowels.
}
