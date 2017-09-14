//
//  Array.hh
//  Fleece
//
//  Created by Jens Alfke on 5/12/16.
//  Copyright (c) 2016 Couchbase. All rights reserved.
//

#pragma once

#include "Value.hh"

namespace fleece {

    class Dict;

    /** A Value that's an array. */
    class Array : public Value {
        struct impl {
            const Value* _first;
            uint32_t _count;
            bool _wide;

            impl(const Value*) noexcept;
            const Value* second() const noexcept      {return _first->next(_wide);}
            bool next();
            const Value* firstValue() const noexcept  {return _count ? Value::deref(_first, _wide) : nullptr;}
            const Value* operator[] (unsigned index) const noexcept;
            size_t indexOf(const Value *v) const noexcept;
        };

    public:

        /** The number of items in the array. */
        uint32_t count() const noexcept;

        /** Accesses an array item. Returns nullptr for out of range index.
            If you're accessing a lot of items of the same array, it's faster to make an
            iterator and use its sequential or random-access accessors. */
        const Value* get(uint32_t index) const noexcept;

        /** An empty Array. */
        static const Array* const kEmpty;

        /** A stack-based array iterator */
        class iterator {
        public:
            iterator(const Array* a) noexcept;

            /** Returns the number of _remaining_ items. */
            uint32_t count() const noexcept                  {return _a._count;}

            const Value* value() const noexcept              {return _value;}
            explicit operator const Value* const () noexcept {return _value;}
            const Value* operator-> () const noexcept        {return _value;}

            /** Returns the current item and advances to the next. */
            const Value* read() noexcept                     {auto v = _value; ++(*this); return v;}

            /** Random access to items. Index is relative to the current item.
                This is very fast, faster than array::get(). */
            const Value* operator[] (unsigned i) noexcept    {return _a[i];}

            /** Returns false when the iterator reaches the end. */
            explicit operator bool() const noexcept          {return _a._count > 0;}

            /** Steps to the next item. (Throws if there are no more items.) */
            iterator& operator++();

            /** Steps forward by one or more items. (Throws if stepping past the end.) */
            iterator& operator += (uint32_t);

        private:
            const Value* rawValue() noexcept                 {return _a._first;}

            impl _a;
            const Value *_value;
            
            friend class Value;
        };

        iterator begin() const noexcept                      {return iterator(this);}

        constexpr Array()  :Value(internal::kArrayTag, 0, 0) { }

    private:
        friend class Value;
        friend class Dict;
        template <bool WIDE> friend struct dictImpl;
    };

}
