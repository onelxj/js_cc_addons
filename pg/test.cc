// #include <node/node.h>
// #include <node/v8.h>

#include "pg/greeter.pb.h"
#include "json/json.h"
#include <condition_variable>
#include <iostream>
#include <map>
#include <node.h>
#include <thread>
#include <v8-array-buffer.h>
#include <v8.h>

using namespace v8;

void put(const v8::FunctionCallbackInfo<v8::Value> &args);

class GreeterAdaptor {
public:
  GreeterAdaptor() = default;
  ~GreeterAdaptor() { std::cout << "~GreeterAdaptor()\n"; }

  void Register(const v8::FunctionCallbackInfo<v8::Value> &args) {}

  void print(const std::string &message) { std::cout << message << "\n"; }

private:
  std::map<std::string, Local<Object>> objects;
};

class GreeterAdaptorWrapper {
public:
  static std::string response;
  static void New(const v8::FunctionCallbackInfo<v8::Value> &args) {
    if (args.IsConstructCall()) {
      GreeterAdaptor *adaptor = new GreeterAdaptor();
      v8::Isolate *isolate = args.GetIsolate();
      v8::Local<v8::Object> obj = args.This();
      v8::Persistent<v8::Object, v8::CopyablePersistentTraits<v8::Object>>
          persistent(isolate, obj);
      persistent.SetWeak(adaptor, DestructorCallback,
                         v8::WeakCallbackType::kParameter);
      args.GetReturnValue().Set(obj);
    }
  }

  static void
  DestructorCallback(const v8::WeakCallbackInfo<GreeterAdaptor> &data) {
    GreeterAdaptor *adaptor = data.GetParameter();
    delete adaptor;
  }

  static void PrintMethod(const v8::FunctionCallbackInfo<v8::Value> &args) {
    v8::Isolate *isolate = args.GetIsolate();

    if (args.Length() < 1) {
      isolate->ThrowException(
          v8::Exception::TypeError(v8::String::NewFromUtf8Literal(
              isolate, "Wrong number of arguments")));
      return;
    }

    v8::Local<v8::External> field =
        v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0));
    GreeterAdaptor *adaptor = static_cast<GreeterAdaptor *>(field->Value());

    v8::String::Utf8Value str(isolate, args[0]);
    std::string cppStr(*str);

    adaptor->print(cppStr);
  }

  static void Initialize(v8::Local<v8::Object> exports) {
    v8::Isolate *isolate = exports->GetIsolate();

    // Declare the Printer class in V8
    v8::Local<v8::FunctionTemplate> adaptorTpl =
        v8::FunctionTemplate::New(isolate, New);
    adaptorTpl->SetClassName(
        v8::String::NewFromUtf8Literal(isolate, "GreeterAdaptor"));
    adaptorTpl->InstanceTemplate()->SetInternalFieldCount(1);

    // Link the print method
    NODE_SET_PROTOTYPE_METHOD(adaptorTpl, "print", PrintMethod);

    v8::Local<v8::Function> constructor =
        adaptorTpl->GetFunction(isolate->GetCurrentContext()).ToLocalChecked();
    exports
        ->Set(isolate->GetCurrentContext(),
              v8::String::NewFromUtf8Literal(isolate, "GreeterAdaptor"),
              constructor)
        .FromJust();
    NODE_SET_METHOD(exports, "put", put);
  }
};

NODE_MODULE(NODE_GYP_MODULE_NAME, GreeterAdaptorWrapper::Initialize)

std::string response;
std::mutex mtx;
std::condition_variable cv;
bool ready = false;

void Callback(const v8::FunctionCallbackInfo<v8::Value> &args) {
  Local<Value> result = args[0];
  v8::String::Utf8Value utf8Value(args.GetIsolate(), result);
  std::string response = *utf8Value;
  std::cout << response << std::endl;

  Json::Reader reader;
  Json::Value root;
  reader.parse(response, root);

  std::cout << root;

  pg::SendMessageResponse response_proto;
  response_proto.set_message(root["message"].asString());
  // std::cout << "CC: Proto:\n";
  // std::cout << response_proto.message() << "\n";
  response = response_proto.message();

  {
    std::lock_guard<std::mutex> lock(mtx);
    ready = true;
    cv.notify_one();
  }
}

