#include <emlite/emlite.h>

EMLITE_USED int main() {
    emlite_init_handle_table();
    em_Val console = em_Val_global("console");
    em_Val_call(
        console, "log", em_Val_from("200")
    );
    emlite_reset_object_map();
}
