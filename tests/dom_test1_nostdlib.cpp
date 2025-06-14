// clang-format off
// clang++-18 --target=wasm32 -o dom_test1_nostdlib.wasm -Iinclude tests/dom_test1_nostdlib.cpp -nostdlib -Os -Wl,--allow-undefined,--no-entry,--import-memory,--export-memory,--export-all,--strip-all
// clang-format on

#define EMLITE_IMPL
#include <emlite/emlite.hpp>

using namespace emlite;

EMLITE_USED extern "C" int add(int a, int b) {
    Console().log(Val("Hello from Emlite"));

    auto doc = Val::global("document");
    // operator[]
    auto body = doc.call("getElementsByTagName", Val("body"))[0];
    auto btn  = doc.call("createElement", Val("BUTTON"));
    btn.set("textContent", Val("Click Me!"));
    body.call("appendChild", btn);

    // emlite_val_make_callback
    btn.call("addEventListener", Val("click"), Val([](auto) -> Handle {
                 Console().call("log", Val("Clicked"));
                 return Val::undefined().as_handle();
             }));

    // check Val::new_
    auto String = Val::global("String");
    auto str1   = String.new_(Val("created a string object number 1"));
    auto str2   = String.new_(Val("created a string object number 2"));

    // check uniqueness of objects of the same type!
    Console().log(str1);
    Console().log(str2);
    Console().log(str1);

    // check copyStringToWasm
    Console().log(Val(str1.as<UniqCPtr<char>>().get()));
    Console().log(Val(str2.as<UniqCPtr<char>>().get()));
    Console().log(Val(str1.as<UniqCPtr<char>>().get()));

    // operator()
    auto floor = Val::global("Math").get("floor");
    auto ret   = floor(Val(2.5));
    Console().log(ret);

    // test EMLITE_EVAL and also operator()
    // clang-format off
    auto retval = EMLITE_EVAL(
        let a = %d;
        let b = %d;
        console.log(a, b);
        b
    , 5, 6);
    // clang-format on
    Console().log(retval);

    // test await
    auto Notification = Val::global("Notification");
    auto status       = Notification.call("requestPermission").await();
    Console().log(status);

    return a + b;
}

int main() {}