use emlite::*;

fn main() {
    #[allow(non_snake_case)]
    let mut AudioContext = Val::global("AudioContext");
    if !AudioContext.as_bool() {
        println!("No global AudioContext, trying webkitAudioContext");
        AudioContext = Val::global("webkitAudioContext");
    }

    println!("Got an AudioContext");
    let context = AudioContext.new(&[]);
    let oscillator = context.call("createOscillator", &[]);

    println!("Configuring oscillator");
    oscillator.set("type", "triangle");
    oscillator.get("frequency").set("value", Val::from(261.63)); // Middle C

    Val::global_this().set("oscillator", oscillator);
    Val::global_this().set("context", context);

    let document = Val::global("document");
    let elem = document.call("createElement", &argv!["BUTTON"]);
    elem.set("textContent", "Click");
    let body = document.call("getElementsByTagName", &argv!["body"]).at(0);
    elem.call(
        "addEventListener",
        &argv![
            "click",
            Val::make_fn(|_| {
                let oscillator = Val::global("oscillator");
                let context = Val::global("context");
                println!("Playing");
                oscillator.call("connect", &argv![context.get("destination")]);
                oscillator.call("start", &argv![0]);
                println!("All done!");
                Val::undefined().as_handle()
            })
        ],
    );
    body.call("appendChild", &argv![elem]);
}
