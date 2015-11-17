//
//  Writer.hh
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

#ifndef Fleece_Writer_hh
#define Fleece_Writer_hh

#include "slice.hh"

namespace fleece {

    /** A simple write-only stream that buffers its output into a slice.
        (Used instead of C++ ostreams because those have too much overhead.) */
    class Writer {
    public:
        static const size_t kDefaultInitialCapacity = 256;

        Writer(size_t initialCapacity =kDefaultInitialCapacity);
        Writer(const Writer&);
        Writer(Writer&&);
        ~Writer();

        Writer& operator= (Writer&&);

        size_t length() const                   {return _buffer.size - _available.size;}

        /** Returns the data written, without copying.
            The returned slice becomes invalid as soon as any more data is written. */
        slice output() const                    {return slice(_buffer.buf, length());}
        
        /** Returns the data written. The Writer stops managing the memory; it is the caller's
            responsibility to free the returned slice's buf! */
        slice extractOutput();

        void write(const void* data, size_t length);

        Writer& operator<< (uint8_t byte)       {return operator<<(slice(&byte,1));}
        Writer& operator<< (slice s)            {write(s.buf, s.size); return *this;}

        /** Reserves space for data without actually writing anything yet.
            The data must be written later using rewrite() otherwise there will be garbage in
            the output. */
        size_t reserveSpace(size_t length);

        /** Overwrites already-written data.
            @param pos  The position in the output at which to start overwriting
            @param newData  The data that replaces the old */
        void rewrite(size_t pos, slice newData);

    private:
        void growToFit(size_t);
        const Writer& operator=(const Writer&); // forbidden

        slice _buffer;
        slice _available;
    };

}

#endif /* Fleece_Writer_hh */
