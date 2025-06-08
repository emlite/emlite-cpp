#include <iostream>
#include <vector>

#define EMLITE_IMPL
#include <emlite/emlite.h>

using namespace emlite;

EMLITE_USED extern "C" int add(int a, int b) {
    auto con1 = Console();
    con1.log(Val("Hello from Emlite"));

    auto doc = Val::global("document");
    // operator[]
    auto body = doc.call("getElementsByTagName", Val("body"))[0];
    auto btn  = doc.call("createElement", Val("BUTTON"));
    btn.set("textContent", Val("Click Me!"));
    // test as<> and wasi's fd_write
    printf("%s\n", btn.get("textContent").as<std::string>().c_str());
    fflush(stdout);
    printf("%s\n", btn.type_of().c_str());

    body.call("appendChild", btn);

    // emlite_val_make_callback
    btn.call("addEventListener", Val("click"), Val([](auto) -> handle {
                 auto console = Console();
                 console.call("log", Val("Clicked"));
                 return Val::undefined().as_handle();
             }));

    // check memory growth!
    std::vector<int> vals = {0, 1, 2};
    for (int i = 0; i < 100; i++) {
        vals.push_back(i);
    }

    // check wasi's fd_write shim works
    printf("%d\n", vals.back());

    // check Val::new_
    auto String = Val::global("String");
    auto str1   = String.new_(Val("created a string object number 1"));
    auto str2   = String.new_(Val("created a string object number 2"));

    // check uniqueness of objects of the same type!
    con1.log(str1);
    con1.log(str2);
    con1.log(str1);

    // check copyStringToWasm
    con1.log(Val(str1.as<std::string>()));
    con1.log(Val(str2.as<std::string>()));
    con1.log(Val(str1.as<std::string>()));

    // operator()
    auto floor = Val::global("Math").get("floor");
    auto ret   = floor(Val(2.5));
    con1.log(ret);

    // test EMLITE_EVAL and also operator()
    // clang-format off
    auto retval = EMLITE_EVAL(
        let a = {};
        let b = {};
        console.log(a, b);
        b
    , 5, 6);
    // clang-format on
    con1.log(retval);

    // test await
    auto Notification = Val::global("Notification");
    auto status       = Notification.call("requestPermission").await();
    con1.log(status);

    return a + b;
}

int main() {}