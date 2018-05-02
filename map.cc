#include <node.h>
#include "object_wrap.h"
#include "utility.h"
#include <unordered_map>
#include <tuple>
#include <string>

using Args	= v8::FunctionCallbackInfo<v8::Value> const;
using PropArgs	= v8::PropertyCallbackInfo<v8::Value> const;

template< class T = v8::Value > using Global = v8::Global<T>;
template< class T = v8::Value > using Local  = v8::Local<T>;


struct Entry;

class EntryOwner {
private:
	friend class Entry;
	virtual void garbage_collect( Entry &entry ) = 0;
};

using Key = std::string;

struct Entry {
	EntryOwner &owner;
	Key const key;
	Global<> value;

	static void gc_callback( v8::WeakCallbackInfo<Entry> const &info ) {
		auto &entry = *info.GetParameter();
		entry.owner.garbage_collect( entry );
	}

	void set( v8::Isolate *isolate, Local<> handle ) {
		value.Reset( isolate, handle );
		value.SetWeak( this, gc_callback, v8::WeakCallbackType::kParameter );
		value.MarkIndependent();
	}

	Entry( EntryOwner &owner, Key const &key ) : owner{owner}, key{key} {};
};


using MapBase = std::unordered_map<Key, Entry>;

class WeakValueMap : public ObjectWrap, public EntryOwner, private MapBase {
private:
	void garbage_collect( Entry &entry ) final override {
		erase( entry.key );
	}

	template< typename Callback >
	void find( Key const &key, Callback callback ) {
		auto i = MapBase::find( key );
		if( i != end() )
			callback( i->second );
	}

	Entry &find_or_insert( Key const &key ) {
		auto i = emplace( std::piecewise_construct,
				std::forward_as_tuple( key ),
				std::forward_as_tuple( *this, key ) ).first;
		return i->second;
	}

public:
	//-------- constructor ----------------------------------------------------------------

	static void constructor( Args &args ) {
		auto isolate = args.GetIsolate();

		//Make sure this is a new-call or throw a type error
		if (!args.IsConstructCall()) {
			auto msg = intern_string( isolate, "Class constructor "
					"WeakValueMap cannot be invoked without 'new'" );
			isolate->ThrowException(v8::Exception::TypeError(msg));
			return;
		}

		auto obj = new WeakValueMap();
		obj->Wrap( isolate, args.This() );
		args.GetReturnValue().Set( args.This() );
	}

	//-------- callback wrappers ----------------------------------------------------------

	template< void (WeakValueMap::*getter)( PropArgs & ) >
	static void wrap( Local<v8::String> prop, PropArgs &args ) {
		auto obj = ObjectWrap::Unwrap<WeakValueMap>( args.Holder() );
		(obj->*getter)( args );
	}

	template< void (WeakValueMap::*method)( Args &, Key const & ) >
	static void wrap( Args &args ) {
		auto isolate = args.GetIsolate();
		auto context = isolate->GetCurrentContext();

		auto key = Local<v8::String>{};
		if( ! args[0]->ToString( context ).ToLocal( &key ) )
			return;

		auto &&keybuf = v8::String::Utf8Value{ isolate, key };
		assert( *keybuf != NULL );

		auto obj = ObjectWrap::Unwrap<WeakValueMap>( args.Holder() );
		(obj->*method)( args, *keybuf );
	}

	//-------- properties -----------------------------------------------------------------

	void size_getter( PropArgs &args ) {
		args.GetReturnValue().Set( (uint32_t) size() );
	}

	//-------- methods --------------------------------------------------------------------

	void get_method( Args &args, Key const &key ) {
		find( key, [&]( Entry &entry ) {
			args.GetReturnValue().Set( entry.value );
		});
	}

	void set_method( Args &args, Key const &key ) {
		//Delete from the map if the value to insert is undefined
		if( args[1]->IsUndefined() )
			return delete_method( args, key );

		auto &entry = find_or_insert( key );
		entry.set( args.GetIsolate(), args[1] );

		args.GetReturnValue().Set( args.This() );
	}

	void delete_method( Args &args, Key const &key ) {
		erase( key );

		args.GetReturnValue().Set( args.This() );
	}
};


//-------- module initialization --------------------------------------------------------------

void initialize( Local<v8::Object> exports, Local<v8::Object> module,
		Local<v8::Context> context )
{
	ObjectBuilder exp { context, exports };

	exp.add_class( "WeakValueMap", WeakValueMap::constructor, []( ClassBuilder cls ) {
		cls.set_internal_field_count( 1 );
		cls.add_property( "size", WeakValueMap::wrap<&WeakValueMap::size_getter> );
		cls.add_method( "get",    WeakValueMap::wrap<&WeakValueMap::get_method> );
		cls.add_method( "set",    WeakValueMap::wrap<&WeakValueMap::set_method> );
		cls.add_method( "delete", WeakValueMap::wrap<&WeakValueMap::delete_method> );
	});
}

NODE_MODULE_CONTEXT_AWARE( WeakValueMap, initialize )
