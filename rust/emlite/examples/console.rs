use emlite::{argv, Console};

fn main() {
    let con = Console::get();
    con.log(&argv!["Hello from Emlite!"]);
}
