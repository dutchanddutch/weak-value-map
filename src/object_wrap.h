// Based on node_object_wrap.h, which is:
//
// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "v8.h"
#include <cassert>


class ObjectWrap {
 public:
  ObjectWrap() {
    refs_ = 0;
  }


  virtual ~ObjectWrap() {
    assert(refs_ == 0);
    assert(persistent().IsEmpty());
  }


  template <class T>
  static inline T* Unwrap(v8::Local<v8::Object> handle) {
    assert(!handle.IsEmpty());
    assert(handle->InternalFieldCount() > 0);
    // Cast to ObjectWrap before casting to T.  A direct cast from void
    // to T won't work right when T has more than one base class.
    void* ptr = handle->GetAlignedPointerFromInternalField(0);
    ObjectWrap* wrap = static_cast<ObjectWrap*>(ptr);
    return static_cast<T*>(wrap);
  }


  NODE_DEPRECATED("Use ObjectWrap::handle(isolate)",)
  inline v8::Local<v8::Object> handle() {
    return handle(v8::Isolate::GetCurrent());
  }


  inline v8::Local<v8::Object> handle(v8::Isolate* isolate) {
    return v8::Local<v8::Object>::New(isolate, persistent());
  }


  inline v8::Persistent<v8::Object>& persistent() {
    return handle_;
  }


 protected:
  NODE_DEPRECATED("Use ObjectWrap::Wrap(isolate, handle)",)
  inline void Wrap(v8::Local<v8::Object> handle) {
    Wrap(v8::Isolate::GetCurrent(), handle);
  }

  inline void Wrap(v8::Isolate* isolate, v8::Local<v8::Object> handle) {
    assert(persistent().IsEmpty());
    assert(handle->InternalFieldCount() > 0);
    handle->SetAlignedPointerInInternalField(0, this);
    persistent().Reset(isolate, handle);
    MakeWeak();
  }


  inline void MakeWeak() {
    persistent().SetWeak(this, WeakCallback, v8::WeakCallbackType::kParameter);
#if ( V8_MAJOR_VERSION < 7 )
    persistent().MarkIndependent();
#endif
  }

  /* Ref() marks the object as being attached to an event loop.
   * Refed objects will not be garbage collected, even if
   * all references are lost.
   */
  virtual void Ref() {
    assert(!persistent().IsEmpty());
#if ( V8_MAJOR_VERSION < 7 )
    assert(!persistent().IsNearDeath());
#endif
    persistent().ClearWeak();
    refs_++;
  }

  /* Unref() marks an object as detached from the event loop.  This is its
   * default state.  When an object with a "weak" reference changes from
   * attached to detached state it will be freed. Be careful not to access
   * the object after making this call as it might be gone!
   * (A "weak reference" means an object that only has a
   * persistent handle.)
   *
   * DO NOT CALL THIS FROM DESTRUCTOR
   */
  virtual void Unref() {
    assert(!persistent().IsEmpty());
#if ( V8_MAJOR_VERSION < 7 )
    assert(!persistent().IsNearDeath());
#endif
    assert(!persistent().IsWeak());
    assert(refs_ > 0);
    if (--refs_ == 0)
      MakeWeak();
  }

  int refs_;  // ro

 private:
  static void WeakCallback(
      const v8::WeakCallbackInfo<ObjectWrap>& data) {
    ObjectWrap* wrap = data.GetParameter();
    assert(wrap->refs_ == 0);
#if ( V8_MAJOR_VERSION < 7 )
    assert(wrap->handle_.IsNearDeath());
#endif
    wrap->handle_.Reset();
    data.SetSecondPassCallback(WeakCallback2);
  }

  static void WeakCallback2(
      const v8::WeakCallbackInfo<ObjectWrap>& data) {
    ObjectWrap* wrap = data.GetParameter();
    delete wrap;
  }

  v8::Persistent<v8::Object> handle_;
};
