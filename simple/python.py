# // initialize: js->c++->python->python create thread and eventloop->return back
# // loop->enqueue tasks->return js

# // js -> c++ calls into Python with the loop and a C++ callback function and
# // Python uses the loop to create_task and adds a callback to invoke c++
# // callback function when task completes -> returns to C++ then return a JS
# // promise back to JS then js await that promise which gets set by Python code

# // js:
# // js_promise = call_python_task_via_cxx()
# // console.log(await js_promise)

# // c++:
# // js_promise = // create a promise
# // py_callback = ...;
# // pybind11::get_attr(task_creator)(loop_, py_callback)
# // return js_promise

# // py:
# // async def sleep_then_42():
# //    await asyncio.sleep(5)
# //    return 42
# //
# // def task_creator(loop, py_callback):
# //    future = loop.ensure_future(sleep_then_42())
# //    future.add_done_callback(py_callback)

import asyncio
import sys
import threading
import time
import traceback


async def the_function():
    print("The function", file=sys.stderr)
    # await asyncio.sleep(5)
    return 42


async def the_function_test():
    # await asyncio.sleep(10)
    print("The function test", file=sys.stderr)
    return 24


def cb(res):
    print("Callback invoked: ", file=sys.stderr)
    print(res.result(), file=sys.stderr)


def callback_cxx():
    print("callback_cxx", file=sys.stderr)
    # print(future)
    # print(future.result())
    # return 1


class EventLoopThread:

    def __init__(self):
        # self.loop = None  # asyncio.new_event_loop()
        self.loop = None
        # self._thread = threading.Thread(target=self.run_event_loop_)
        self.loop_ready_ = threading.Event()

    # def start(self):
    #     self._thread.start()
    #     self.loop_ready_.wait()

    def run_forever_(self):
        if self.loop is None:
            raise RuntimeError("loop is not initialized")
        asyncio.set_event_loop(self.loop)
        self.loop_ready_.set()
        self.loop.run_forever()

    def start(self):
        print("run_event_loop_ entry", file=sys.stderr)
        # try:
        print("before create new loop", file=sys.stderr)
        self.loop = asyncio.new_event_loop()
        print("after create new loop", file=sys.stderr)
        thread = threading.Thread(target=self.run_forever_)
        thread.start()
        self.loop_ready_.wait()
        print("Python event loop thread started")
        # asyncio.set_event_loop(self.loop)
        # print("after set new loop", file=sys.stderr)
        # self.loop_ready_.set()
        # print("before run_forever", file=sys.stderr)
        # self.loop.run_forever()
        # print("after run_forever", file=sys.stderr)
        # except Exception as e:
        # print("Error!!!!!!", file=sys.stderr)
        # print(e, file=sys.stderr)

    # def stop(self):
    #     self.loop.stop()

    def test(self):
        print("IN TEST", file=sys.stderr)
        if (self.loop is None):
            raise RuntimeError("loop is none")

        print("IN TEST before create_task", file=sys.stderr)
        task = self.loop.create_task(the_function_test())
        print("IN TEST after create_task", file=sys.stderr)
        task.add_done_callback(cb)
        print("IN TEST after add_done_callback", file=sys.stderr)
        time.sleep(5)

    def create_task(self):
        if self.loop is None:
            print("create_task loop none")
            return
            # raise RuntimeError("create_task: Loop is none")
        # call_soon_threadsafe
        print("loop is running: ", file=sys.stderr)
        print(self.loop.is_running(), file=sys.stderr)
        print(self.loop.is_closed(), file=sys.stderr)
        task = self.loop.create_task(the_function())
        print("after create task", file=sys.stderr)
        # task.add_done_callback(cxx_callback)
        task.add_done_callback(callback_cxx)

    def run(self, callable, cxx_callback):
        if self.loop is None:
            print("run loop none")
            return
        f = self.loop.run_until_complete(callable())
        cxx_callback(f)

    def call_soon(self):
        f = self.loop.call_soon_threadsafe(callback_cxx())
        print(f)
        # return f


# event_loop_thread = EventLoopThread()
# event_loop_thread.start()
# event_loop_thread.create_task(the_function, callback_cxx)
