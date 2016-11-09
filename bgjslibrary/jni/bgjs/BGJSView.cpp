#include "BGJSView.h"
#include "os-android.h"
#include "mallocdebug.h"
#include <v8.h>

#include "GLcompat.h"

#if defined __ANDROID__
#include "ClientAndroid.h"
#endif


// #define DEBUG_GL 1
#undef DEBUG_GL
// #define DEBUG 1
#undef DEBUG

#define LOG_TAG	"BGJSView"

using namespace v8;

static void checkGlError(const char* op) {
#ifdef DEBUG_GL
	for (GLint error = glGetError(); error; error = glGetError()) {
		LOGI("after %s() glError (0x%x)\n", op, error);
	}
#endif
}


void getWidth(Local<String> property,
              		const v8::PropertyCallbackInfo<Value>& info) {
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	void* ptr = wrap->Value();
	int value = static_cast<BGJSView*>(ptr)->width;
#ifdef DEBUG
	LOGD("getWidth %u", value);
#endif
	info.GetReturnValue().Set(value);
}

void setWidth(Local<String> property, Local<Value> value,
              		const v8::PropertyCallbackInfo<void>& info) {
}

void getHeight(Local<String> property,
               		const v8::PropertyCallbackInfo<Value>& info) {
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	void* ptr = wrap->Value();
	int value = static_cast<BGJSView*>(ptr)->height;
#ifdef DEBUG
	LOGD("getHeight %u", value);
#endif
	info.GetReturnValue().Set(value);
}

void getPixelRatio(Local<String> property,
                   		const v8::PropertyCallbackInfo<Value>& info) {
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	void* ptr = wrap->Value();
	float value = static_cast<BGJSView*>(ptr)->pixelRatio;

#ifdef DEBUG
	LOGD("getPixelRatio %f", value);
#endif
	info.GetReturnValue().Set(value);
}

void setPixelRatio(Local<String> property, Local<Value> value,
        const v8::PropertyCallbackInfo<void>& info) {
}

void setHeight(Local<String> property, Local<Value> value,
        const v8::PropertyCallbackInfo<void>& info) {
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	void* ptr = wrap->Value();
#ifdef DEBUG
	LOGI("Setting height to %d", value->Int32Value());
#endif
	static_cast<BGJSView*>(ptr)->height = value->Int32Value();
}

void BGJSView::js_view_on(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Isolate* isolate = args.GetIsolate();
    v8::Locker l(isolate);
    HandleScope scope(isolate);
	BGJSView *view = externalToClassPtr<BGJSView>(args.Data());
	LOGD("BGJSView.on: BGJSView instance is %p", view);
	if (args.Length() == 2 && args[0]->IsString() && args[1]->IsObject()) {
		Handle<Object> func = args[1]->ToObject();
		if (func->IsFunction()) {
			String::Utf8Value eventUtf8(args[0]->ToString());
			// v8::Persistent<v8::Object, v8::CopyablePersistentTraits<v8::Object> > funcPersist(isolate, func);
			v8::Persistent<v8::Object, v8::CopyablePersistentTraits<v8::Object> >* funcPersist = new v8::Persistent<v8::Object, v8::CopyablePersistentTraits<v8::Object> >(isolate, func);
			BGJS_NEW_PERSISTENT_PTR(funcPersist);
			const char *event = *eventUtf8;
			// LOGD("BGJSView.on: persist is %p, event is %s", funcPersist, event);
			if (strcmp(event, "event") == 0) {
				view->_cbEvent.push_back(funcPersist);
			} else if (strcmp(event, "close") == 0) {
				view->_cbClose.push_back(funcPersist);
			} else if (strcmp(event, "resize") == 0) {
				view->_cbResize.push_back(funcPersist);
			} else if (strcmp(event, "redraw") == 0) {
				view->_cbRedraw.push_back(funcPersist);
			}
		}
	}
	args.GetReturnValue().SetUndefined();
}

BGJSView::BGJSView(Isolate* isolate, const BGJSContext *ctx, float pixelRatio, bool doNoClearOnFlip) {
	HandleScope scope(isolate);
	opened = false;
	_contentObj = 0;

	this->noClearOnFlip = doNoClearOnFlip;

	this->pixelRatio = pixelRatio;

	//  new JS BGJSView object
	this->_jsContext = ctx;
	LOGD("BGJSView context is %p", ctx);
	v8::Local<v8::ObjectTemplate> bgjsgl = v8::ObjectTemplate::New(isolate);
	// bgjsglft->SetClassName(String::NewFromUtf8(isolate, "BGJSView"));
	// v8::Local<v8::ObjectTemplate> bgjsgl = bgjsglft->InstanceTemplate();
	bgjsgl->SetInternalFieldCount(1);
	bgjsgl->SetAccessor(String::NewFromUtf8(isolate, "width", NewStringType::kNormal).ToLocalChecked(), getWidth, setWidth);
	bgjsgl->SetAccessor(String::NewFromUtf8(isolate, "height", NewStringType::kNormal).ToLocalChecked(), getHeight, setHeight);
	bgjsgl->SetAccessor(String::NewFromUtf8(isolate, "devicePixelRatio", NewStringType::kNormal).ToLocalChecked(), getPixelRatio, setPixelRatio);

	// bgjsgl->SetAccessor(String::New("magnifierPoint"), getMagnifierPoint, setMagnifierPoint);
    // NODE_SET_METHOD(bgjsgl, "on", makeStaticCallableFunc(BGJSView::js_view_on));
    // EscapableHandleScope scope(Isolate::GetCurrent());
    // return scope.Escape(External::New(Isolate::GetCurrent(), reinterpret_cast<void *>(this)))
    Local<External> staticCallableThis = External::New(isolate, reinterpret_cast<void *>(this));
    Handle<FunctionTemplate> ft = FunctionTemplate::New(isolate, BGJSView::js_view_on, staticCallableThis);
    bgjsgl->Set(String::NewFromUtf8(isolate, "on"), ft);

    BGJS_RESET_PERSISTENT(isolate, this->jsViewOT, bgjsgl);
}

