// clang-format off
// example build command:
// clang-tidy tests/dom_test1.cpp -- -Iinclude -std=c++20 --target=wasm32-wasi --sysroot=/home/ray/dev/wasi-sysroot-25.0/
// clang-format on

#include <cstddef>
#include <cstdio>
#include <vector>

#define EMLITE_IMPL
#include <emlite/emlite.hpp>

using namespace emlite;

EMLITE_USED extern "C" int add(int a, int b) {
    Console().log("Hello from Emlite");

    auto doc = Val::global("document");
    // operator[]
    auto body = doc.call("getElementsByTagName", "body")[0];
    auto btn  = doc.call("createElement", "BUTTON");
    btn.set("textContent", "Click Me!");
    // test as<> and wasi's fd_write
    puts(btn.get("textContent").as<Uniq<char[]>>().get());

    puts(btn.type_of().get());

    body.call("appendChild", btn);
    // emlite_val_make_callback
    btn.call(
        "addEventListener",
        "click",
        Val::make_fn([](auto) -> Handle {
            Console().call("log", "Clicked");
            return Val::undefined().as_handle();
        })
    );

    // check memory growth!
    std::vector<int> vals = {0, 1, 2};
    for (int i = 0; i < 100; i++) {
        vals.push_back(i);
    }

    // check wasi's fd_write shim works
    printf("%d\n", vals.back());

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

    auto arr =
        Val::global("eval")("let arr = [1, 2, 3, 4, 5]; arr"
        );

    size_t len = 0;
    auto arr2  = Val::vec_from_js_array<int>(arr, len);

    printf("%ld\n", len);
    for (size_t i = 0; i < len; i++) {
        printf("%d\n", arr2[i]);
    }

    return a + b;
}

int main() {
    int a = add(1, 2);
    emlite_print_object_map();
}