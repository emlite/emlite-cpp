#include <math.h>
#include <stdio.h>

#define EMLITE_IMPL
#include <emlite/emlite.hpp>

using namespace emlite;

int main() {
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
    oscillator.set("type", Val("triangle"));
    oscillator.get("frequency").set("Value", Val(261.63));

    Val::global().set("oscillator", oscillator);
    Val::global().set("context", context);

    auto doc = Val::global("document");
    auto body =
        doc.call("getElementsByTagName", Val("body"))[0];
    auto btn = doc.call("createElement", Val("BUTTON"));
    btn.set("textContent", Val("Play!"));
    btn.call(
        "addEventListener",
        Val("click"),
        Val::make_fn([](auto) -> Handle {
            auto oscillator = Val::global("oscillator");
            auto context    = Val::global("context");
            printf("Playing\n");
            oscillator.call(
                "connect", context.get("destination")
            );
            oscillator.call("start", Val(0));
            
            printf("All done!\n");
            return Val::undefined().as_handle();
        })
    );
    body.call("appendChild", btn);
}