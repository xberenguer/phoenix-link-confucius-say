# Sounds, music and event order

Audio belongs to game events, not to a generic jingle on every stopped reel. The game includes reconstructed common cues, dedicated feature-entry material, a long Hold/award stream and a separate reconstructed Twin phrase. These are not a recovered original manufacturer soundbank, and acoustic identity has not been established.

The [audio API](../firmware/src/audio.h), [registered cue bank](../firmware/src/audio_cues.cpp), [UI routing](../firmware/src/ui.cpp) and [music worker](../firmware/src/audio_music.cpp) are the implementation authority. A cue enum existing does not mean that it is registered or dispatched in every state.

## Common play sounds

| Event | Current routing |
|---|---|
| Reels begin moving | Looping reconstructed reel bed |
| A reel/group impacts | Finite reel-stop or selected Phoenix-arrival family, with ordered pending tails |
| Phoenix anticipation/arrival | Initial, Follow or Third family where the current landing selector qualifies it |
| Awarded Confucius/Sage win | Scoped win cue; no generic flourish on every winning or stopped reel |
| Cash added | Dedicated cash-in cue |
| A Phoenix amount is collected | Collection/count-up cue tied to the pickup event |
| Three-Wisdom initial entry | Dedicated measured 7.12-second announcement cue |

The Phoenix families are **not** a random rising scale and not simply one note for every Phoenix. Current selection examines which presented reel group contains arrivals, how many Phoenixes have already appeared and whether the natural trigger remains possible. The Third-family branch is limited to separately presented ordinary columns; quick-stop/coalesced, Twin and Hold cases retain their scoped behavior. This logic remains a partial reconstruction, not a recovered original event table. See [landing_cues.h](../firmware/src/landing_cues.h).

The bank includes analytically generated material and fresh-phase spectral reconstructions from repeated machine events. The longer clean-reference feature stream has a different provenance. Clearer reconstructed audio can still have an imperfect timbre, envelope or melody; no single “procedural” label describes the entire current bank.

## Feature music programs

| Program | Lifecycle |
|---|---|
| Hold | Independent streamed bed during the bonus; first respin can wait for an audible opening lead |
| Twin Free | Full 27.911-second reconstructed phrase, restarting after its loop boundary |
| Collected | A distinct collection/completion portion rather than the Twin bed |
| Major award | Finite named-award program with its own timing and tail |
| Big award | Finite qualifying cash/coin celebration program |
| Off | Stop requesting a feature bed while preserving the worker's applicable finite-tail policy |

Twin uses immutable mono PCM at 22,050 Hz. START requests its phrase before the first free-spin preparation. It remains eligible through the last Free presentation even if the Engine has already switched its next action to Base. Hold, collection and finite awards take priority. Returning from nested Hold starts a fresh Twin phrase during the source-fitted return fire; it does not resume an arbitrary old elapsed phase.

The longer program is stored in [feature_music.mp3](../firmware/data/feature_music.mp3); the Twin bank is compiled in [twin_music42.cpp](../firmware/src/twin_music42.cpp). Source reconstruction and playback streaming are separate from any perceptual claim of exact vendor identity.

## The reference video's “1:27” passage

The clean [qQ4gPanpkdU reference around 1:27](https://www.youtube.com/watch?v=qQ4gPanpkdU&t=87s) helped identify a celebration program. Its timestamp is not a rule saying every Hold should play that phrase 87 seconds after entry.

In the simulator, ordinary collection, named awards, large-cash preludes and a qualifying final-Free combined result have separate routes. A qualifying coin fountain is coordinated with its award program. The cash threshold is still a provisional presentation policy; having fourteen Phoenixes does not by itself select the coin phrase. See [Hold endings](hold-and-spin.md#count-up-and-ending-order).

Manual play and autoplay share those event routes. Backend queue acceptance proves software submission, not the speaker's acoustic onset. Software diagnostics cannot alone establish that all audible stops are aligned to a camera frame.

## Settings and auditions

Hold grey → Sound provides master mute, separate spin and stop/landing switches, fine volume and test playback. Disabling the reel bed does not disable win/feature music. Master mute does silence both categories.

Volume moves in 1% steps through the quiet 0–10% range, then 5% steps. It clamps at either end instead of wrapping a quiet setting to full volume. In the volume page, blue raises and grey lowers it.

<img src="media/volume.png" alt="Fine-volume page, synthetic demonstration state" width="720">

PLAY TEST JINGLE auditions the dedicated Wisdom entry; PLAY FEATURE MUSIC auditions the feature program; PLAY AWARD JINGLE auditions the award program. An audition is not a paid spin and must not change the meter, bank or RNG. Twin has an explicit developer audition path; no extra invented player button is implied by that API.

## Bank and playback checks

The spin bed and Initial arrival PCM reproduce from analytic recipes; they do not contain a sampled crowd recording. Finite landing submissions retain their queued tails in bounded backend tests, without a fixed added half-second delay. That checks digital routing, not the physical speaker's onset or acoustic cleanliness. MP3 completion/award and the direct Twin bank have separate cache/decoder checks.

Initial three-Wisdom entry uses its dedicated cue. Some retrigger/delayed-Free branches use a separate reconstructed FreeStart; their exact original melody and timing remain under investigation. Registered compatibility cues are not evidence that normal Hold dispatches those cues.

## Remaining work

Original cue registers, all landing eligibility cases, recording-room transfer functions, exactly matched envelopes, missing source variants and perceived speaker balance remain incomplete. A low level or deliberate silence must not be “fixed” by adding an unsupported generic jingle.

For crackle, missing music or battery resets, use [troubleshooting](troubleshooting.md#audio). The official [StickS3 hardware notes](https://docs.m5stack.com/en/core/StickS3) recommend keeping battery-powered speaker volume below 75% to reduce unexpected power-related resets.
