#include <stdio.h>

#define EMLITE_IMPL
#include <emlite/emlite.h>

em_Val console_log(em_Val args) {
    em_Val console = em_Val_global("console");
    return em_Val_call(console, "log", 1, args);
}

Handle btn_click_cb(Handle h) {
    console_log(em_Val_from_string("Clicked"));
    return emlite_val_undefined();
}

EMLITE_USED int add(int a, int b) {
    console_log(em_Val_from_string("Hello from Emlite"));

    em_Val doc  = em_Val_global("document");
    em_Val body = em_Val_at(
        em_Val_call(
            doc,
            "getElementsByTagName",
            1,
            em_Val_from_string("body")
        ),
        0
    );
    em_Val btn = em_Val_call(
        doc,
        "createElement",
        1,
        em_Val_from_string("BUTTON")
    );
    em_Val_set(
        btn, "textContent", em_Val_from_string("Click Me!")
    );

    printf(
        "%s\n",
        em_Val_as_string(em_Val_get(btn, "textContent"))
    );
    (void)fflush(stdout);
    printf("%s\n", em_Val_typeof(btn));

    em_Val_call(body, "appendChild", 1, btn);

    em_Val_call(
        btn,
        "addEventListener",
        2,
        em_Val_from_string("click"),
        em_Val_make_js_function(btn_click_cb)
    );

    // check em_Val_new
    em_Val String = em_Val_global("String");
    em_Val str1   = em_Val_new(
        String,
        1,
        em_Val_from_string(
            "created a string object number 1"
        )
    );
    em_Val str2 = em_Val_new(
        String,
        1,
        em_Val_from_string(
            "created a string object number 2"
        )
    );

    console_log(str1);
    console_log(str2);
    console_log(str1);

    console_log(em_Val_from_string(em_Val_as_string(str1)));
    console_log(em_Val_from_string(em_Val_as_string(str2)));
    console_log(em_Val_from_string(em_Val_as_string(str1)));

    em_Val floor =
        em_Val_get(em_Val_global("Math"), "floor");
    em_Val ret =
        em_Val_invoke(floor, 1, em_Val_from_double(2.5));
    console_log(ret);

    // // clang-format off
    em_Val retval = EMLITE_EVAL(let a = % d; let b = % d;
                                console.log(a, b);
                                b, 5, 6);
    // clang-format on
    console_log(retval);

    // // test await
    em_Val Notification = em_Val_global("Notification");
    em_Val status       = em_Val_await(
        em_Val_call(Notification, "requestPermission", 0)
    );
    console_log(status);

    return a + b;
}

int main() {}