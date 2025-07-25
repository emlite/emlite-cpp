#include <stdio.h>

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
        Val::make_fn([](auto p) -> Val {
            auto [params, len] = p;
            puts(params[1].template as<Uniq<char[]>>().get());
            return Val::undefined();
        })
    );
}