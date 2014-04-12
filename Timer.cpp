//////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012 by Byron Watkins <ByronWatkins@clear.net>
// Timer library for arduino.
//
// This file is free software; you can redistribute it and/or modify
// it under the terms of either the GNU General Public License version 2
// or the GNU Lesser General Public License version 2.1, both as
// published by the Free Software Foundation.
//////////////////////////////////////////////////////////////////////////////////////
/// Timer.cpp - Source file for an interrupt driven timer class.
///
/// Usage:  1  If the default prescaler is insufficient to the desired usage, Call
///            'configTimers' to configure a suitable prescaler.
///         2  Instantiate a structure of type timeElement.
///         3  Populate the structure with the timer period, number of times to repeat
///            the alarm (0=infinite), a pointer to a timerCallBack_t function, and a user
///            defined (i.e. arbitrary) structure pointer.  This user-defined structure
///            pointer will be delivered as the sole argument to the call-back function
///            each time the timer expires.
///         4  Call the 'startTimer' function with the pointer to this populated
///            timeElement structure's pointer as its argument.  'startTimer' returns a
///            byte ID to identify the timer for the 'cancelTimer' function.
///         5  Call the 'cancelTimer' function in the event that the timer should be
///            terminated.  The argument for 'cancelTimer' should be 'startTimer's'
///            return value.
///         6  Up to 'MAX_LIST_PTRS' timers can be executing simultaneously.  'MAX_LIST_PTRS' is
///            defined in List.h.  The default value can be changed at the user's
///            discretion; RAM can be freed by its reduction or more timers can be
///            available by its increase.
//////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <avr/interrupt.h>
#include "Timer.h"
#include <string.h>

extern Timer timer;

void inline Timer2ISR ()
{
    timer.NextTick();
}

ISR (TIMER2_OVF_vect)
{
    Timer2ISR ();
}

/// The interrupt service routine calls the timer.NextTick () member function each
/// time Timer/Counter 2 overflows.  NextTick () increments presentTime, executes
/// timeElement.clockAlarm () for each timer that expired, updateTimeOut () each
/// expired timer, and inserts the timer back into the sorted timerList.
/// timeElement.clockAlarm () executes the timerCallBack function specified by
/// the user in timeElement.setCallBack ().
/**
    \return A boolean value indicating whether to restart the timer again.
    \sa timerCallBack_t
*/
bool timeElement::clockAlarm ()
{
	callFunction ();
	if (0 != _repeats)		// _repeats == 0 means ad infinitum so don't change.
		if (0 == --_repeats)
			return false;		// timer expired for the last time.
	return true;
}

/// Each time the timer expires this function adds the timePeriod to the timeOut
/// time.  This sets the timer to expire again when presentTime catches up to
/// timeOut again.  All of the time registers roll over from 0x7fff to 0.
/**
    \return The timer's new timeOut time.
*/
int16_t timeElement::updateTimeOut ()
{
	return _timeOut = (_timePeriod + _timeOut) & 0x7fff;
}

/// Copy and assign a timeElement.
/**
	\param s is a reference to the source timeElement for the copy.
	\return a reference to the result of the copy to facilitate chain assignments.
*/
timeElement & timeElement::operator= (timeElement &s)
{
	memcpy (this, &s, sizeof (timeElement));
	return *this;
}

/// Configure the hardware to divide processor clock by 1, to interrupt the processor
/// when the counter register overflows from 0xFF to 0x00, and to enable interrupts.
/// Also, tag all elements in List as available.
///
/// By changing the prescaler and interrupt mode, the user has a great deal of control
/// over the timers' resolution and duration.  Such detailed control requires code
/// changes, but the prescaler can be changed using configTimers.
/// \sa configTimers
Timer::Timer()
{
    // Configure hardware timer for interrupt and default prescaler.
    TCCR2A = 0x00;  // Disable waveform generation and frequency construction.
    TCCR2B = 0x01;  // Set prescaler division to 1.
    TIMSK2 = 0x01;  // Enable interrupt on timer overflow.
}

///
/// Disable the interrupt for timer 2.
///
Timer::~Timer()
{
    TIMSK2 = 0;  // Disable timer interrupt.
}

/// Start another timer to callback a user's function with the user's parameters at user-defined
/// intervals for a user-defined number of times.
/**
    \param pArg a pointer to a timer structure that the user has filled with desired timer properties.
    \sa timeElement, cancelTimer, modifyTimer
*/
void Timer::startTimer (p_timeElement pArg)
{
	pArg->setTimeOut (_presentTime + pArg->getTimePeriod ()); // Set the expiration time.

	// Search the _timeOutList for correct insertion point to keep the list sorted.
	// This saves the interrupt service routine from needing to check every timer
	// for expiration; if timer[0] has not expired, then none have because the list
	// is sorted.

	InsertTimer (pArg);
}

