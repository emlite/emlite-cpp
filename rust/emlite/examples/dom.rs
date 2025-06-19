use emlite::*;

fn main() {
    let document = Val::global("document");
    let elem = document.call("createElement", &argv!["BUTTON"]);
    elem.set(&"textContent", Val::from("Click"));
    let body = document.call("getElementsByTagName", &argv!["body"]).at(0);
    elem.call(
        "addEventListener",
        &argv![
            "click",
            Val::make_fn(|ev| {
                let console = Val::global("console");
                console.call("clear", &[]);
                println!("client x: {}", Val::take_ownership(ev).get("clientX").as_i32());
                println!("hello from Rust");
                Val::undefined().as_handle()
            })
        ],
    );
    body.call("appendChild", &argv![elem]);
}
