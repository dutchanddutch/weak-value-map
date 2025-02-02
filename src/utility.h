#pragma once
#include <v8.h>

inline v8::PropertyAttribute operator | ( v8::PropertyAttribute a, v8::PropertyAttribute b ) {
	return (v8::PropertyAttribute)( (int)a | (int)b );
}

inline v8::PropertyAttribute &operator |= ( v8::PropertyAttribute &a, v8::PropertyAttribute b ) {
	return a = a | b;
}


inline v8::Local<v8::String> intern_string( v8::Isolate *isolate, char const *s ) {
	auto type = v8::NewStringType::kInternalized;
	return v8::String::NewFromUtf8( isolate, s, type ).ToLocalChecked();
}


struct ClassBuilder {
	v8::Isolate *isolate;
	v8::Local<v8::FunctionTemplate> cls;

	void set_internal_field_count( int value ) {
		cls->InstanceTemplate()->SetInternalFieldCount( value );
	}

	void add_method( char const *name, v8::FunctionCallback callback,
			v8::PropertyAttribute attributes = v8::DontEnum )
	{
		auto signature = v8::Signature::New( isolate, cls );
		auto fun = v8::FunctionTemplate::New( isolate, callback, {}, signature );
		auto fun_name = intern_string( isolate, name );
		auto prototype = cls->PrototypeTemplate();
		prototype->Set( fun_name, fun, attributes );
	}

	void add_accessor( char const *name, v8::AccessorGetterCallback getter,
			v8::AccessorSetterCallback setter = NULL,
			v8::PropertyAttribute attributes = v8::DontEnum )
	{
		auto prop_name = intern_string( isolate, name );
		auto instance = cls->InstanceTemplate();
		if( ! setter )
			attributes |= v8::ReadOnly;
		instance->SetAccessor( prop_name, getter, setter, {}, v8::DEFAULT, attributes );
	}

	void add_accessor( char const *name, v8::AccessorNameGetterCallback getter,
			v8::AccessorNameSetterCallback setter = NULL,
			v8::PropertyAttribute attributes = v8::DontEnum )
	{
		auto prop_name = intern_string( isolate, name );
		auto instance = cls->InstanceTemplate();
		if( ! setter )
			attributes |= v8::ReadOnly;
		instance->SetAccessor( prop_name, getter, setter, {}, v8::DEFAULT, attributes );
	}
};

struct ObjectBuilder {
	v8::Local<v8::Context> context;
	v8::Local<v8::Object> target;

	void add_class( char const *name, v8::FunctionCallback constructor,
			void (*body)( ClassBuilder ) )
	{
		auto isolate = context->GetIsolate();
		auto cls = v8::FunctionTemplate::New( isolate, constructor, {} );
		auto cls_name = intern_string( isolate, name );
		cls->SetClassName( cls_name );

		body( { isolate, cls } );

		auto fun = cls->GetFunction( context ).ToLocalChecked();
		target->Set( context, cls_name, fun ).ToChecked();
	}

	void add_method( char const *name, v8::FunctionCallback callback,
			v8::PropertyAttribute attributes = v8::DontEnum )
	{
		auto isolate = context->GetIsolate();
		auto tmpl = v8::FunctionTemplate::New( isolate, callback, {} );
		auto fun_name = intern_string( isolate, name );

		auto fun = tmpl->GetFunction( context ).ToLocalChecked();
		target->Set( context, fun_name, fun ).ToChecked();
	}

	void add_property( char const *name, v8::Local<v8::Value> value,
			v8::PropertyAttribute attributes = v8::None )
	{
		auto isolate = context->GetIsolate();
		auto prop_name = intern_string( isolate, name );
		target->DefineOwnProperty( context, prop_name, value, attributes ).ToChecked();
	}

	void add_accessor( char const *name, v8::AccessorNameGetterCallback getter,
			v8::AccessorNameSetterCallback setter = NULL,
			v8::PropertyAttribute attributes = v8::DontEnum )
	{
		auto isolate = context->GetIsolate();
		auto prop_name = intern_string( isolate, name );
		if( ! setter )
			attributes |= v8::ReadOnly;
		target->SetAccessor( context, prop_name, getter, setter, {}, v8::DEFAULT, attributes ).ToChecked();
	}
};
