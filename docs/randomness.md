# How randomness works

Entropy is raw unpredictability. The game uses it to make random choices through a cryptographically secure random-number generator, or **CSPRNG**.

**Electrical noise inside the Stick → hardware random bytes → AES-256 CSPRNG → game choices**

## Where the Stick gets entropy

The Stick's ESP32-S3 chip has a hardware random-number generator. For offline use, Espressif provides an internal noise source: tiny fluctuations in an internal reference voltage. The chip's **ADC**, a circuit that measures voltage, samples that noise and feeds it into the hardware generator. The chip also mixes in samples from an asynchronous internal oscillator, but Espressif does not guarantee that secondary source alone is sufficient.

The game explicitly enables the internal noise source, gathers **48 bytes** with Espressif's hardware-random API, then releases the source. This works without Wi-Fi or Bluetooth. It does not record room sound or use the microphone. See [Espressif's ESP32-S3 explanation](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32s3/api-reference/system/random.html) and the game's [hardware-source code](../firmware/src/platform_entropy125.h).

Those bytes seed **Mbed TLS AES-256 CTR-DRBG**. Fresh hardware entropy is requested before each game action, including paid spins, Free Games and Hold & Spin respins. The generator supplies the random choices for that action; unused output is discarded afterward. A restart gets fresh entropy rather than replaying a fixed seed. The [browser version](browser-play.md) uses the browser's `crypto.getRandomValues` instead of the Stick's hardware source.

## What uses randomness

| Game element | What is randomly chosen |
|---|---|
| Opening screen | The unpaid opening reel symbols; this display awards nothing |
| Paid spins and Free Games | Reel symbols, subject to the current mode's rules and configured weights |
| Phoenix prizes | Cash values, eligible named prizes and prize multipliers |
| Hold & Spin respins | Whether an empty position receives a Phoenix, its prize, and any applicable revival chance |
| Confucius / Sage | Generated Wild arrangements and applicable expansion chances |
| Collection counter | Initial/reset number and hidden trigger point; a previously unused counter bucket is initialized when selected |
| Lucky Chance | The result, when called; this separate Engine rule is not offered as ordinary play in the current UI |

Other events follow rules. Counting a qualifying Phoenix advances the collection counter. Matching symbols pay according to the paytable. Filling all 15 Hold positions awards the Grand. These events do not need a separate random decision once their conditions are met.

Reel motion, coin particles, count-up animations and music present the result; they do not select another winning outcome. Editing credits or jackpot prices does not itself roll a new spin. See the [Engine](../firmware/src/phoenix_engine.h) for the complete rules.

## Is it secure?

Hardware entropy feeding a cryptographic generator is an established design; Espressif recommends a strong software generator such as CTR-DRBG when the hardware noise source is enabled only temporarily. This implementation enables the source during collection, refreshes the CSPRNG before each action and stops play if its source or generator reports a fault. It does not fall back to a clock-based seed.

That is the intended security design, not proof against every attack. This project has software checks and statistical testing, but no independent physical-chip entropy measurement or certification of the complete implementation. Its obvious stuck-output check cannot detect every possible weak source. See [math and evidence](math-and-evidence.md#outcome-generation) and [testing](testing.md) for the scope of the checks.

## Randomness and odds are different

The random generator chooses a result. The configured probabilities determine how often each result should occur. Symbols and prizes do **not** all have equal chances.

A secure generator makes choices difficult to predict; it does not establish that this simulator has the original cabinet's odds. Original casino probability tables remain unrecovered. See [the local probability model](math-and-evidence.md#the-local-reel-and-feature-law).
