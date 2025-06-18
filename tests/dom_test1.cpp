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
    printf(
        "%s\n",
        btn.get("textContent").as<Uniq<char[]>>().get()
    );
    fflush(stdout);
    printf("%s\n", btn.type_of().get());

    body.call("appendChild", btn);

    // emlite_val_make_callback
    btn.call(
        "addEventListener",
        Val("click"),
        Val([](auto) -> Handle {
            Console().call("log", Val("Clicked"));
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

    printf("%ld\n", len);
    for (size_t i = 0; i < len; i++) {
        printf("%d\n", arr2[i]);
    }

    return a + b;
}

int main() {}