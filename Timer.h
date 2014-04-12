//////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012 by Byron Watkins <ByronWatkins@clear.net>
// Timer library for arduino.
//
// This file is free software; you can redistribute it and/or modify
// it under the terms of either the GNU General Public License version 2
// or the GNU Lesser General Public License version 2.1, both as
// published by the Free Software Foundation.
//////////////////////////////////////////////////////////////////////////////////////
/// Timer.h - Header file for an interrupt driven timer class.
///
/// Usage:  1  If the default prescaler is insufficient to the desired usage, Call
///            'configTimers' to configure a suitable prescaler.  The timers will
///				expire in intervals, T, given by
///
///										256 * p * x
///								T = ---------------
///											F_CPU
///
///				where p is the prescaler and x is the timeElement.setPeriod (x) value.
///         2  Instantiate an object of type timeElement.
///         3  Populate the object with the timer period, number of times to repeat
///            the alarm (0=infinite), a pointer (timerCallBack_t) to a function, and a
///            user defined (i.e. arbitrary) object pointer.  This user-defined object
///            pointer will be delivered as the sole argument to the call-back function
///            each time the timer expires.
///         4  Call the 'startTimer' function with the pointer to this populated
///            timeElement object's pointer as its argument.
///         5  Call the 'cancelTimer' function in the event that the timer should be
///            terminated.  The argument for 'cancelTimer' should be the pointer to
///            the same timeElement object that was sent to 'startTimer'.
///         6  Up to 'MAX_LIST_PTRS' timers can be executing simultaneously.
///            'MAX_LIST_PTRS' is defined to be 5 in List.h if the user does not define
///				 it first.  The default value (5) can be changed at the user's
///            discretion; RAM can be freed by its reduction or more timers can be
///            available by its increase.
///			7	While a timer is running, its parameters can be modified using the
///				timeElement.modifyXxxx () functions.  Using the timeElement.setXxxx ()
///				functions while the timer is running can cause abnormal behavior.
//////////////////////////////////////////////////////////////////////////////////////

#ifndef TIMER_H
#define TIMER_H

#include "List.h"

#include <inttypes.h>
#include <avr/io.h>
#include <util/atomic.h>

typedef void (*timerCallBack_t)(void *);

/// timeElement stores all the information needed for the smooth functioning of the
/// interrupt driven timer class.
///
class timeElement
{
public:
/// Constructs a timeElement object to hold everything needed to operate a timer.
/**
	\param p is the timer's timeout period and defaults to 0x7FFF.
	\param r is the number of times the timer will time out before self canceling;
			 r defaults to 0 (infinite and never stops).
*/
	timeElement (uint16_t p=0x7fff, uint16_t r=0)
		: _timePeriod (p), _repeats (r) {}

	/// Set the timeout period for the timer.  Do NOT use this function if the timer
	/// has already been started (startTimer); use modifyPeriod () instead.
	/**
		\param p is the timer's new timeout period.
	*/
	void setPeriod (uint16_t p) {_timePeriod = p;}

	/// Set the number of times the timer will time out before self canceling.
	/**
		\param r is the new number of repeats.  0 means never stop.
	*/
	void setRepeats (uint16_t r) {_repeats = r;}

	/// Set the function to be called each time the timer expires.
	/**
		\param cb is a pointer to the function called in the ISR.
		\sa modifyCallBack, setArg, modifyArg.
	*/
	void setCallBack (timerCallBack_t cb) {_callBack = cb;}

	/// Set the argument passed to CallBack function.
	/**
		\param a is a (void *) pointer to the data passed to CallBack.
	*/
	void setArg (void *a) {_arg = a;}

	/// Set the presentTime needed for the timer to expire.  Ordinarily,
	/// users will not need this function and timer.getPresentTime ()
	/// will also be needed to use it effectively.
	/**
		\param to is the value presentTime will need to be to trigger the timer.
		\sa getPresentTime, updateTimeOut
	*/
	void setTimeOut (uint16_t to) {_timeOut = to & 0x7FFF;}

	/// Replace the timer's timeout period.
	/**
		\param p is the new value of the timeout period.
	*/
	void modifyPeriod (uint16_t p)
	{
		ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
			_timePeriod = p;
	}

	/// Replace the number of times the timer will expire before self canceling.
	/**
		\param r is the new number of times the timer will expire.
	*/
	void modifyRepeats (uint16_t r)
	{
		ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
			_repeats = r;
	}

	/// Replace the CallBack function.
	/**
		\param cb is a pointer to the new CallBack function.
		\sa timerCallBack_t
	*/
	void modifyCallBack (timerCallBack_t cb)
	{
		ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
			_callBack = cb;
	}

	/// Replace the argument sent to the CallBack function.
	/***
		\param a is a pointer to the new CallBack function argument.
		\sa timerCallBack_t
	*/
	void modifyArg (void *a)
	{
		ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
			_arg = a;
	}

