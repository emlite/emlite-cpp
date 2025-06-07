use emlite::{Console, Val};

fn main() {
    let con = Console::get();
    con.log(&[&Val::from("Hello from Emlite!")]);
}