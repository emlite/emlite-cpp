#define EMLITE_IMPL
#include <emlite/emlite.hpp>

using namespace emlite;

int main() {
    Console().log(Val("Hello from Emlite"));

    auto doc = Val::global("document");
    auto body =
        doc.call("getElementsByTagName", Val("body"))[0];
    auto btn = doc.call("createElement", Val("BUTTON"));
    btn.set("textContent", Val("Click Me!"));
    body.call("appendChild", btn);
    btn.call(
        "addEventListener",
        Val("click"),
        Val::make_fn([](auto h) -> Handle {
            size_t len     = 0;
            auto param_vec = Val::vec_from_js_array<Handle>(
                Val::take_ownership(h), len
            );
            Console().log(Val::take_ownership(param_vec[0]));
            return Val::undefined().as_handle();
        })
    );
}