	/// Replace the timeout time for the timer.  Normally the user will not use
	/// this function and timer.getPresentTime () will be needed to use this
	/// function effectively.
	/**
		\param to is the new value of the timeout time for the timer.
		\sa getPresentTime
	*/
	void modifyTimeOut (uint16_t to)
	{
		ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
			_timeOut = to & 0x7FFF;
	}

	/// Request the timeout period for the timer.
	/**
		\return _timePeriod for the timer.
	*/
	uint16_t getTimePeriod () const {return _timePeriod;}

	/// Request the time when the timer will next expire.
	/**
		\return _timeOut is the value of _presentTime when the timer will expire.
	*/
	uint16_t getTimeOut () const {return _timeOut;}

	/// Request the number of repeats before the timer will self-cancel.
	/**
		\return _repeats is the number if expirations remaining to the timer.
	*/
	uint16_t getRemaining () const {return _repeats;}

	/// Adds _timePeriod to _timeOut.  Normally, this function is called during
	/// a timer alarm to prepare the timer for the next expiration.  The user
	/// might possibly use this function to skip an alarm if he has already
	/// executed the CallBack function.  This does NOT modify _repeats, and so
	/// might result in extra repetetions.  Executing this strategy too early
	/// for very long period timers may also result in an earlier timeout
	/// instead of a skipped alarm.
	int16_t updateTimeOut ();

	/// Execute the CallBack function and send the correct argument.  This
	/// does not change _repeats and might result in extra alarms being
	/// executed.
	void callFunction () {(*_callBack) (_arg);}

	/// Execute the CallBack function with the correct argument and decrement
	/// _repeats so that no extra alarms will be executed.
	/**
		\return true if more repetitions are needed or false if the timer needs
				  canceled.
	*/
	bool clockAlarm ();
	timeElement & operator= (timeElement &s);
protected:
private:
	uint16_t _timeOut,	///< The future expiration time and is not needed by user.
	_timePeriod,     		///< The number of ticks between callbacks.
	_repeats;				///< The number of times the timer should call callBack
								///<   function.  0 means never stop until canceled.
	timerCallBack_t _callBack;  ///< Set to the callback function address.

	void *_arg;    /// A pointer to a struct that is passed along to callBack.
	///<   Inside the callback function, this pointer can be static_cast<>()
	///<   to the data structure type and used.  An arbitrary amount of
	///<   information can be passed to the callback function by packaging
	///<   it into a suitable static or global structure and placing its
	///<   pointer in _arg.
};
typedef timeElement *p_timeElement;

#define GET_TIMEOUT(x) static_cast<p_timeElement>(_timeOutList[x])->getTimeOut ()

class Timer
{
friend inline void Timer2ISR ();   ///< The interrupt service routine needs member access.

public:
	/// The constructor initializes the hardware and empties the List.
	Timer();

	/// The destructor disables the interrupt.
	virtual ~Timer();

	/// Add a timer to the List.
	void startTimer (const p_timeElement pArg /**< Points to user filled timeElement.*/);

	/// Remove a timer from the List.
	void cancelTimer (const p_timeElement pTE /**< Same pointer sent to startTimer.*/);

	/// Changes the length of every clock tick.
	void configTimers (const uint8_t prescaler /**< 0 <= prescaler <= 7*/);

	/// Make the time sent as argument larger than presentTime.
	/**
		\param time is usually a timeout value with 0 most significant bit.
		\return a value greater than presentTime even if setting the most significant
				  bit is needed.
	*/
	uint16_t normalizeTimeOut (const uint16_t time) const;

	/// Request the presentTime of the interrupt clock.
	/**
		\return _presentTime
	*/
	uint16_t getPresentTime () const {return _presentTime;}

	/// Request the number of timers already started and store in the ordered list.
	/**
		\return _timeOutList.GetCount ()
	*/
	uint8_t  getCount () const {return _timeOutList.GetCount ();}

	/// Request the timeout time for an indexed timer.
	/**
		\param i is the index of the relevant timer in the timeOutList.
		\return the value of presentTime needed for the timer to expire.
	*/
	uint16_t getTimeOut (uint8_t i) {return GET_TIMEOUT (i);}

	/// Asks whether the timeOutList is full or whether more timers can be started.
	/**
		\return true if no more timers can be started or false if the list has more space.
	*/
	bool isFull () const {return _timeOutList.isFull ();}

protected:
	inline void NextTick ();   ///< Called by ISR to increment _presentTime & call back
	uint8_t Search (const p_timeElement pArg); ///< Find _timeOutList insertion for pArg.
	void InsertTimer (const p_timeElement pArg);	///< Find the correct place in timeOutList
										///< for the timer pointed to by pArg and insert it into the list.

private:
	List _timeOutList;   ///< Stores pointers to timeElement structures
	uint16_t _presentTime;	///< The interrupt clock.
};

//extern Timer timer;
//Timer timer;

#endif // TIMER_H
