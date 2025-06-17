#include <stdio.h>

#define EMLITE_IMPL
#include <emlite/emlite.hpp>

using namespace emlite;

int main() {
    auto require = Val::global("require");
    auto fs = require(Val("fs"));
    
    fs.call("readFile", Val("LICENSE"), Val("utf8"), Val([](Handle h) -> Handle {
        puts(Val::from_handle(Val::from_handle(h)[1].as<int>()).as<UniqCPtr<char[]>>().get());
        return Val::undefined().as_handle();
    }));
}