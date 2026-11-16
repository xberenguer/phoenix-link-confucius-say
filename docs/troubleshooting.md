# Troubleshooting

Match Game Menu's version to the current [release identity](../RELEASE.json). [Controls](controls.md), [audio](audio.md) and [fresh-session rules](saving.md) describe the intended behavior.

## Progress disappeared after restart

Every device boot, flash and browser reload starts a fresh manual session with 1,000 default denomination credits. There is no restore/import flow. A hidden browser tab can continue its still-live session when shown again; reloading it creates a new one.

## Grey opens wagers instead of settings

Short-grey opens the cabinet panel. Hold-grey exits that panel directly to Game, or opens settings from gameplay. Double-grey opens the live Jackpots board; use Display → Flip Screen for rotation. The short action waits through the 400 ms double-click window. See [controls](controls.md).

## Unavailable wager or funding

The menu offers both $1/5-line and $1/10-line play; the latter has multipliers 1, 2, 3, 4, 5 and the [documented family-adaptation scope](wagers.md). A funded red BET button starts immediately. Merely browsing/cancelling does not stake credits.

An unaffordable wager asks for play money. Credit additions are available at safe Ready/Base boundaries; an unfinished feature or service/capacity fault can block it. Funding does not clear the session's meters. See [settings](settings.md#credits).

## Random source error

The page says **RANDOM SOURCE ERROR / PLAY STOPPED / NO OUTCOME WAS COMMITTED**. A random-source action failed; no partial new stake or prize was committed. Repeated presses do not re-deal it. Restart for a new session. If it recurs, verify the intended build/board or browser WebCrypto support rather than enabling a weak fallback.

Host randomness tests do not establish physical entropy quality. The supported native build relies on its pinned offline S3 entropy prerequisites; changing ADC/RF/microphone ownership requires another review.

<a id="audio"></a>

## No sound or crackle

Use the Sound menu's master sound, stops/landings, fine volume and test entries. Browsers may require an initial gesture. A missing required music file is different from a muted channel. Browser and Stick speakers/mixing have different timing and acoustic behavior.

For crackle/resets on battery, lower volume and consult [M5Stack's speaker guidance](https://docs.m5stack.com/en/core/StickS3). Check USB and battery behavior separately; source fidelity remains a listening question.

## Paused feature or missing artwork

Free and Hold use their own START/continuation phases. A moving or collecting board need not be ready for a new paid spin. Hold/Free work stays in its live round; a restart intentionally ends that session.

The application needs both the Phoenix atlas and feature music filesystem. Check the [image roles and offsets](build-and-flash.md) when files are missing. Source qualification and asset failure messages should be investigated with current status/build hashes.

## USB and build tools

If buttons/animation work but USB status is silent, check the data cable, actual port and competing serial readers. A timeout alone does not identify a game freeze. Verify the intended board before any flash.

For an Apple-silicon “Bad CPU type” filesystem-tool error, use the portable asset builder in [build and flash](build-and-flash.md). It does not require an incompatible host mkspiffs executable.

## Browser version

Reload the game while online to check for a new release. Once its complete bundle is available, older game tabs refresh automatically. Offline play uses the last complete cached release.
