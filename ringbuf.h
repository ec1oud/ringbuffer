/******************************************************************************
Copyright (c) 2019, Shawn Rutledge <s@ecloud.org>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

#pragma once

#include <stdint.h>

#ifndef MIN
#define MIN(x,y) (x<y)?x:y
#endif

/**
	Mini, memory-conserving ring buffer implementation.  Size <= 255;
	different sized ring buffers are different types.
*/
template <class RingBufEntry, uint8_t Size>
class RingBuf
{
protected:
	uint8_t consumeOffset;					  // Offset where next byte can be read
	uint8_t loadOffset;						  // Offset where next byte can be stored + 1
	bool empty;								  // true if count() must be 0, false if count() > 0
	RingBufEntry buf[Size];

	/*
		https://en.wikipedia.org/wiki/Circular_buffer#Circular_buffer_mechanics
		"When they are equal, the buffer is empty, and
		when the start is one greater than the end, the buffer is full"
		Here consumeOffset is the "start" and loadOffset is the "end":
		where _would_ the next element be consumed from, and
		where _would_ the next element be inserted, respectively.
		But loadOffset == consumeOffset == 0 doesn't always mean "empty"
		in this implementation, so we need to maintain the empty flag.
	*/

public:
	/**
		Construct an empty ring buffer.
	*/
	RingBuf() :
		consumeOffset(0),
		loadOffset(0),
		empty(true) { }

	/**
		@return the number of entries that are loaded into the ring buffer
	*/
	uint8_t count() const {
		int16_t ret = loadOffset - consumeOffset;
		if (ret < 0)
			ret += Size;
		if (!ret && !empty)
			return uint8_t(Size);
		return uint8_t(ret);
	}

	/**
		@return the number of unused entries in the ring buffer
	*/
	uint8_t available() const {
		return Size - count();
	}

	/**
		Look at the next available entry without consuming it.
		@return the next entry, or 0 if unavailable (better call count() first to find out)
	*/
	RingBufEntry peek() const {
		return buf[consumeOffset];
	}

	/**
		Look at the nth available entry without consuming it.
		@return the entry, or 0 if unavailable (better call count() first to find out)
	*/
	RingBufEntry at(uint16_t idx) const {
		uint16_t offset;
		offset = consumeOffset + idx;
		if (offset >= Size)
			offset -= Size;
		return buf[offset];
	}

	/**
		Look at the nth entry (counting from the beginning of the buffer) without consuming it.
		@return the entry
	*/
	RingBufEntry atAbs(uint16_t idx) const {
		uint16_t offset = idx;
		if (offset >= Size)
			offset -= Size;
		return buf[offset];
	}

	/**
		Discard count entries from the ring buffer.
		@return number of entries actually consumed
	*/
	uint8_t remove(uint8_t c) {
		uint8_t countWas = count();
		if (count() <= c)
			empty = true;
		uint16_t newConsume = (uint16_t)(consumeOffset) + (uint16_t)(MIN(c, count()));
		if (newConsume >= Size)
			newConsume -= Size;
		//qDebug() << "removing" << c << "of" << count() << "consumeOffset" << consumeOffset << "->" << newConsume;
		consumeOffset = newConsume;
		if (empty)
			loadOffset = consumeOffset;
		//qDebug() << "count" << countWas << "->" << count();
		return countWas - count();
	}

	/**
		Take one entry from the ring buffer.
		@return the consumed entry, or 0 if unavailable (better call count() first to find out)
	*/
	RingBufEntry take() {
		uint16_t offset = consumeOffset;
		RingBufEntry ret = buf[offset];
		offset++;
		if (offset >= Size)
			offset -= Size;
		if (count() == 1)
			empty = true;
		consumeOffset = offset;
		return ret;
	}

	/**
		Put one entry into the ring buffer. If it's too full, the oldest data will be overwritten.
	*/
	void insert(RingBufEntry entry) {
		if (!available())
			remove(1);
		uint8_t lo = loadOffset;
		// Wrap around just prior to insertion, if we need to.
		if (lo >= Size)
			lo = 0;
		// It's OK to store one more.
		buf[lo] = entry;
		// Update loadOffset for next time.
		++lo;
		loadOffset = lo;
		empty = false;
	}

	/**
		Put one entry into the ring buffer, if possible.
		@return true if success, false if it's rejected (due to being full)
	*/
	bool insertIfOK(RingBufEntry entry) {
		bool ret = available();
		if (ret)
			insert(entry);
		return ret;
	}
};
