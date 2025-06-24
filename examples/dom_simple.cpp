#define EMLITE_IMPL
#include <emlite/emlite.hpp>

using namespace emlite;

int main() {
    Console().log("Hello from Emlite");

    auto doc  = Val::global("document");
    auto body = doc.call("getElementsByTagName", "body")[0];
    auto btn  = doc.call("createElement", "BUTTON");
    btn.set("textContent", "Click Me!");
    btn.call(
        "addEventListener",
        "click",
        Val::make_fn([](auto h) -> Handle {
            size_t len     = 0;
            auto param_vec = Val::vec_from_js_array<Val>(
                Val::take_ownership(h), len
            );
            Console().log(param_vec[0]);
            return Val::undefined().as_handle();
        })
    );
    body.call("appendChild", btn);
}