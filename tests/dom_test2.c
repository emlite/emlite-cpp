#include <stdio.h>

#define EMLITE_IMPL
#include <emlite/emlite.h>

em_Val console_log(const char *s) {
    em_Val str     = em_Val_from_string(s);
    em_Val console = em_Val_global("console");
    em_Val ret     = em_Val_call(console, "log", 1, str);
    em_Val_delete(console);
    em_Val_delete(str);
    return ret;
}

Handle btn_click_cb(Handle h) {
    console_log("Clicked");
    return emlite_val_undefined();
}

EMLITE_USED int add(int a, int b) {
    console_log("Hello from Emlite");

    em_Val doc       = em_Val_global("document");
    em_Val body_elem = em_Val_from_string("body");
    em_Val bodies    = em_Val_call(
        doc, "getElementsByTagName", 1, body_elem
    );
    em_Val body = em_Val_at(bodies, 0);
    em_Val elem = em_Val_from_string("BUTTON");
    em_Val btn = em_Val_call(doc, "createElement", 1, elem);
    em_Val btn_label = em_Val_from_string("Click Me!");
    em_Val_set(btn, "textContent", btn_label);

    em_Val textContent = em_Val_get(btn, "textContent");
    printf("%s\n", em_Val_as_string(textContent));
    (void)fflush(stdout);
    printf("%s\n", em_Val_typeof(btn));

    em_Val appended = em_Val_call(body, "appendChild", 1, btn);

    em_Val event  = em_Val_from_string("click");
    em_Val btn_cb = em_Val_make_fn(btn_click_cb);
    em_Val_call(btn, "addEventListener", 2, event, btn_cb);

    // check em_Val_new
    em_Val String       = em_Val_global("String");
    em_Val str1_content = em_Val_from_string(
        "created a string object number 1"
    );
    em_Val str2_content = em_Val_from_string(
        "created a string object number 2"
    );
    em_Val str1 = em_Val_new(String, 1, str1_content);
    em_Val str2 = em_Val_new(String, 1, str2_content);

    console_log(em_Val_as_string(str1));
    console_log(em_Val_as_string(str2));
    console_log(em_Val_as_string(str1));

    em_Val Math       = em_Val_global("Math");
    em_Val floor      = em_Val_get(Math, "floor");
    em_Val double_inp = em_Val_from_double(2.5);
    em_Val ret        = em_Val_invoke(floor, 1, double_inp);
    console_log(em_Val_as_string(ret));

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
    console_log(em_Val_as_string(retval));

    // test await
    em_Val Notification = em_Val_global("Notification");
    em_Val call =
        em_Val_call(Notification, "requestPermission", 0);
    em_Val status = em_Val_await(call);
    console_log(em_Val_as_string(status));

    // emlite_reset_object_map();
    em_Val_delete(status);
    em_Val_delete(call);
    em_Val_delete(Notification);
    em_Val_delete(retval);
    em_Val_delete(ret);
    em_Val_delete(double_inp);
    em_Val_delete(floor);
    em_Val_delete(Math);
    em_Val_delete(str2);
    em_Val_delete(str2_content);
    em_Val_delete(str1);
    em_Val_delete(str1_content);
    em_Val_delete(String);
    em_Val_delete(btn_cb);
    em_Val_delete(event);
    em_Val_delete(appended);
    em_Val_delete(textContent);
    em_Val_delete(btn_label);
    em_Val_delete(btn);
    em_Val_delete(elem);
    em_Val_delete(body);
    em_Val_delete(bodies);
    em_Val_delete(body_elem);
    em_Val_delete(doc);

    return a + b;
}

int main() {
    int a = add(1, 2);
    emlite_print_object_map();
}