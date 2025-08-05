#include <emlite/emlite.hpp>

using namespace emlite;

class MyJsClass : public Val {
    explicit MyJsClass(Handle h)
        : Val(Val::take_ownership(h)) {}

  public:
    static void define() {
        EMLITE_EVAL({
            class MyJsClass {
                constructor(x, y) {
                    this.x = x;
                    this.y = y;
                }
                print() { console.log(this.x, this.y); }
            } globalThis["MyJsClass"] = MyJsClass;
        });
    }
    static MyJsClass take_ownership(Handle h) {
        return MyJsClass(h);
    }
    MyJsClass(int x, int y)
        : Val(Val::global("MyJsClass").new_(x, y)) {}
    MyJsClass(const Val &val)
        : Val(val) {}
    void print() { call("print"); }
};

int main() {
    emlite::init();
    MyJsClass::define();
    auto c = MyJsClass(5, 6);
    c.call("print");
    auto b = EMLITE_EVAL({
        let b = new MyJsClass(6, 7);
        b.print();
        b
    });
    auto a = b.as<MyJsClass>();
    a.print();
}