Handle<Value> BGJSView::startJS(Isolate* isolate, const char* fnName,
        const char* configJson, Handle<Value> uiObj, long configId, bool hasIntradayQuotes) {
    v8::Locker l(isolate);
    EscapableHandleScope scope(isolate);

	Handle<Value> config;

	if (configJson) {
		config = String::NewFromUtf8(isolate, configJson);
	} else {
		config = v8::Undefined(isolate);
	}

	Local<Object> objInstance = (*reinterpret_cast<Local<ObjectTemplate>*>(&this->jsViewOT))->NewInstance();
	LOGD("startJS. jsContext %p, jsC->c %p, objInstance %p configJson %s", this->_jsContext, BGJSInfo::_context.Get(isolate), objInstance, configJson ? configJson : "null");
	objInstance->SetInternalField(0, External::New(isolate, reinterpret_cast<void *>(this)));
	// Local<Object> instance = bgjsglft->GetFunction()->NewInstance();
	BGJS_RESET_PERSISTENT(isolate, this->_jsObj, objInstance);

	Handle<Value> argv[5] = { uiObj, objInstance, config, Number::New(isolate, configId),
	    Number::New(isolate, hasIntradayQuotes) };

	Local<Value> res = this->_jsContext->callFunction(isolate,
	        BGJSInfo::_context.Get(isolate)->Global(), fnName, 5,
			argv);
	if (res->IsNumber()) {
		_contentObj = res->ToNumber()->Value();
#ifdef DEBUG
		LOGD ("startJS return id %d", _contentObj);
#endif
	} else {
		LOGI ("Did not receive a return id from startJS");
	}
	return scope.Escape(res);
}

void BGJSView::sendEvent(Isolate* isolate, Handle<Object> eventObjRef) {
	if (!opened) {
		return;
	}

	TryCatch trycatch;
	HandleScope scope (isolate);

	Handle<Value> args[] = { eventObjRef };

	const int count = _cbEvent.size();

	// eventObjRef->Set(String::New("target"))


	for (std::vector<Persistent<Object, v8::CopyablePersistentTraits<v8::Object> >*>::size_type i = 0; i < count; i++) {
		if (!opened) {
			return;
		}

		Persistent<Object, v8::CopyablePersistentTraits<v8::Object> >* cb = _cbEvent[i];

		LOGD("BGJSView sendEvent call");


		// if (!cb->isEmpty()) {
		    Local<Object> callback = (*reinterpret_cast<Local<Object>*>(cb));
			Handle<Value> result = callback->CallAsFunction(callback, 1, args);
			if (result.IsEmpty()) {
				BGJSContext::ReportException(&trycatch);
			}
		// }
	}
}

void BGJSView::call(Isolate* isolate, std::vector<Persistent<Object, v8::CopyablePersistentTraits<v8::Object> >*> &list) {
	TryCatch trycatch;
	HandleScope scope(isolate);

	Handle<Value> args[] = { };

	const int count = list.size();

	for (std::vector<Persistent<Object, v8::CopyablePersistentTraits<v8::Object> >*>::size_type i = 0; i < count; i++) {
	    Persistent<Object, v8::CopyablePersistentTraits<v8::Object> >* cb = list[i];
	    Local<Object> callback = (*reinterpret_cast<Local<Object>*>(cb));
	    LOGD("BGJSView call call");
		Local<Value> result = callback->CallAsFunction(callback, 0, args);
		if (result.IsEmpty()) {
			BGJSContext::ReportException(&trycatch);
		}
	}
}

BGJSView::~BGJSView() {
	opened = false;
	// Dispose of permanent references to event listeners
	while (!_cbClose.empty()) {
		v8::Persistent<v8::Object, v8::CopyablePersistentTraits<v8::Object> >* item = _cbClose.back();
		_cbClose.pop_back();
		BGJS_CLEAR_PERSISTENT_PTR(item);
	}
	while (!_cbResize.empty()) {
		v8::Persistent<v8::Object, v8::CopyablePersistentTraits<v8::Object> >* item = _cbResize.back();
		_cbResize.pop_back();
		BGJS_CLEAR_PERSISTENT_PTR(item);
	}
	while (!_cbEvent.empty()) {
		v8::Persistent<v8::Object, v8::CopyablePersistentTraits<v8::Object> >* item = _cbEvent.back();
		_cbEvent.pop_back();
		BGJS_CLEAR_PERSISTENT_PTR(item);
	}
	while (!_cbRedraw.empty()) {
		v8::Persistent<v8::Object, v8::CopyablePersistentTraits<v8::Object> >* item = _cbRedraw.back();
		_cbRedraw.pop_back();
		BGJS_CLEAR_PERSISTENT_PTR(item);
	}
	BGJS_CLEAR_PERSISTENT(this->jsViewOT);
	BGJS_CLEAR_PERSISTENT(this->_jsObj);
}
