//////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012 by Byron Watkins <ByronWatkins@clear.net>
// List library for arduino.
//
// This file is free software; you can redistribute it and/or modify
// it under the terms of either the GNU General Public License version 2
// or the GNU Lesser General Public License version 2.1, both as
// published by the Free Software Foundation.
//////////////////////////////////////////////////////////////////////////////////////
/// List.cpp - Source file for List class.
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

#include "List.h"

/// Constructor empties List and NULLs first entry.
List::List()
{
    m_ptrList [0] = static_cast<pObject>(0);
    m_Counter = 0;
}

/// Destructor frees memory.
List::~List()
{
    //dtor
}

/// Add an object's pointer to the bottom of List.  The object must be declared
/// static or global so it never goes out of scope.
/**
    \param pData A pointer to the data added to List.
    \return New number of elements in List.
*/
uint8_t List::Add (pObject pData)
{
    if (m_Counter < MAX_LIST_PTRS)
        m_ptrList [m_Counter++] = pData;

    return m_Counter;
}

/// Insert a new element into List at arbitrary position.
/**
    \param index The index number of the new element after insertion.
    \param pData The pointer to be inserted into the List.
*/
void List::InsertAt (const uint8_t index, pObject pData)
{
	if (m_Counter < MAX_LIST_PTRS)
	{
		for (uint8_t i=m_Counter; i>index; i--)
			m_ptrList [i] = m_ptrList [i-1];

		m_ptrList [index] = pData;
		m_Counter++;
	}
}

/// Exchange two elements in the List.
/**
	\param i1 The index number of the first element to be swapped.
	\param i2 The index number of the second element to be swapped.
*/
void List::Swap (const uint8_t i1, const uint8_t i2)
{
	pObject temp = m_ptrList [i1];
	m_ptrList [i1] = m_ptrList [i2];
	m_ptrList [i2] = temp;
}

/// Remove a specific element from List.
/**
    \param index The index number of the element to be deleted.
*/
void List::Remove (const uint8_t index)
{
	for (uint8_t i=index; i<m_Counter-1; i++)
		m_ptrList [i] = m_ptrList [i+1];

	if (index < m_Counter)
		m_Counter--;
}

/// Element accessor operator.  The element can then be read or written.
/// Using static_cast<>(), the returned element's members can also be
/// read or written.
/**
    \param index The index number of the element to be accessed.
    \return The element referenced by index is a pointer to the user's struct.
*/
pObject List::operator [] (const uint8_t index)
{
    return m_ptrList [index];
}

/// Get the number of elements presently held by List.
/**
	\return the number of elements already in the list.
*/
uint8_t List::GetCount () const
{
    return m_Counter;
}

