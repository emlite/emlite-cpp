#include <stdio.h>

#define EMLITE_IMPL
#include <emlite/emlite.hpp>

using namespace emlite;

int main() {
    // `require` was exposed to Emlite in tests/node_test.js
    auto require = Val::global("require");

    auto process = require("process");
    Console().log(process.call("cwd"));

    // `fs` was exposed to Emlite in tests/node_test.js
    Val::global("fs").call(
        "readFile",
        "LICENSE",
        "utf8",
        Val::make_fn([](Handle h) -> Handle {
            size_t len     = 0;
            auto param_vec = Val::vec_from_js_array<Val>(
                Val::take_ownership(h), len
            );
            puts(param_vec[1].as<Uniq<char[]>>().get());
            return Val::undefined().as_handle();
        })
    );
}