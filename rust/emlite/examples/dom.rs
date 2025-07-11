use emlite::*;

fn main() {
    let document = Val::global("document");
    let elem = document.call("createElement", &argv!["BUTTON"]);
    elem.set("textContent", Val::from("Click"));
    let body = document.call("getElementsByTagName", &argv!["body"]).at(0);
    elem.call(
        "addEventListener",
        &argv![
            "click",
            Val::make_fn(|ev| {
                let console = Console::get();
                console.call("clear", &[]);
                console.log(&[ev[0].get("clientX")]);
                println!("client x: {}", ev[0].get("clientX").as_::<i32>());
                println!("hello from Rust");
                Val::undefined()
            })
        ],
    );
    body.call("appendChild", &argv![elem]);
}
