//////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012 by Byron Watkins <ByronWatkins@clear.net>
// List library for arduino.
//
// This file is free software; you can redistribute it and/or modify
// it under the terms of either the GNU General Public License version 2
// or the GNU Lesser General Public License version 2.1, both as
// published by the Free Software Foundation.
//////////////////////////////////////////////////////////////////////////////////////
/// List.h - Header file for List class.
///
/// Usage:   1  Instantiate a List object.  The maximum list length is determined by
///             MAX_LIST_PTRS defined below.
///          2  Create struct objects and "Add" pointers to the structs to the List.
///             One may also "InsertAt" pointers to the struct.  Pointers to
///             structs can also be added using the listName[n]=pStruct operator.
///          3  "Remove" objects from the list.  This also moves List objects below
///             the removed object up one index number.
///          4  "GetCount" returns the current list length and can be used to point
///             to the first empty entry.
///          5  It is essential that users understand that List stores only the
///             pointers to the data and not the data itself.  If the object
///             containing the data is deleted or goes out of scope, then the
///             pointer in List becomes useless.
//////////////////////////////////////////////////////////////////////////////////////

#ifndef LIST_H
#define LIST_H

#include <inttypes.h>
#include <avr/io.h>
//#include <avr/pgmspace.h>

#ifndef MAX_LIST_PTRS
  #define MAX_LIST_PTRS 5     ///< Maximum length of List.
#endif

typedef void *pObject;	///< List holds pointers of pObject type.

class List
{
public:
    /// Constructor empties List and NULLs first entry.
    List();

    /// Destructor frees memory.
    virtual ~List();

    /// Add an object's pointer to the end of List.
    /**
		\param pData is the data pointer to be added to the list.
		\return the index of the new element.
    */
    uint8_t Add (pObject pData);

    /// Opens a space and inserts a new pointer into List.
    /**
		\param index will be the list index of the new element after insert.
		\param pData is the data pointer to be inserted at location index.
    */
    void InsertAt (const uint8_t index, pObject pData);

	/// Trade places with two elements in the list.
	/**
		\param i1 is the list index of one element to be swapped.
		\param i2 is the list index of the other element to be swapped.
	*/
    void Swap (const uint8_t i1, const uint8_t i2);

    /// Asks whether the list is full or whether more space is available.
    /**
		\return true if the list is full or false if more entries may be added.
    */
    bool isFull () const {return m_Counter == MAX_LIST_PTRS;}

    /// Delete a List entry and move up bottom of List to fill void.
    /**
		\param index is the index of the element to be deleted.
    */
    void Remove (const uint8_t index);

    /// Access a List entry for read or write.
    pObject operator [] (const uint8_t index);

    /// Retrieve the number of List entries.
    /**
		\return the number of elements presently stored in the list.
    */
    uint8_t GetCount () const;
protected:
private:
    uint8_t m_Counter;
    pObject m_ptrList [MAX_LIST_PTRS];
};

#endif // LIST_H
