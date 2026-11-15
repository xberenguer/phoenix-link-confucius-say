# Troubleshooting

Start with the version shown in Game Menu. These notes describe 0.2.102; future releases may have different recovery wiring. The [controls](controls.md), [audio guide](audio.md) and [save guide](saving.md) contain the intended behavior.

## Buttons and autoplay

| Symptom | Check |
|---|---|
| Grey single press seems delayed | It waits through a 400 ms double-click window in the game view |
| Grey key does not open the panel immediately during a feature | Entry can be queued until the running sequence permits it; owed wager changes remain protected |
| Blue appears to stop autoplay rather than spin | A short blue press while autoplay is active turns it off |
| Holding blue is not toggling | Hold for about 850 ms; a chord or another held button is a different input path |
| Screen orientation changed | Double grey in normal play flips it; Display → Flip Screen also controls it |
| Green LED blinks | In the documented power-key download mode this is expected; release the power key and finish/exit download mode |

Use the large grey game-control button for menus. The small power key has the hardware power/download role; the game does not map it to wager selection.

## Wagers and credits

“SELECT PLAY 5” on the $1/10-line draft means that visible control's red bet row is not qualified; choose PLAY 5. It is not a fabricated $1/10-line pay deck. Other unavailable custom/legacy contexts can require service rather than silently repricing an owed game.

With auto-refill off, an unaffordable red BET choice cannot create a paid spin. Add supported play money at a safe boundary. Funding controls can be unavailable while a Free/Hold session remains owed. Changing the cash display or bankroll must not clear the meter.

Two outer reels showing matching results during Wisdom free games is the intended Twin mechanic. Matching Phoenix stops may carry different amounts. It is not evidence that all ordinary Base reels should duplicate one another.

## Audio

For no sound, check master sound, nonzero volume and the separate spin/landing switches. Then try the menu's test jingle and feature/award auditions. A submitted or queued audio diagnostic does not prove the physical speaker made sound.

For missing Hold/Twin music with functioning small cues, verify that both required filesystem files were installed and that the app matches them. An app-only update cannot create missing runtime data.

For crackle or unexpected resets at high volume, test at a lower level, compare USB and battery power, and consult the official [StickS3 speaker note](https://docs.m5stack.com/en/core/StickS3). The device maker recommends below 75% on battery. The reconstruction's loudness and exact source identity still need perceptual checking.

Not every Phoenix landing selects an arrival note, and not every Hold ending selects a coin celebration. Compare the actual event branch and award basis before assuming that autoplay suppressed a jingle. See [audio event order](audio.md).

## Graphics or missing animation

A missing Phoenix atlas can cause a fallback or unavailable artwork even when static UI text works. Both runtime assets are required. Verify the filesystem rather than replacing it with arbitrary images from a reference video.

Adjacent Confucius faces do not always join under the current policy: awarded adjacency is required. J/Q/K are currently static sprites rather than recovered unique factory sequences. The [graphics inventory](graphics.md) makes those limits explicit.

## Storage and USB

A checkpoint pause or service page protects a record it cannot safely advance. Do not erase it merely to make the warning disappear. Use read-only status/export, identify the exact version and retain the record for investigation.

If USB status is silent but animations and buttons still work, distinguish transport trouble from an application freeze. Reconnect with a known USB data cable and verify the expected board before flashing. Avoid opening several serial readers at once.

If a fresh build fails with “Bad CPU type” from a filesystem tool on an Apple-silicon host, use the staged portable `scripts/build_assets.py` described in [build and flash](build-and-flash.md). Version 92's historical [deferred-save limitation](saving.md#version-92s-save-timing-limit) was addressed for outcomes by version 93's storage barrier. For later versions, inspect the exact release's storage diagnostics and canonical record. A visual or RNG change does not resolve an unacknowledged storage operation.
