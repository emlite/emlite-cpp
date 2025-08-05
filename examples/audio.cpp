#include <math.h>
#include <stdio.h>

#include <emlite/emlite.hpp>

using namespace emlite;

int main() {
    emlite::init();
    auto AudioContext = Val::global("AudioContext");
    if (!AudioContext.as<bool>()) {
        printf("No global AudioContext, trying "
               "webkitAudioContext\n");
        AudioContext = Val::global("webkitAudioContext");
    }

    printf("Got an AudioContext\n");
    auto context    = AudioContext.new_();
    auto oscillator = context.call("createOscillator");

    printf("Configuring oscillator\n");
    oscillator.set("type", "triangle");
    oscillator.get("frequency").set("Value", 261.63);


    auto doc  = Val::global("document");
    auto body = doc.call("getElementsByTagName", "body")[0];
    auto btn  = doc.call("createElement", "BUTTON");
    btn.set("textContent", "Play!");
    emlite_val_inc_ref(oscillator.as_handle());
    btn.call(
        "addEventListener",
        "click",
        Val::make_fn([=](auto params) -> Val {
            printf("Playing\n");
            oscillator.call(
                "connect", context.get("destination")
            );
            oscillator.call("start", 0);
            printf("All done!\n");
            return Val::undefined();
        })
    );
    body.call("appendChild", btn);
}