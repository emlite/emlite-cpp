#include <emlite/emlite.h>

em_Val console_log(em_Val args) {
    em_Val console = em_Val_global("console");
    return em_Val_call(console, "log", args);
}

Handle btn_click_cb(Handle h, Handle data) {
    console_log(em_Val_from("Clicked"));
    return EMLITE_UNDEFINED;
}

EMLITE_USED int add(int a, int b) {
    console_log(em_Val_from("Hello from Emlite"));

    em_Val doc  = em_Val_global("document");
    em_Val body = em_Val_at(
        em_Val_call(
            doc,
            "getElementsByTagName",
            em_Val_from("body")
        ),
        em_Val_from(0)
    );
    em_Val btn = em_Val_call(
        doc,
        "createElement",
        em_Val_from("BUTTON")
    );
    em_Val_set(
        btn, em_Val_from("textContent"), em_Val_from("Click Me!")
    );

    em_Val_call(body, "appendChild", btn);
    em_Val_call(
        btn,
        "addEventListener",
        em_Val_from("click"),
        em_Val_make_fn(btn_click_cb, 0)
    );

    // check em_Val_new
    em_Val String = em_Val_global("String");
    em_Val str1   = em_Val_new(
        String,
        em_Val_from(
            "created a string object number 1"
        )
    );
    em_Val str2 = em_Val_new(
        String,
        em_Val_from(
            "created a string object number 2"
        )
    );

    console_log(str1);
    console_log(str2);
    console_log(str1);

    console_log(em_Val_from(em_Val_as(char *, str1)));
    console_log(em_Val_from(em_Val_as(char *, str2)));
    console_log(em_Val_from(em_Val_as(char *, str1)));

    em_Val floor =
        em_Val_get(em_Val_global("Math"), em_Val_from("floor"));
    em_Val ret =
        em_Val_invoke(floor, em_Val_from(2.5));
    console_log(ret);

    // clang-format off
    em_Val retval = EMLITE_EVAL(
        {
            let a = %d;
            let b = %d;
            console.log(a, b);
            b
        },
        5,
        6
    );
    // clang-format on
    console_log(retval);

    // test await
    em_Val Notification = em_Val_global("Notification");
    em_Val status       = em_Val_await(
        em_Val_call(Notification, "requestPermission")
    );
    console_log(status);

    emlite_reset_object_map();
    return a + b;
}

int main() {
    emlite_init_handle_table();
}