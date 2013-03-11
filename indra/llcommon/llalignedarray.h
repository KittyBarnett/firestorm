/** 
 * @file llalignedarray.h
 * @brief A static array which obeys alignment restrictions and mimics std::vector accessors.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLALIGNEDARRAY_H
#define LL_LLALIGNEDARRAY_H

#include "llmemory.h"

#if LL_WINDOWS
#include "llvector4a.h" // for 16b fast copy
#endif

template <class T, U32 alignment>
class LLAlignedArray
{
public:
	T* mArray;
	U32 mElementCount;
	U32 mCapacity;

	LLAlignedArray();
	~LLAlignedArray();

	void push_back(const T& elem);
	U32 size() const { return mElementCount; }
	void resize(U32 size);
	T* append(S32 N);
	T& operator[](int idx);
	const T& operator[](int idx) const;
};

template <class T, U32 alignment>
LLAlignedArray<T, alignment>::LLAlignedArray()
{
	llassert(alignment >= 16);
	mArray = NULL;
	mElementCount = 0;
	mCapacity = 0;
}

template <class T, U32 alignment>
LLAlignedArray<T, alignment>::~LLAlignedArray()
{
	ll_aligned_free(mArray);
	mArray = NULL;
	mElementCount = 0;
	mCapacity = 0;
}

template <class T, U32 alignment>
void LLAlignedArray<T, alignment>::push_back(const T& elem)
{
	T* old_buf = NULL;
	if (mCapacity <= mElementCount)
	{
		mCapacity++;
		mCapacity *= 2;
		T* new_buf = (T*) ll_aligned_malloc(mCapacity*sizeof(T), alignment);
		if (mArray)
		{
#if LL_WINDOWS
			LLVector4a::memcpyNonAliased16((F32*) new_buf, (F32*) mArray, sizeof(T)*mElementCount);
#else
			memcpy((F32*)new_buf, (F32*)mArray, sizeof(T)*mElementCount);
#endif
		}
		old_buf = mArray;
		mArray = new_buf;
	}

	mArray[mElementCount++] = elem;

	//delete old array here to prevent error on a.push_back(a[0])
	ll_aligned_free(old_buf);
}

template <class T, U32 alignment>
void LLAlignedArray<T, alignment>::resize(U32 size)
{
	if (mCapacity < size)
	{
		mCapacity = size+mCapacity*2;
		T* new_buf = mCapacity > 0 ? (T*) ll_aligned_malloc(mCapacity*sizeof(T), alignment) : NULL;
		if (mArray)
		{
#if LL_WINDOWS
			LLVector4a::memcpyNonAliased16((F32*) new_buf, (F32*) mArray, sizeof(T)*mElementCount);
#else
			memcpy((F32*) new_buf, (F32*) mArray, sizeof(T)*mElementCount);
#endif
			ll_aligned_free(mArray);
		}

		/*for (U32 i = mElementCount; i < mCapacity; ++i)
		{
			new(new_buf+i) T();
		}*/
		mArray = new_buf;
	}

	mElementCount = size;
}


template <class T, U32 alignment>
T& LLAlignedArray<T, alignment>::operator[](int idx)
{
	llassert(idx < mElementCount);
	return mArray[idx];
}

template <class T, U32 alignment>
const T& LLAlignedArray<T, alignment>::operator[](int idx) const
{
	llassert(idx < mElementCount);
	return mArray[idx];
}

template <class T, U32 alignment>
T* LLAlignedArray<T, alignment>::append(S32 N)
{
	U32 sz = size();
	resize(sz+N);
	return &((*this)[sz]);
}

#endif

