#define EMLITE_IMPL
#include <emlite/emlite.h>

using namespace emlite;

int main() {
    auto con1 = Console();

    // clang-format off
    auto ret = EMLITE_EVAL(
       let c = ValMap.toValue({});
       c.log("Hello");
       5
    , con1.as_handle());
    // clang-format on

    con1.log(ret);
}