/// Remove a specific timer from List and prevent additional alarms it might have caused.
/**
    \param pTE A reference to the timer to be canceled.
    \sa startTimer, modifyTimer
*/
void Timer::cancelTimer (const p_timeElement pTE)
{
    uint8_t i;

    // First, find the timer.
    for (i=0; i<_timeOutList.GetCount (); i++)
        if (static_cast<p_timeElement>((void *)_timeOutList[i]) == pTE)
            break;

    _timeOutList.Remove (i); // Remove the timer from the list.
}

/// Change the prescaler division of all timers.
/**
    \param prescaler The value written to the three lsb of TCCR2B.
    \sa Datasheet for your processor.  Reference TCCR2B.
*/
void Timer::configTimers (const uint8_t prescaler)
{
    TCCR2B = prescaler & 0x07;
}

/// Since the values of _presentTime and _timeOut roll over at 0x7FFF
/// it is difficult to compare two _timeOuts so that they can be sorted
/// into increasing order.  After all, if _presentTime > _timeOut_a, then
/// _presentTime must finish its count to 0x7FFF, roll over to 0, and
/// increase again to _timeOut before the alarm will execute; this is
/// much farther in the future than another timeout whose value has
/// _timeOut_b > _presentTime despite the fact that
/// _timeOut_a < _presentTime < _timeOut_b would yield the wrong
/// result.  The solution I have chosen is to reserve the most
/// significant bit of the word for the case when _presentTime > _timeOut_a
/// to make _timeOut_a + 0x8000 > _timeOut_b as it should be.
/**
	\param time is the value to be normalized.
	\return normalized value of presentTime when alarm will execute.
*/
uint16_t Timer::normalizeTimeOut (const uint16_t time) const
{
	uint16_t t;
	uint8_t __sreg = SREG;
	noInterrupts ();

	if (_presentTime > time) t = time | 0x8000;
	else t = time;

	SREG = __sreg;

	return t;
}

/// The NextTick () function is called by the interrupt service routine each time
/// Timer/Counter 2 overflows.  NextTick () increments presentTime, calls
/// timeElement.clockAlarm () for each expired timer, and re-inserts the timer
/// into the sorted timerList if another alarm is indicated.  NextTick () should
/// ONLY be called by the ISR if the clock is to keep correct time.  Since this
/// is part of an ISR, interrupts are already disabled.
/**
    \sa timeElement, timeElement.clockAlarm
*/
inline void Timer::NextTick ()
{
	_presentTime = (_presentTime + 1) & 0x7fff;  // Add one tick to clock.

	if (0 == getCount ()) return;		// No timers.

	// Check for expired timers.
	while (_presentTime == GET_TIMEOUT (0))
	{
		// Update the timed out timers... one timer each time through while loop.
		static_cast<p_timeElement>(_timeOutList[0])->updateTimeOut ();

		// Execute the CallBack function.
		static_cast<p_timeElement>(_timeOutList[0])->clockAlarm ();

		// Sort the list.
		for (uint8_t i=1; i < _timeOutList.GetCount (); i++)
			if (normalizeTimeOut (GET_TIMEOUT (i-1)) > normalizeTimeOut (GET_TIMEOUT (i)))
				_timeOutList.Swap (i-1, i);
			else
				break;
	}
}

/// Insert timer with pointer pArg into _timerList at the location that keeps the list
/// sorted in increasing timeout time order.  This function is for internal use only.
/**
	\param pArg is a pointer to the timerElement that needs inserted into the list.
*/
void Timer::InsertTimer (const p_timeElement pArg)
{
	uint8_t __sreg = SREG;

	if (_timeOutList.GetCount () > 0)
	{
		_timeOutList.InsertAt (Search (pArg), (pObject) pArg); // Re-insert into List.
	}
	else
		_timeOutList.Add ((pObject) pArg);

	SREG = __sreg;
}

/// Find the position to insert pArg into _timeOutList so that the list remains sorted
/// _timeOutList[0] will timeout first, _timeOutList[1] will timeout second, ...
/// This function is for internal use only.
/**
	\param pArg is a pointer to the timeElement that needs inserted into the list.
	\return the smallest index of timeElements having larger timeouts than pArg.
*/
uint8_t Timer::Search (const p_timeElement pArg)
{
	if (0 == _timeOutList.GetCount ())
		return 0;

	uint8_t i, __sreg = SREG;	// Prevent _presentTime changing.
	noInterrupts ();

	for (i=0; i<_timeOutList.GetCount (); i++)
		if (normalizeTimeOut (pArg->getTimeOut ()) < normalizeTimeOut (GET_TIMEOUT (i)))
			break;

	SREG = __sreg;			// Restore the processor's status register and interrupt mask.

	// Return the index of the first List element needing moved.
	return (i);
}
