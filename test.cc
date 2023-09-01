#include <fstream>
#include <iostream>
#include <libplatform/libplatform.h>
#include <sstream>
#include <v8.h>

std::string ReadFile(const std::string &filepath) {
  std::ifstream t(filepath);
  std::stringstream buffer;
  buffer << t.rdbuf();
  return buffer.str();
}

int main(int argc, char *argv[]) {
  // Initialize V8
  v8::V8::InitializeICUDefaultLocation(argv[0]);
  v8::V8::InitializeExternalStartupData(argv[0]);
  std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();

  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator =
      v8::ArrayBuffer::Allocator::NewDefaultAllocator();

  v8::Isolate *isolate = v8::Isolate::New(create_params);
  {
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
    v8::Local<v8::Context> context = v8::Context::New(isolate, nullptr, global);
    v8::Context::Scope context_scope(context);

    // Read, compile, and run the external JavaScript code
    std::string jsCode = ReadFile("testjs.js");
    v8::Local<v8::String> source =
        v8::String::NewFromUtf8(isolate, jsCode.c_str()).ToLocalChecked();
    v8::Local<v8::Script> script =
        v8::Script::Compile(context, source).ToLocalChecked();
    auto r = script->Run(context);

    // Invoke a specific function from the script (if required)
    v8::Local<v8::Function> greet_fn =
        context->Global()
            ->Get(context,
                  v8::String::NewFromUtf8(isolate, "greet").ToLocalChecked())
            .ToLocalChecked()
            .As<v8::Function>();
    v8::Local<v8::Value> args[1] = {
        v8::String::NewFromUtf8(isolate, "World").ToLocalChecked()};
    v8::Local<v8::Value> result =
        greet_fn->Call(context, context->Global(), 1, args).ToLocalChecked();

    v8::String::Utf8Value utf8(isolate, result);
    std::cout << *utf8 << std::endl;
  }

  // Cleanup
  isolate->Dispose();
  v8::V8::Dispose();
  v8::V8::ShutdownPlatform();
  delete create_params.array_buffer_allocator;

  return 0;
}