void put(const v8::FunctionCallbackInfo<v8::Value> &args) {
  v8::Isolate *isolate = args.GetIsolate();

  // Ensure there's at least one argument passed and it's an object
  if (args.Length() < 1 || !args[0]->IsObject()) {
    isolate->ThrowException(v8::String::NewFromUtf8Literal(
        isolate, "Expected one object argument"));
    return;
  }

  v8::Local<v8::Object> jsObj =
      args[0]->ToObject(isolate->GetCurrentContext()).ToLocalChecked();
  Local<Value> proto = jsObj->GetPrototype();
  Local<Value> constructor =
      proto.As<Object>()
          ->Get(isolate->GetCurrentContext(),
                String::NewFromUtf8(isolate, "constructor").ToLocalChecked())
          .ToLocalChecked();
  Local<Value> constructorName = constructor.As<Function>()->GetName();
  String::Utf8Value utf8Name(isolate, constructorName);
  std::string className(*utf8Name);

  std::cout << "Class Name: " << className << std::endl;

  //   pg::SendMessageRequest request;
  //   request.set_data("Hello from C++!");
  std::string serializedMessage;
  Json::Value v;
  v["data"] = "Hello from C++!";
  Json::FastWriter fastWriter;
  serializedMessage = fastWriter.write(v);
  Local<String> v8String =
      String::NewFromUtf8(isolate, serializedMessage.c_str()).ToLocalChecked();

  // Convert the serialized proto to a Node.js Buffer
  Local<ArrayBuffer> arrayBuffer =
      ArrayBuffer::New(isolate, serializedMessage.size());
  memcpy(arrayBuffer->Data(), serializedMessage.data(),
         serializedMessage.size());

  // Assuming the first argument is the JS object instance
  v8::Local<v8::Value> methodMethod =
      jsObj
          ->Get(isolate->GetCurrentContext(),
                v8::String::NewFromUtf8Literal(isolate, "method"))
          .ToLocalChecked();
  v8::Local<v8::Function> methodFunction =
      v8::Local<v8::Function>::Cast(methodMethod);

  // // Call the method on the object instance
  const int argc = 1;
  Local<Value> argv[argc] = {v8String};
  v8::Local<v8::Value> jsresponse =
      methodFunction->Call(isolate->GetCurrentContext(), jsObj, argc, argv)
          .ToLocalChecked();
  std::cout << "Method in progress\n";
  Local<Promise> promise = jsresponse.As<Promise>();
  Local<FunctionTemplate> callback = FunctionTemplate::New(isolate, Callback);
  Local<Function> func =
      callback->GetFunction(isolate->GetCurrentContext()).ToLocalChecked();
  Local<Value> thenArgs[] = {func};
  Local<Function> thenFunction = Local<Function>::Cast(
      promise
          ->Get(isolate->GetCurrentContext(),
                String::NewFromUtf8(isolate, "then").ToLocalChecked())
          .ToLocalChecked());
  thenFunction->Call(isolate->GetCurrentContext(), promise, 1, thenArgs)
      .ToLocalChecked();

  std::unique_lock<std::mutex> lock(mtx);
  while (!ready) {
    std::cout << "Empty\n";
    cv.wait(lock);
  }

  //   v8::String::Utf8Value utf8Value1(isolate, jsresponse);
  //   std::string response = *utf8Value1;
  //   std::cout << response << std::endl;

  //   Json::Reader reader;
  //   Json::Value root;
  //   reader.parse(response, root);

  //   std::cout << root;

  //   pg::SendMessageResponse response_proto;
  //   response_proto.set_message(root["message"].asString());
  //   std::cout << "CC: Proto:\n";
  //   std::cout << response_proto.message() << "\n";
}

// void Initialize(v8::Local<v8::Object> exports) {

// }

// NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)
