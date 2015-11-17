//
//  Writer.cc
//  Fleece
//
//  Created by Jens Alfke on 2/4/15.
//  Copyright (c) 2015 Couchbase. All rights reserved.
//
//  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//  except in compliance with the License. You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0
//  Unless required by applicable law or agreed to in writing, software distributed under the
//  License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
//  either express or implied. See the License for the specific language governing permissions
//  and limitations under the License.

#include "Writer.hh"
#include <assert.h>


namespace fleece {

    Writer::Writer(size_t initialCapacity)
    :_buffer(::malloc(initialCapacity), initialCapacity),
     _available(_buffer)
    {
        if (!_buffer.buf)
            throw std::bad_alloc();
    }

    Writer::Writer(const Writer& w)
    :_buffer(w.output().copy()),
     _available(_buffer.end(), (size_t)0)
    { }

    Writer::Writer(Writer&& w)
    :_buffer(w._buffer),
     _available(w._available)
    {
        w._buffer = slice::null; // stop w's destructor from freeing the buffer
    }

    Writer::~Writer() {
        ::free((void*)_buffer.buf);
    }

    Writer& Writer::operator= (Writer&& w) {
        _buffer = w._buffer;
        _available = w._available;
        w._buffer = slice::null; // stop w's destructor from freeing the buffer
        return *this;
    }

    void Writer::write(const void* data, size_t length) {
        if (_available.size < length)
            growToFit(length);
        ::memcpy((void*)_available.buf, data, length);
        _available.moveStart(length);
    }

    size_t Writer::reserveSpace(size_t length) {
        size_t pos = this->length();
        if (_available.size < length)
            growToFit(length);
        _available.moveStart(length);
        return pos;
    }


    void Writer::rewrite(size_t pos, slice data) {
        assert(pos+data.size <= length());
        ::memcpy((void*)&_buffer[pos], data.buf, data.size);
    }

    void Writer::growToFit(size_t amount) {
        size_t len = length();
        size_t newSize = _buffer.size ?: kDefaultInitialCapacity/2;
        do {
            newSize *= 2;
        } while (len + amount > newSize);
        void* newBuf = ::realloc((void*)_buffer.buf, newSize);
        if (!newBuf)
            throw std::bad_alloc();
        _buffer = _available = slice(newBuf, newSize);
        _available.moveStart(len);
    }

    slice Writer::extractOutput() {
        void* buf = (void*)_buffer.buf;
        size_t len = length();
        if (len < _buffer.size)
            buf = ::realloc(buf, len);
        _buffer = _available = slice::null;
        return slice(buf, len);
    }

}