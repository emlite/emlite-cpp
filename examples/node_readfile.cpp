#include <stdio.h>

#define EMLITE_IMPL
#include <emlite/emlite.hpp>

using namespace emlite;

int main() {
    auto require = Val::global("require");
    auto fs      = require(Val("fs"));

    fs.call(
        "readFile",
        Val("LICENSE"),
        Val("utf8"),
        Val([](Handle h) -> Handle {
            size_t len     = 0;
            auto param_vec = Val::vec_from_js_array<Handle>(
                Val::from_handle(h), len
            );
            puts(Val::from_handle(param_vec[1])
                     .as<Uniq<char[]>>()
                     .get());
            return Val::undefined().as_handle();
        })
    );
}