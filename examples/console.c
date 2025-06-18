#define EMLITE_IMPL
#include <emlite/emlite.h>

EMLITE_USED int main() {
    em_Val console = em_Val_global("console");
    em_Val_call(
        console, "log", 1, em_Val_from_string("200")
    );
}
