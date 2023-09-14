const adaptor = require("./adaptor");
async function func() {
  await new Promise(resolve => setTimeout(resolve, 5000));
}

class YourClass {
  async method(message) {
    console.log("method js called");
    await func();
    console.log("Sleep done");

    console.log(message)

    const response = {"message":"Hello from JS!"};
    const jsonString = JSON.stringify(response)
    return jsonString
  }
  async explicit_function(message) {
    console.log("method js called");
    return func().then(() => {
      console.log("Sleep done");

      console.log(message)
  
      const response = {"message":"Hello from JS!"};
      const jsonString = JSON.stringify(response)
      return jsonString
    });
  }
}



let myInstance = new YourClass();
adaptor.put(myInstance);
{
const greeter = new adaptor.GreeterAdaptor();
greeter.print("JS argument string");
}


// func()