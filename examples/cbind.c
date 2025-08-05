#include <emlite/emlite.h>

typedef struct {
    em_Val inner;
} MyJsClass;

static void MyJsClass_define() {
    EMLITE_EVAL({
        class MyJsClass {
            constructor(x, y) {
                this.x = x;
                this.y = y;
            }
            print() { console.log(this.x, this.y); }
        } globalThis['MyJsClass'] = MyJsClass;
    });
}

MyJsClass MyJsClass_from_handle(Handle h) {
    return (MyJsClass){.inner = em_Val_from_handle(h)};
}
MyJsClass MyJsClass_new(int x, int y) {
    return (MyJsClass
    ){.inner = em_Val_new(em_Val_global("MyJsClass"), em_Val_from(x), em_Val_from(y))
    };
}

MyJsClass MyJsClass_from_val(em_Val val) {
    return (MyJsClass){.inner = val};
}

void MyJsClass_print(MyJsClass *self) {
    em_Val_call(self->inner, "print");
}

int main() {
    emlite_init_handle_table();
    MyJsClass_define();
    MyJsClass c = MyJsClass_new(5, 6);
    MyJsClass_print(&c);
    em_Val b    = EMLITE_EVAL({
        let b = new MyJsClass(6, 7);
        b.print();
        b
    });
    MyJsClass a = MyJsClass_from_val(b);
    MyJsClass_print(&a);
}
