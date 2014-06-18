#ifndef __BGJSMODULE_H
#define __BGJSMODULE_H  1

#include "BGJSInfo.h"
#include "BGJSClass.h"
#include "BGJSJavaWrapper.h"
#include <string>

/**
 * BGJSModule
 * Base class for an ejecta-v8 extension.
 *
 * Copyright 2014 Kevin Read <me@kevin-read.com> and BörseGo AG (https://github.com/godmodelabs/ejecta-v8/)
 * Licensed under the MIT license.
 */

class BGJSContext;

class BGJSModule : public BGJSInfo, public BGJSClass {
	// attributes
	std::string name;

protected:
	// methods
public:
	// attributes
	static BGJSContext* _bgjscontext;

	// members
	BGJSModule(const char* name);
	std::string getName() const;
	static void doRegister(BGJSContext *context);
	virtual ~BGJSModule() = 0;

	virtual bool initialize() = 0;
	virtual v8::Handle<v8::Value> initWithContext(BGJSContext* context) = 0;

	static void javaToJsField (const char* fieldName, const char fieldType, JNIEnv *env, jobject &jobj, v8::Handle<v8::Object> &jsObj);

};

#endif
