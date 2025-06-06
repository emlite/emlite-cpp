#include <string.h>

#define EMLITE_IMPL
#include <emlite/emlite.h>

int main() {
    handle global  = emlite_val_global_this();
    handle console = emlite_val_obj_prop(global, "console", strlen("console"));
    VAL_OBJ_CALL(console, "log", emlite_val_make_int(200));
}
