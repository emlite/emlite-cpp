#define EMLITE_IMPL
#include <emlite/emlite.hpp>

using namespace emlite;

int main() {
    Console().log(Val("Hello from Emlite"));

    auto doc  = Val::global("document");
    auto body = doc.call("getElementsByTagName", Val("body"))[0];
    auto btn  = doc.call("createElement", Val("BUTTON"));
    btn.set("textContent", Val("Click Me!"));
    body.call("appendChild", btn);
    btn.call("addEventListener", Val("click"), Val([](auto h) -> Handle {
                 Console().log(Val::from_handle(h));
                 return Val::undefined().as_handle();
             }));
}