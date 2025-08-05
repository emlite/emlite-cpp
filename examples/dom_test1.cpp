#include <cstddef>
#include <cstdio>
#include <vector>

#include <emlite/emlite.hpp>

using namespace emlite;

int main() {
    emlite::init();
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
        Val::make_fn([=](auto p) -> Val {
            auto [params, len] = p;
            Console().log(params[0]);
            Console().log(doc);
            return Val::undefined();
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
    // emlite_print_object_map();
}