#include <Python.h>
#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#include <thread>

namespace py = pybind11;

struct Middleware {
  Middleware() {
    // py::initialize_interpreter();
    // py::scoped_interpreter guard{};
    // std::cout << "State: " << PyGILState_Check() << "\n";
    // py::gil_scoped_acquire acq;
    // std::cout << "State: " << PyGILState_Check() << "\n";
    // std::cout << "Middleware ctor\n";
  }

  ~Middleware() {
    // py::finalize_interpreter();
    std::cout << "~Middleware()\n";
  }

  void initialize() {
    ready.exchange(false);
    std::cout << "State: " << PyGILState_Check() << "\n";
    event_loop_runner = std::thread([&]() {
      {
        std::cout << "THREAD State: " << PyGILState_Check() << "\n";
        {
          py::gil_scoped_acquire acquire;
          std::cout << "THREAD State: " << PyGILState_Check() << "\n";
          std::cout << "there\n";

          std::cout << "THREAD State: " << PyGILState_Check() << "\n";
          std::cout << "blocked\n";
          py::object module = py::module::import("simple.python");
          event_loop_thread = module.attr("EventLoopThread")();
          event_loop_thread.attr("start")();
        }
        ready.exchange(true);
        std::cout << "THREAD State after: " << PyGILState_Check() << "\n";
        std::unique_lock<std::mutex> lock(python_thread_lock_);
        cv.wait(lock, [&]() { return !is_running; });
      }
    });
    is_running = true;
    // event_loop_runner.detach();
  }

  // void CallFromJsAndGetPromise() {
  //   py::gil_scoped_acquire acquire;
  //   std::cout << "After acquire\n";
  //   std::cout << "STATE: " << PyGILState_Check() << "\n";

  //   Isolate* isolate = Isolate::GetCurrent();
  //   auto resolver = v8::Local<v8::Promise::Resolver>::New(isolate,
  //   persistent);

  //   py::cpp_function fn(
  //       [&](py::object future) {
  //         std::cout << "C++: Callback\n";
  //         resolver->Resolve(isolate->GetCurrentContext(),
  //         v8::Int32::New(isolate, future.cast<int>())).FromJust();
  //         persistent.Reset();
  //       },
  //       py::arg("future"));
  //   std::cout << "cpp_fun created\n";
  //   py::object callable =
  //   py::module::import("simple.python").attr("the_function");
  //   // {
  //   // std::cout << "Before create_task call\n";
  //   // std::cout << "STATE: " << PyGILState_Check() << "\n";
  //   event_loop_thread.attr("create_task")(callable, fn);
  //   // std::this_thread::sleep_for(std::chrono::seconds(3));
  //   // }
  // }
  // resolver->Resolve(isolate->GetCurrentContext(), v8::Int32::New(isolate,
  // f.get()))
  //     .FromJust();
  // persistent.Reset();
  // } catch (std::runtime_error e) {
  //   std::cout << "C++ EXCEPTION: \n";
  //   std::cout << e.what() << "\n\n";
  // }

  void test() {
    py::gil_scoped_acquire aquire;
    // std::unique_lock<std::mutex> lock(lock_);
    std::cout << "COUNT: " << event_loop_thread.ref_count();
    event_loop_thread.attr("call_soon")();
  }

  // pybind11::scoped_interpreter guard;
  py::object event_loop_thread;
  std::condition_variable cv;
  std::mutex python_thread_lock_;
  bool is_running = false;
  std::thread event_loop_runner;
  std::atomic<bool> ready;
};

int main() {
  py::initialize_interpreter();
  py::gil_scoped_release release;

  Middleware mw;
  mw.initialize();

  while (!mw.ready.load()) {
  }

  mw.test();
  // block main thead;
  std::promise<int> p;
  auto f = p.get_future();
  f.get();
  return 0;
}