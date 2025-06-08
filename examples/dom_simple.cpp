#define EMLITE_IMPL
#include <emlite/emlite.h>

using namespace emlite;

int main() {
    auto con1 = Console();
    con1.log(Val("Hello from Emlite"));

    auto doc  = Val::global("document");
    auto body = doc.call("getElementsByTagName", Val("body"))[0];
    auto btn  = doc.call("createElement", Val("BUTTON"));
    btn.set("textContent", Val("Click Me!"));
    body.call("appendChild", btn);
    btn.call("addEventListener", Val("click"), Val([](auto h) -> handle {
                 auto console = Console();
                 console.log(Val::from_handle(h));
                 return Val::undefined().as_handle();
             }));
}