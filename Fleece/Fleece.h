//
// Fleece.h
//
// Copyright (c) 2016 Couchbase, Inc All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once
#ifndef _FLEECE_H
#define _FLEECE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

    // This is the C API! For the C++ API, see Fleece.hh.


    /** \defgroup Fleece Fleece
        @{ */

    /** \name Types and Basic Functions
        @{ */


    //////// TYPES

#ifndef FL_IMPL
    typedef const struct _FLValue* FLValue;         ///< A reference to a value of any type.
    typedef const struct _FLArray* FLArray;         ///< A reference to an array value.
    typedef const struct _FLDict*  FLDict;          ///< A reference to a dictionary (map) value.
    typedef struct _FLEncoder*     FLEncoder;       ///< A reference to an encoder
    typedef struct _FLSharedKeys*  FLSharedKeys;    ///< A reference to a shared-keys mapping
    typedef struct _FLKeyPath*     FLKeyPath;       ///< A reference to a key path
#endif

    /** A simple reference to a block of memory. Does not imply ownership. */
    typedef struct FLSlice {
        const void *buf;
        size_t size;
    } FLSlice;

    /** Creates a slice pointing to the contents of a C string. */
    static inline FLSlice FLStr(const char *str) {
        FLSlice foo = { str, str ? strlen(str) : 0 };
        return foo;
    }

    // Macro version of FLStr, for use in initializing compile-time constants.
    // STR must be a C string literal.
    #ifdef _MSC_VER
    #define FLSTR(STR) {("" STR), sizeof(("" STR))-1}
    #else
    #define FLSTR(STR) ((FLSlice){("" STR), sizeof(("" STR))-1})
    #endif

    // A convenient constant denoting a null slice.
    #ifdef _MSC_VER
    static const FLSlice kFLSliceNull = { NULL, 0 };
    #else
    #define kFLSliceNull ((FLSlice){nullptr, 0})
    #endif


