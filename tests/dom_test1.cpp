// clang-format off
// example build command:
// clang-tidy tests/dom_test1.cpp -- -Iinclude -std=c++20 --target=wasm32-wasi --sysroot=/home/ray/dev/wasi-sysroot-25.0/
// clang-format on

#include <cstddef>
#include <iostream>
#include <vector>

#define EMLITE_IMPL
#include <emlite/emlite.hpp>

using namespace emlite;

EMLITE_USED extern "C" int add(int a, int b) {
    Console().log(Val("Hello from Emlite"));

    auto doc = Val::global("document");
    // operator[]
    auto body =
        doc.call("getElementsByTagName", Val("body"))[0];
    auto btn = doc.call("createElement", Val("BUTTON"));
    btn.set("textContent", Val("Click Me!"));
    // test as<> and wasi's fd_write
    std::cout
        << btn.get("textContent").as<Uniq<char[]>>().get()
        << std::endl;

    std::cout << btn.type_of().get() << std::endl;
    
    // emlite_val_make_callback
    btn.call(
        "addEventListener",
        Val("click"),
        Val::make_fn([](auto) -> Handle {
            Console().call("log", Val("Clicked"));
            return Val::undefined().as_handle();
        })
    );
    
    body.call("appendChild", btn);

    // check memory growth!
    std::vector<int> vals = {0, 1, 2};
    for (int i = 0; i < 100; i++) {
        vals.push_back(i);
    }

    // check wasi's fd_write shim works
    std::cout << vals.back() << std::endl;

    // check Val::new_
    auto String = Val::global("String");
    auto str1 =
        String.new_(Val("created a string object number 1")
        );
    auto str2 =
        String.new_(Val("created a string object number 2")
        );

    // check uniqueness of objects of the same type!
    Console().log(str1);
    Console().log(str2);
    Console().log(str1);

    // check copyStringToWasm
    Console().log(Val(str1.as<Uniq<char[]>>().get()));
    Console().log(Val(str2.as<Uniq<char[]>>().get()));
    Console().log(Val(str1.as<Uniq<char[]>>().get()));

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
    auto status =
        Notification.call("requestPermission").await();
    Console().log(status);

    auto arr = Val::global("eval")(
        Val("let arr = [1, 2, 3, 4, 5]; arr")
    );

    size_t len = 0;
    auto arr2  = Val::vec_from_js_array<int>(arr, len);

    std::cout << len << std::endl;
    for (size_t i = 0; i < len; i++) {
        std::cout << arr2[i] << std::endl;
    }

    return a + b;
}

int main() {
    int a = add(1, 2);
    emlite_print_object_map();
}