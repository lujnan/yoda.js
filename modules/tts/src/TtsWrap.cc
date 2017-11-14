#include "src/TtsWrap.h"
#include <tts/tts_client.h>

using namespace v8;
using namespace std;

uv_async_t async;
uv_mutex_t async_lock;

void AsyncCallback(uv_async_t* handle) {
  Nan::HandleScope scope;
  TtsWrap* tts = static_cast<TtsWrap*>(handle->data);
  Local<Value> argv[] = {
    Nan::New(tts->event.c_str()).ToLocalChecked(),
    Nan::New(tts->id),
  };
  tts->callback->Call(2, argv);
  uv_mutex_unlock(&async_lock);
}

void onstart(int id, void* data) {
  uv_mutex_lock(&async_lock);
  TtsWrap* tts = static_cast<TtsWrap*>(data);
  tts->id = id;
  tts->event = "start";
  uv_async_send(&async);
}

void oncancel(int id, void* data) {
  uv_mutex_lock(&async_lock);
  TtsWrap* tts = static_cast<TtsWrap*>(async.data);
  tts->id = id;
  tts->event = "cancel";
  uv_async_send(&async);
}

void oncomplete(int id, void* data) {
  uv_mutex_lock(&async_lock);
  TtsWrap* tts = static_cast<TtsWrap*>(async.data);
  tts->id = id;
  tts->event = "complete";
  uv_async_send(&async);
}

void onerror(int id, int err, void* data) {
  uv_mutex_lock(&async_lock);
  TtsWrap* tts = static_cast<TtsWrap*>(async.data);
  tts->id = id;
  tts->err = err;
  tts->event = "error";
  uv_async_send(&async);
}

TtsWrap::TtsWrap() {
  tts_init();
}

TtsWrap::~TtsWrap() {
  tts_destory();
}

NAN_MODULE_INIT(TtsWrap::Init) {
  Local<FunctionTemplate> tmpl = Nan::New<FunctionTemplate>(New);
  tmpl->SetClassName(Nan::New("TtsWrap").ToLocalChecked());
  tmpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tmpl, "say", Say);
  Nan::SetPrototypeMethod(tmpl, "stop", Stop);

  Local<Function> func = Nan::GetFunction(tmpl).ToLocalChecked();
  Nan::Set(target, Nan::New("TtsWrap").ToLocalChecked(), func);
}

NAN_METHOD(TtsWrap::New) {
  TtsWrap* tts = new TtsWrap();
  tts->callback = new Nan::Callback(info[0].As<Function>());
  tts->Wrap(info.This());

  struct tts_callback func = { onstart, oncancel, oncomplete, onerror };
  tts_set(&func);

  uv_mutex_init(&async_lock);
  uv_async_init(uv_default_loop(), &async, AsyncCallback);
  async.data = (void*)tts;

  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(TtsWrap::Say) {
  TtsWrap* tts = Nan::ObjectWrap::Unwrap<TtsWrap>(info.This());
  v8::String::Utf8Value text(info[0].As<String>());
  int id = tts_speak(*text, (void*)tts);
  info.GetReturnValue().Set(Nan::New(id));
}

NAN_METHOD(TtsWrap::Stop) {
  int id = info[0]->NumberValue();
  tts_cancel(id, nullptr);
  info.GetReturnValue().Set(Nan::New(id));
}

void InitModule(Handle<Object> target) {
  TtsWrap::Init(target);
}

NODE_MODULE(tts, InitModule);
