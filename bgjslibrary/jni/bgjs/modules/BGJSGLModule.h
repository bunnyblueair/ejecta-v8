#ifndef __BGJSGLMODULE_H
#define __BGJSGLMODULE_H	1

#include "../BGJSCanvasContext.h"
#include "../BGJSModule.h"

class BGJSContext2dGL;

class BGJSGLModule : public BGJSModule {
	bool initialize();
	~BGJSGLModule();
	BGJSCanvasContext *_canvasContext;

public:
	BGJSGLModule();
	v8::Local<v8::Value> initWithContext(v8::Isolate* isolate, const BGJSContext* context);
	static void doRequire (v8::Isolate* isolate, v8::Handle<v8::Object> target);
	static void create(const v8::FunctionCallbackInfo<v8::Value>& args);
	static void js_canvas_constructor(const v8::FunctionCallbackInfo<v8::Value>& args);
	static void js_canvas_getContext(const v8::FunctionCallbackInfo<v8::Value>& args);
	static void js_context_destruct (const v8::WeakCallbackData<v8::Object, BGJSContext2dGL>& info);
	// static v8::Handle<v8::Value> js_context_beginPath(const v8::Arguments& args);
	// static Handle<Value> BGJSCanvasGL:js_context_beginPath(const Arguments& args)

	static v8::Persistent<v8::Function> g_classRefCanvasGL;
	static v8::Persistent<v8::Function> g_classRefContext2dGL;
};


#endif
