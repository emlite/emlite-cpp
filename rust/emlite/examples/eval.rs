use emlite::{Console, eval};

fn main() {
    let con = Console::get();
    let ret = eval!(
        r#"
        let con = ValMap.toValue({});
        con.log("Hello");
        6
    "#,
        con.as_handle()
    );
    con.log(&[ret]);
}
