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
        Val::make_fn([](auto p) -> Val {
            auto [params, len] = p;
            Console().log(params[0]);
            return Val::undefined();
        })
    );
    body.call("appendChild", btn);
}