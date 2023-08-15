#include "node/node.h"
#include <iostream>
#include <pybind11/embed.h>

namespace demo {
namespace py = pybind11;

using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Value;

void Method(const FunctionCallbackInfo<Value> &args) {
  std::cout << "'Method invoked'" << std::endl;
  Isolate *isolate = args.GetIsolate();
  py::scoped_interpreter guard{};
  py::print("Python print"); // use the Python API
  py::module math = py::module::import("math");
  py::object result = math.attr("sqrt")(25);
  std::string result_str("C++ RESULT: Sqrt of 25 is: RESULT FROM PYTHON: " +
                         std::to_string(result.cast<float>()));
  args.GetReturnValue().Set(
      String::NewFromUtf8(isolate, result_str.c_str()).ToLocalChecked());
}

void Initialize(Local<Object> exports) {
  NODE_SET_METHOD(exports, "xx", Method);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)

} // namespace demo