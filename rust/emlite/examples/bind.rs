use emlite::{env::Handle, *};
use std::ops::{Deref, DerefMut};

struct MyJsClass {
    val: Val,
}

impl MyJsClass {
    fn define() {
        eval!(
            r#"
            class MyJsClass {
                constructor(x, y) {
                    this.x = x;
                    this.y = y;
                }
                print() { console.log(this.x, this.y); }
            } globalThis["MyJsClass"] = MyJsClass;
        "#
        );
    }
    fn new(x: i32, y: i32) -> Self {
        Self {
            val: Val::global("MyJsClass").new(&argv![x, y]),
        }
    }
    fn print(&self) {
        self.val.call("print", &[]);
    }
}

impl FromVal for MyJsClass {
    fn from_val(v: &Val) -> Self {
        MyJsClass { val: v.clone() }
    }
    fn take_ownership(v: Handle) -> Self {
        Self::from_val(&Val::take_ownership(v))
    }
    fn as_handle(&self) -> Handle {
        self.val.as_handle()
    }
}

impl Deref for MyJsClass {
    type Target = Val;

    fn deref(&self) -> &Self::Target {
        &self.val
    }
}

impl DerefMut for MyJsClass {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.val
    }
}

impl From<MyJsClass> for Val {
    fn from(s: MyJsClass) -> Val {
        let handle = s.as_handle();
        std::mem::forget(s);
        Val::take_ownership(handle)
    }
}

fn main() {
    MyJsClass::define();
    let c = MyJsClass::new(5, 6);
    c.call("print", &[]);
    let b = eval!(
        r#"
        let b = new MyJsClass(6, 7);
        b.print();
        b
    "#
    );
    let a = b.as_::<MyJsClass>();
    a.print();
    let console = Console::get();
    console.log(&[a.into()]);
}
