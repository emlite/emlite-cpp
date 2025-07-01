// clang-format off
// example build command:
// clang++ --target=wasm32 -o dom_test1_nostdlib.wasm -Iinclude tests/dom_test1_nostdlib.cpp -nostdlib -Os -Wl,--allow-undefined,--no-entry,--import-memory,--export-memory,--export=main,--export=malloc,--export-if-defined=add,--export-table,,--strip-all
// clang-format on

#define EMLITE_IMPL
#include <emlite/emlite.hpp>

using namespace emlite;

EMLITE_USED extern "C" int add(int a, int b) {
    Console().log("Hello from Emlite");

    auto arr = Uniq<int[]>(new int[200]);
    for (int i = 0; i < 200; i++)
        arr[i] = i;

    auto doc = Val::global("document");
    // operator[]
    auto body = doc.call("getElementsByTagName", "body")[0];
    auto btn  = doc.call("createElement", "BUTTON");
    btn.set("textContent", "Click Me!");

    // emlite_val_make_callback
    btn.call(
        "addEventListener",
        "click",
        Val::make_fn([](auto) -> Handle {
            Console().call("log", "Clicked");
            return Val::undefined().as_handle();
        })
    );

    body.call("appendChild", btn);
    // check Val::new_
    auto String = Val::global("String");
    auto str1 =
        String.new_("created a string object number 1");
    auto str2 =
        String.new_("created a string object number 2");

    // check uniqueness of objects of the same type!
    Console().log(str1);
    Console().log(str2);
    Console().log(str1);

    // check copyStringToWasm
    Console().log(str1.as<Uniq<char[]>>().get());
    Console().log(str2.as<Uniq<char[]>>().get());
    Console().log(str1.as<Uniq<char[]>>().get());

    // operator()
    auto floor = Val::global("Math").get("floor");
    auto ret   = floor(2.5);
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
    auto status =
        Notification.call("requestPermission").await();
    Console().log(status);
    auto twenty = arr[20];
    return a + twenty + b;
}

int main() {}