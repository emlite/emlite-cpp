#include <emlite/emlite.hpp>

using namespace emlite;

int main() {
    // clang-format off
    auto ret = EMLITE_EVAL({
       let c = EMLITE_VALMAP.toValue(%d);
       c.log('Hello');
       5
    }, Console().as_handle());

    EMLITE_EVAL({
        console.log("Works");
    });
    // clang-format on

    Console().log(ret);
}