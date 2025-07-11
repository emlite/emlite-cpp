use emlite::{Console, argv};

fn main() {
    let con = Console::get();
    con.log(&argv!["Hello from Emlite!"]);
}
