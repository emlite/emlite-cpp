#define EMLITE_IMPL
#include <emlite/emlite.h>

using namespace emlite;

int main() {
    auto require = Val::global("require");
    auto fs = require(Val("fs"));
    
    fs.call("readFile", Val("index.html"), Val("utf8"), Val([](handle h) -> handle {
        puts(Val::from_handle(Val::from_handle(h)[1].as<int>()).as<std::string>().c_str());
        return Val::undefined().as_handle();
    }));
}