#define FL_SLICE_DEFINED


    /** A block of memory returned from an API call. The caller takes ownership, may modify the
        bytes, and must call FLSliceFree when done. */
    typedef struct FLSliceResult {
        void *buf;      // note: not const, since caller owns the buffer
        size_t size;

#ifdef __cplusplus
        explicit operator FLSlice () const {return {buf, size};}
#endif
    } FLSliceResult;

    typedef FLSlice FLString;
    typedef FLSliceResult FLStringResult;

    /** Frees the memory of a FLSliceResult. */
    void FLSliceResult_Free(FLSliceResult);


    /** Equality test of two slices. */
    bool FLSlice_Equal(FLSlice a, FLSlice b);

    /** Lexicographic comparison of two slices; basically like memcmp(), but taking into account
        differences in length. */
    int FLSlice_Compare(FLSlice, FLSlice);


    /** Types of Fleece values. Basically JSON, with the addition of Data (raw blob). */
    typedef enum {
        kFLUndefined = -1,  // Type of a nullptr FLValue (i.e. no such value)
        kFLNull = 0,
        kFLBoolean,
        kFLNumber,
        kFLString,
        kFLData,
        kFLArray,
        kFLDict
    } FLValueType;


    /** Output formats a FLEncoder can generate. */
    typedef enum {
        kFLEncodeFleece,
        kFLEncodeJSON,
        kFLEncodeJSON5
    } FLEncoderFormat;

    
    typedef enum {
        kFLNoError = 0,
        kFLMemoryError,        // Out of memory, or allocation failed
        kFLOutOfRange,         // Array index or iterator out of range
        kFLInvalidData,        // Bad input data (NaN, non-string key, etc.)
        kFLEncodeError,        // Structural error encoding (missing value, too many ends, etc.)
        kFLJSONError,          // Error parsing JSON
        kFLUnknownValue,       // Unparseable data in a Value (corrupt? Or from some distant future?)
        kFLInternalError,      // Something that shouldn't happen
        kFLNotFound,           // Key not found
        kFLSharedKeysStateError, // Misuse of shared keys (not in transaction, etc.)
    } FLError;

    /** @} */


    //////// VALUE


    /** \name Parsing And Converting Values
        @{ */

    /** Returns a reference to the root value in the encoded data.
        Validates the data first; if it's invalid, returns nullptr.
        Does NOT copy or take ownership of the data; the caller is responsible for keeping it
        intact. Any changes to the data will invalidate any FLValues obtained from it. */
    FLValue FLValue_FromData(FLSlice data);

    /** Returns a pointer to the root value in the encoded data, _with only minimal validation_;
        or returns nullptr if the validation failed.
        This is significantly faster than FLValue_FromData, but should only be used if the data was
        generated by a trusted encoder and has not been altered or corrupted. For example, this
        can be used to parse Fleece data previously stored in a local database, as long as the
        database record has some kind of integrity check like a CRC32.
        If invalid data is read by this call, subsequent calls to Value accessor functions can
        crash or return bogus results (including data from arbitrary memory locations.) */
    FLValue FLValue_FromTrustedData(FLSlice data);

    /** Directly converts JSON data to Fleece-encoded data.
        You can then call FLValueFromTrustedData to get the root as a Value. */
    FLSliceResult FLData_ConvertJSON(FLSlice json, FLError *outError);

    /** Produces a human-readable dump of the Value encoded in the data. */
    FLStringResult FLData_Dump(FLSlice data);

    /** @} */
    /** \name Value Accessors
        @{ */

    // Value accessors -- safe to call even if the value is nullptr.

    /** Returns the data type of an arbitrary Value.
        (If the value is a nullptr pointer, returns kFLUndefined.) */
    FLValueType FLValue_GetType(FLValue);

    /** Returns true if the value is non-nullptr and is represents an integer. */
    bool FLValue_IsInteger(FLValue);

    /** Returns true if the value is non-nullptr and represents an _unsigned_ integer that can only
        be represented natively as a `uint64_t`. In that case, you should not call `FLValueAsInt`
        because it will return an incorrect (negative) value; instead call `FLValueAsUnsigned`. */
    bool FLValue_IsUnsigned(FLValue);

    /** Returns true if the value is non-nullptr and represents a 64-bit floating-point number. */
    bool FLValue_IsDouble(FLValue);

    /** Returns a value coerced to boolean. This will be true unless the value is nullptr (undefined),
        null, false, or zero. */
    bool FLValue_AsBool(FLValue);

    /** Returns a value coerced to an integer. True and false are returned as 1 and 0, and
        floating-point numbers are rounded. All other types are returned as 0.
        Warning: Large 64-bit unsigned integers (2^63 and above) will come out wrong. You can
        check for these by calling `FLValueIsUnsigned`. */
    int64_t FLValue_AsInt(FLValue);

    /** Returns a value coerced to an unsigned integer.
        This is the same as `FLValueAsInt` except that it _can't_ handle negative numbers, but
        does correctly return large `uint64_t` values of 2^63 and up. */
    uint64_t FLValue_AsUnsigned(FLValue);

    /** Returns a value coerced to a 32-bit floating point number. */
    float FLValue_AsFloat(FLValue);

    /** Returns a value coerced to a 64-bit floating point number. */
    double FLValue_AsDouble(FLValue);

    /** Returns the exact contents of a string value, or null for all other types. */
    FLString FLValue_AsString(FLValue);

    /** Returns the exact contents of a data value, or null for all other types. */
    FLSlice FLValue_AsData(FLValue);

    /** If a FLValue represents an array, returns it cast to FLArray, else nullptr. */
    FLArray FLValue_AsArray(FLValue);

    /** If a FLValue represents a dictionary, returns it as an FLDict, else nullptr. */
    FLDict FLValue_AsDict(FLValue);

    /** Returns a string representation of any scalar value. Data values are returned in raw form.
        Arrays and dictionaries don't have a representation and will return nullptr. */
    FLStringResult FLValue_ToString(FLValue);

    /** Encodes a Fleece value as JSON (or a JSON fragment.) 
        Any Data values will become base64-encoded JSON strings. */
    FLStringResult FLValue_ToJSON(FLValue);

    /** Encodes a Fleece value as JSON5, a more lenient variant of JSON that allows dictionary
        keys to be unquoted if they're alphanumeric. This tends to be more readable. */
    FLStringResult FLValue_ToJSON5(FLValue v);

    /** Most general Fleece to JSON converter. */
    FLStringResult FLValue_ToJSONX(FLValue v,
                                  FLSharedKeys sk,
                                  bool json5,
                                  bool canonicalForm);


    /** Converts valid JSON5 to JSON. */
    FLStringResult FLJSON5_ToJSON(FLString json5, FLError *error);

    //////// ARRAY


    /** @} */
    /** \name Arrays
        @{ */

    /** Returns the number of items in an array, or 0 if the pointer is nullptr. */
    uint32_t FLArray_Count(FLArray);

    /** Returns true if an array is empty. Slightly faster than `FLArray_Count(a) == 0` */
    bool FLArray_IsEmpty(FLArray);

    /** Returns an value at an array index, or nullptr if the index is out of range. */
    FLValue FLArray_Get(FLArray, uint32_t index);


    /** Opaque array iterator. Put one on the stack and pass its address to
        `FLArrayIteratorBegin`. */
    typedef struct {
        void* _private1;
        uint32_t _private2;
        bool _private3;
        void* _private4;
    } FLArrayIterator;

    /** Initializes a FLArrayIterator struct to iterate over an array.
        Call FLArrayIteratorGetValue to get the first item, then FLArrayIteratorNext. */
    void FLArrayIterator_Begin(FLArray, FLArrayIterator*);

    /** Returns the current value being iterated over. */
    FLValue FLArrayIterator_GetValue(const FLArrayIterator*);

    /** Returns a value in the array at the given offset from the current value. */
    FLValue FLArrayIterator_GetValueAt(const FLArrayIterator*, uint32_t offset);

    /** Returns the number of items remaining to be iterated, including the current one. */
    uint32_t FLArrayIterator_GetCount(const FLArrayIterator* i);

    /** Advances the iterator to the next value, or returns false if at the end. */
    bool FLArrayIterator_Next(FLArrayIterator*);


    //////// DICT


    /** @} */
    /** \name Dictionaries
        @{ */

    /** Returns the number of items in a dictionary, or 0 if the pointer is nullptr. */
    uint32_t FLDict_Count(FLDict);

    /** Returns true if a dictionary is empty. Slightly faster than `FLDict_Count(a) == 0` */
    bool FLDict_IsEmpty(FLDict);

    /** Looks up a key in a _sorted_ dictionary, returning its value.
        Returns nullptr if the value is not found or if the dictionary is nullptr. */
    FLValue FLDict_Get(FLDict, FLSlice keyString);

    /** Looks up a key in a _sorted_ dictionary, using a shared-keys mapping.
        If the database has a shared-keys mapping, you MUST use this call instead of FLDict_Get.
        Returns nullptr if the value is not found or if the dictionary is nullptr. */
    FLValue FLDict_GetSharedKey(FLDict d, FLSlice keyString, FLSharedKeys sk);
    
    /** Gets a string key from an FLSharedKeys object given its integer encoding 
        (for use when FLDictIterator_GetKey returns a value of type 'Number' */
    FLString FLSharedKey_GetKeyString(FLSharedKeys sk, int keyCode, FLError* outError);

    /** Looks up a key in an unsorted (or sorted) dictionary. Slower than FLDict_Get. */
    FLValue FLDict_GetUnsorted(FLDict, FLSlice keyString);


    /** Opaque dictionary iterator. Put one on the stack and pass its address to
        FLDictIterator_Begin. */
    typedef struct {
        void* _private1;
        uint32_t _private2;
        bool _private3;
        void* _private4[3];
    } FLDictIterator;

    /** Initializes a FLDictIterator struct to iterate over a dictionary.
        Call FLDictIterator_GetKey and FLDictIterator_GetValue to get the first item,
        then FLDictIterator_Next. */
    void FLDictIterator_Begin(FLDict, FLDictIterator*);

    /** Same as FLDictIterator_Begin but iterator will translate shared keys to strings. */
    void FLDictIterator_BeginShared(FLDict, FLDictIterator*, FLSharedKeys);

    /** Returns the current key being iterated over. */
    FLValue FLDictIterator_GetKey(const FLDictIterator*);

    /** Returns the current key's string value. */
    FLString FLDictIterator_GetKeyString(const FLDictIterator*);

    /** Returns the current value being iterated over. */
    FLValue FLDictIterator_GetValue(const FLDictIterator*);

    /** Returns the number of items remaining to be iterated, including the current one. */
    uint32_t FLDictIterator_GetCount(const FLDictIterator* i);

    /** Advances the iterator to the next value, or returns false if at the end. */
    bool FLDictIterator_Next(FLDictIterator*);


    /** Opaque key for a dictionary. You are responsible for creating space for these; they can
        go on the stack, on the heap, inside other objects, anywhere. 
        Be aware that the lookup operations that use these will write into the struct to store
        "hints" that speed up future searches. */
    typedef struct {
        void* _private1[4];
        uint32_t _private2, private3;
        bool _private4, private5;
    } FLDictKey;

    /** Initializes an FLDictKey struct with a key string.
        Warning: the input string's memory MUST remain valid for as long as the FLDictKey is in
        use! (The FLDictKey stores a pointer to the string, but does not copy it.)
        @param string  The key string (UTF-8).
        @param cachePointers  If true, the FLDictKey is allowed to cache a direct Value pointer
                representation of the key. This provides faster lookup, but means that it can
                only ever be used with Dicts that live in the same stored data buffer.
        @return  The opaque key. */
    FLDictKey FLDictKey_Init(FLSlice string, bool cachePointers);

    FLDictKey FLDictKey_InitWithSharedKeys(FLSlice string, FLSharedKeys sharedKeys);

    /** Returns the string value of the key (which it was initialized with.) */
    FLString FLDictKey_GetString(const FLDictKey *key);

    /** Looks up a key in a dictionary using an FLDictKey. If the key is found, "hint" data will
        be stored inside the FLDictKey that will speed up subsequent lookups. */
    FLValue FLDict_GetWithKey(FLDict, FLDictKey*);

    /** Looks up multiple dictionary keys in parallel, which can be much faster than individual lookups.
        @param dict  The dictionary to search
        @param keys  Array of keys; MUST be sorted as per FLSliceCompare.
        @param values  The found values will be written here, or NULLs for missing keys.
        @param count  The number of keys in keys[] and the capacity of values[].
        @return  The number of keys that were found. */
    size_t FLDict_GetWithKeys(FLDict dict, FLDictKey keys[], FLValue values[], size_t count);


    //////// PATH


    /** Creates a new FLKeyPath object by compiling a path specifier string. */
    FLKeyPath FLKeyPath_New(FLSlice specifier, FLSharedKeys, FLError *error);

    /** Frees a compiled FLKeyPath object. (It's ok to pass NULL.) */
    void FLKeyPath_Free(FLKeyPath);

    /** Evaluates a compiled key-path for a given Fleece root object. */
    FLValue FLKeyPath_Eval(FLKeyPath, FLValue root);

    /** Evaluates a key-path from a specifier string, for a given Fleece root object.
        If you only need to evaluate the path once, this is a bit faster than creating an
        FLKeyPath object, evaluating, then freeing it. */
    FLValue FLKeyPath_EvalOnce(FLSlice specifier, FLSharedKeys, FLValue root, FLError *error);


    //////// ENCODER


    /** @} */
    /** \name Encoder
        @{ */

    /** Creates a new encoder, for generating Fleece data. Call FLEncoder_Free when done. */
    FLEncoder FLEncoder_New(void);

    /** Creates a new encoder, allowing some options to be customized.
        @param format  The output format to generate (Fleece, JSON, or JSON5.)
        @param reserveSize  The number of bytes to preallocate for the output. (Default is 256)
        @param uniqueStrings  (Fleece only) If true, string values that appear multiple times will be written
            as a single shared value. This saves space but makes encoding slightly slower.
            You should only turn this off if you know you're going to be writing large numbers
            of non-repeated strings. Note also that the `cachePointers` option of FLDictKey
            will not work if `uniqueStrings` is off. (Default is true)
        @param sortKeys  (Fleece only) If true, dictionary keys are written in sorted order. This speeds up
            lookup, especially with large dictionaries, but slightly slows down encoding.
            You should only turn this off if you care about encoding speed but not access time,
            and will be writing large dictionaries with lots of keys. Note that if you turn
            this off, you can only look up keys with FLDictGetUnsorted(). (Default is true) */
    FLEncoder FLEncoder_NewWithOptions(FLEncoderFormat format,
                                       size_t reserveSize,
                                       bool uniqueStrings,
                                       bool sortKeys);

    /** Frees the space used by an encoder. */
    void FLEncoder_Free(FLEncoder);

    /** Tells the encoder to use a shared-keys mapping when encoding dictionary keys. */
    void FLEncoder_SetSharedKeys(FLEncoder, FLSharedKeys);

    /** Associates an arbitrary user-defined value with the encoder. */
    void FLEncoder_SetExtraInfo(FLEncoder e, void *info);

    /** Returns the user-defined value associated with the encoder; NULL by default. */
    void* FLEncoder_GetExtraInfo(FLEncoder e);


    /** Tells the encoder to create a delta from the given Fleece document, instead of a standalone
        document. Any calls to FLEncoder_WriteValue() where the value points inside the base data
        will write a pointer back to the original value. If `reuseStrings` is true, then writing a
        string that already exists in the base will just create a pointer back to the original.
        The resulting data returned by FLEncoder_Finish() will *NOT* be standalone; it can only
        be used by first appending it to the base data. */
    void FLEncoder_MakeDelta(FLEncoder e, FLSlice base, bool reuseStrings);

    /** Resets the state of an encoder without freeing it. It can then be reused to encode
        another value. */
    void FLEncoder_Reset(FLEncoder);

    // Note: The functions that write to the encoder do not return error codes, just a 'false'
    // result on error. The actual error is attached to the encoder and can be accessed by calling
    // FLEncoder_GetError or FLEncoder_End.
    // After an error occurs, the encoder will ignore all subsequent writes.

    /** Writes a `null` value to an encoder. (This is an explicitly-stored null, like the JSON
        `null`, not the "undefined" value represented by a nullptr FLValue pointer.) */
    bool FLEncoder_WriteNull(FLEncoder);

    /** Writes a boolean value (true or false) to an encoder. */
    bool FLEncoder_WriteBool(FLEncoder, bool);

    /** Writes an integer to an encoder. The parameter is typed as `int64_t` but you can pass any
        integral type (signed or unsigned) except for huge `uint64_t`s. */
    bool FLEncoder_WriteInt(FLEncoder, int64_t);

    /** Writes an unsigned integer to an encoder. This function is only really necessary for huge
        64-bit integers greater than or equal to 2^63, which can't be represented as int64_t. */
    bool FLEncoder_WriteUInt(FLEncoder, uint64_t);

    /** Writes a 32-bit floating point number to an encoder.
        (Note: as an implementation detail, if the number has no fractional part and can be
        represented exactly as an integer, it'll be encoded as an integer to save space. This is
        transparent to the reader, since if it requests the value as a float it'll be returned
        as floating-point.) */
    bool FLEncoder_WriteFloat(FLEncoder, float);

    /** Writes a 64-bit floating point number to an encoder.
        (Note: as an implementation detail, if the number has no fractional part and can be
        represented exactly as an integer, it'll be encoded as an integer to save space. This is
        transparent to the reader, since if it requests the value as a float it'll be returned
        as floating-point.) */
    bool FLEncoder_WriteDouble(FLEncoder, double);

    /** Writes a string to an encoder. The string must be UTF-8-encoded and must not contain any
        zero bytes.
        Do _not_ use this to write a dictionary key; use FLEncoder_WriteKey instead. */
    bool FLEncoder_WriteString(FLEncoder, FLString);

    /** Writes a binary data value (a blob) to an encoder. This can contain absolutely anything
        including null bytes. Note that this data type has no JSON representation, so if the
        resulting value is ever encoded to JSON via FLValueToJSON, it will be transformed into
        a base64-encoded string.
        If the encoder is generating JSON, the blob will be written as a base64-encoded string. */
    bool FLEncoder_WriteData(FLEncoder, FLSlice);

    /** Writes raw data to the encoded JSON output. 
        This can easily corrupt the output if you aren't careful!
        It will always fail with an error if the output format is Fleece. */
    bool FLEncoder_WriteRaw(FLEncoder, FLSlice);


    /** Begins writing an array value to an encoder. This pushes a new state where each
        subsequent value written becomes an array item, until FLEncoder_EndArray is called.
        @param reserveCount  Number of array elements to reserve space for. If you know the size
            of the array, providing it here speeds up encoding slightly. If you don't know,
            just use zero. */
    bool FLEncoder_BeginArray(FLEncoder, size_t reserveCount);

    /** Ends writing an array value; pops back the previous encoding state. */
    bool FLEncoder_EndArray(FLEncoder);


    /** Begins writing a dictionary value to an encoder. This pushes a new state where each
        subsequent key and value written are added to the dictionary, until FLEncoder_EndDict is
        called.
        Before adding each value, you must call FLEncoder_WriteKey (_not_ FLEncoder_WriteString!),
        to write the dictionary key.
        @param reserveCount  Number of dictionary items to reserve space for. If you know the size
            of the dictionary, providing it here speeds up encoding slightly. If you don't know,
            just use zero. */
    bool FLEncoder_BeginDict(FLEncoder, size_t reserveCount);

    /** Specifies the key for the next value to be written to the current dictionary. */
    bool FLEncoder_WriteKey(FLEncoder, FLString);

    /** Ends writing a dictionary value; pops back the previous encoding state. */
    bool FLEncoder_EndDict(FLEncoder);


    /** Writes a Fleece Value to an Encoder. */
    bool FLEncoder_WriteValue(FLEncoder, FLValue);

    /** Writes a Fleece Value that uses SharedKeys to an Encoder. */
    bool FLEncoder_WriteValueWithSharedKeys(FLEncoder, FLValue, FLSharedKeys);


    /** Parses JSON data and writes the object(s) to the encoder. (This acts as a single write,
        like WriteInt; it's just that the value written is likely to be an entire dictionary of
        array.) */
    bool FLEncoder_ConvertJSON(FLEncoder e, FLSlice json);


    /** Returns the number of bytes encoded so far. */
    size_t FLEncoder_BytesWritten(FLEncoder e);

    /** Ends encoding; if there has been no error, it returns the encoded data, else null.
        This does not free the FLEncoder; call FLEncoder_Free (or FLEncoder_Reset) next. */
    FLSliceResult FLEncoder_Finish(FLEncoder, FLError*);

    /** Returns the error code of an encoder, or NoError (0) if there's no error. */
    FLError FLEncoder_GetError(FLEncoder e);

    /** Returns the error message of an encoder, or nullptr if there's no error. */
    const char* FLEncoder_GetErrorMessage(FLEncoder e);

    
    /** @} */
    /** @} */

#ifdef __cplusplus
}
#endif


#ifdef __OBJC__
// Include Obj-C/CF utilities:
#include "Fleece+CoreFoundation.h"
#endif


#ifdef __cplusplus
// Now include the C++ convenience wrapper:
#include "FleeceCpp.hh"
#endif

#endif // _FLEECE_H
