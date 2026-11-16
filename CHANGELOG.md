# Current release notes

## 0.2.130

- New sessions open on freshly sampled reels. The opening display does not place a bet, pay a win, advance the counter, record a spin or start a bonus. Credits still start at 1,000 and autoplay starts off.
- Counter editing stays within 0–1886 throughout. The first digit alternates 0/1; subsequent digits follow the selected prefix. When a larger prefix makes the old remainder invalid, the editor fits it within the ceiling. Apply and Cancel retain their existing behavior; jackpot amount editing is unchanged.
- The browser game, player guide, gallery and signed firmware package describe the same final source.

<img src="docs/media/counter-editor130.webp" alt="Counter digits wrap within the valid range" width="720">

## Validation status

Final firmware and offline assets passed complete byte-exact readback on StickS3. Healthy startup verified 1,000 credits, zero actions, autoplay off and all 60 Phoenix animation frames. Browser checks covered three distinct unpaid startup boards, every counter boundary wrap, menus, exact cash values and offline/cache updates.

The startup regression compared 1,024 test entropy tapes and 8,192 following Base/Free/Hold actions with the prior source: financial states, entropy requests and the next random reservoir matched. Startup boards containing feature symbols remained idle and unawarded. All 7,548 legal counter value/focus transitions passed an independent oracle, and the actual menu path passed 2,511 checks with address/undefined-behavior sanitizers. These are software regressions, not measurements of casino frequencies.

Original casino probabilities and complete factory artwork/audio remain unrecovered. The [math reference](docs/math-and-evidence.md) explains the local model and its limits.
