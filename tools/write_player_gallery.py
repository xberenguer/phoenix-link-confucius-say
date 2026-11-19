#!/usr/bin/env python3
"""Write the public gallery from the catalog of recorded gameplay.

The catalog is the sole source of media paths. This script does not create,
alter, or stage game imagery. Rerun it after assembling the gameplay catalog.
"""

from __future__ import annotations

import argparse
import html
import json
from pathlib import Path


PLAY = "https://xberenguer.github.io/phoenix-link-confucius-say/"
PREFIX = "media/gameplay/"


class Gallery:
    def __init__(self, catalog: dict):
        self.images = {item["file"]: item for item in catalog["images"]}
        self.previews = {
            item["preview_for"].removeprefix("docs/"): item
            for item in catalog["images"] if item.get("preview_for")
        }
        self.complete = {
            item["event_id"]: item for item in catalog["images"]
            if item.get("role") == "complete-event"
        }
        self.used: set[str] = set()
        self.warnings: set[str] = set()
        self.motion_in_section: set[str] = set()
        self.complete_shown: set[str] = set()

    def image(self, file: str, alt: str, width: int | None = None) -> str:
        item = self.images.get(file)
        if not item:
            self.warnings.add(f"Missing catalog image: {file}")
            return ""
        self.used.add(file)
        w, h = item["dimensions"]
        width = min(width or 720, w)
        height = round(h * width / w)
        return (
            f'<a href="{html.escape(file, quote=True)}"><img '
            f'src="{html.escape(file, quote=True)}" '
            f'alt="{html.escape(alt, quote=True)}" width="{width}" '
            f'height="{height}" loading="lazy"></a>'
        )

    def section(self, heading: str, intro: str = "", anchor: str = "") -> str:
        self.motion_in_section.clear()
        lead = f'<a id="{anchor}"></a>\n\n' if anchor else ""
        return lead + f"## {heading}\n\n" + (intro + "\n\n" if intro else "")

    def animation_for(self, still: dict) -> dict | None:
        if still["file"] in self.previews:
            return self.previews[still["file"]]
        poster = self.images.get(still.get("poster_for", ""))
        if poster and poster["kind"] in ("animation", "symbol-animation"):
            return poster
        event_id = still.get("event_id")
        if event_id in self.complete:
            return self.complete[event_id]
        context = self.images.get(still.get("context_file", ""))
        if context and context["kind"] in ("animation", "symbol-animation"):
            return context
        if still.get("observed_prize"):
            motion = self.symbol(still.get("still_name", ""), motion=True, full=True)
            if motion:
                context = self.images.get(motion.get("context_file", ""))
                if context:
                    return self.complete.get(context.get("event_id"), context)
        # Named-prize stills retain an event anchor rather than context_file.
        return next((item for item in self.images.values()
                     if item.get("event_id") == event_id
                     and item["kind"] == "animation"
                     and item.get("part", 1) == 1), None)

    def full_recording(self, item: dict, selected: str = "") -> str:
        complete = self.complete.get(item.get("event_id"))
        if not complete or complete["file"] == selected:
            return ""
        file = complete["file"]
        if file in self.complete_shown:
            return ""
        self.complete_shown.add(file)
        seconds = complete.get("duration_ms", 0) / 1000
        duration = f" — {seconds:g} seconds" if seconds else ""
        return (
            "<details>\n<summary>Watch the complete recorded action"
            + duration + "</summary>\n\n"
            + self.image(file, "Complete recorded action, from beginning to end")
            + "\n\n</details>\n\n"
        )

    def pair(self, file: str, title: str, caption: str = "", *,
             moving: bool = True, width: int | None = None) -> str:
        item = self.images.get(file)
        if not item:
            self.warnings.add(f"Missing catalog image: {file}")
            return ""
        text = f"### {title}\n\n"
        if caption:
            text += caption + "\n\n"
        text += self.image(file, title, width) + "\n\n"
        if moving:
            motion = self.animation_for(item)
            if motion:
                motion_file = motion["file"]
                if motion_file not in self.motion_in_section:
                    self.motion_in_section.add(motion_file)
                    text += "**Animation**\n\n" + self.image(
                        motion_file, title + " — recorded animation", width
                    ) + "\n\n"
                else:
                    text += f"[Watch this stage in the animation above.]({motion_file})\n\n"
                text += self.full_recording(item, motion_file)
            else:
                self.warnings.add(f"No recorded animation paired with: {file}")
        return text

    def screen(self, name: str, title: str, caption: str = "", *,
               session: str = "primary", moving: bool = True) -> str:
        return self.pair(PREFIX + f"screens/{session}/{name}.png", title,
                         caption, moving=moving)

    def symbol(self, name: str, motion: bool = False,
               full: bool = False) -> dict | None:
        kind = "symbol-animation" if motion else "symbol-crop"
        choices = [item for item in self.images.values()
                   if item.get("symbol") == name and item["kind"] == kind]
        if full:
            wide = [item for item in choices if item["file"].endswith("-full.png")
                    or item["file"].endswith("-full.webp")]
            if wide:
                choices = wide
        else:
            narrow = [item for item in choices if "-full." not in item["file"]]
            if narrow:
                choices = narrow
        choices.sort(key=lambda item: (item.get("session_id") != "primary", item["file"]))
        return choices[0] if choices else None

    def symbol_image(self, item: dict | None, label: str) -> str:
        if not item:
            self.warnings.add("Missing symbol capture: " + label)
            return "Capture pending"
        # Integer scaling keeps the game's pixel artwork crisp.
        width = item.get("crop", [0, 0, 48, 35])[2] * 3
        return self.image(item["file"], label, width)

    def symbols(self) -> str:
        text = (
            "| Symbol | Still | Animation |\n|---|---|---|\n"
        )
        labels = [
            ("nine", "9 — winning blink"), ("ten", "10 — winning cycle"),
            ("jack", "Jack"), ("queen", "Queen"), ("king", "King"),
            ("bell", "Bell"), ("abacus", "Abacus"), ("book", "Book"),
            ("candle", "Candle — moving flame"),
            ("confucius", "Confucius"), ("wisdom", "Wisdom"),
            ("phoenix", "Phoenix"),
        ]
        for name, label in labels:
            still = self.symbol(name, full=name == "phoenix")
            motion = self.symbol(name, motion=True, full=name == "phoenix")
            motion_cell = self.symbol_image(motion, label + " animation") if motion else "Still artwork"
            text += (f"| **{label}** | {self.symbol_image(still, label)} | "
                     + motion_cell + " |\n")
        return text + "\n"

    def merge(self, rows: int) -> str:
        name = f"confucius-{rows}-row"
        still = self.symbol(name)
        motion = self.symbol(name, motion=True)
        text = f"### {rows}-row gold Confucius\n\n"
        text += "| Joined figure | Animation |\n|---|---|\n"
        text += "| " + self.symbol_image(still, f"{rows}-row gold Confucius")
        text += " | " + self.symbol_image(motion, f"{rows}-row Confucius merge animation") + " |\n\n"
        if motion:
            source = self.animation_for(motion)
            if source:
                text += "**The merge in the full game**\n\n"
                text += self.image(source["file"], f"{rows}-row merge and surrounding reels") + "\n\n"
                text += self.full_recording(motion, source["file"])
        return text

    def build(self) -> str:
        text = (
            "# Gallery\n\n"
            f"[▶ Play the game]({PLAY}) · [Full player guide](README.md)\n\n"
            "Explore the game in playing order: choose a wager, spin the reels, "
            "meet every symbol, and follow the bonus features. **Jackpots come last, "
            "with Grand at the very bottom.** Click any still or animation to open it separately.\n\n"
            "Every image is a frame or rectangular crop from actual play or actual menu navigation. "
            "The animations show the motion recorded in the game. Screens from different spins "
            "are separate examples, rather than one continuous round. "
            "[About the recordings](gallery-recording.md).\n\n"
            "[Spins](#3-spin-land-and-see-a-win) · [Reel symbols](#reel-symbols) · "
            "[Confucius merges](#confucius-merges) · [Free Games](#6-wisdom-and-twin-free-games) · "
            "[Hold & Spin](#7-phoenix-hold--spin-begins) · [Menus and artwork](#menus-and-artwork) · "
            "[Jackpots](#12-jackpots)\n\n"
        )
        text += self.section("1. Choose a denomination and a wager",
                             "Choose a coin value, a PLAY LINES option and a red BET button.")
        for name, title, session in [
            ("startup-denomination", "Choose the denomination", "primary"),
            ("startup-high-limit", "The High Limit selection", "menus"),
            ("startup-lines-and-bet", "Choose the lines and bet", "primary"),
        ]:
            text += self.screen(name, title, session=session, moving=False)
        text += self.section("2. Add play credits",
                             "Open the voucher, enter an amount and select INSERT CREDITS. All credits are pretend money.")
        for name, title in [
            ("unfunded-reels", "Before inserting credits"),
            ("credit-voucher", "Open the voucher"),
            ("credit-voucher-filled", "Enter the credit amount"),
            ("funded-reels", "Ready to play"),
        ]:
            text += self.screen(name, title, moving=False)
        text += self.section("3. Spin, land and see a win",
                             "The reels move, settle, then highlight the positions that won. The symbols shown are the ones that actually landed.")
        for name, title in [("base-spinning", "The reels in motion"),
                            ("base-landing", "The reels land")]:
            text += self.screen(name, title)
        first = next((item for item in self.images.values()
                      if item.get("still_name") == "first-spin-result"), None)
        if first:
            text += self.pair(first["file"], "A complete spin and its result",
                              "The animation follows the spin through its win presentation and back to the ready screen.")
        else:
            text += self.screen("base-ready", "The winning result")
        text += ("The 9 and 10 symbols deliberately blink during their winning cycle. "
                 "Their brief disappearance in the animation is part of the game’s win effect.\n\n")
        text += self.section("4. Every reel symbol",
                             "All twelve symbols can be opened on their own. Close-ups retain the actual reel background. Jack, Queen and King use still artwork; the animated symbols show their recorded movement.",
                             "reel-symbols")
        text += self.symbols()
        text += self.section("5. Confucius joins into gold",
                             "Qualifying adjacent faces join into a two- or three-row figure. Compare the finished figure with its joining animation and the surrounding reels.",
                             "confucius-merges")
        text += self.merge(2) + self.merge(3)
        text += self.section("6. Wisdom and Twin Free Games",
                             "Three or more red Wisdom symbols start six Twin Free Games. The examples below follow the trigger, paired reels, counters and win effects reached during play.")
        for name, title, caption in [
            ("free-free-intro", "Three Wisdom symbols start the feature", "A real trigger with the other symbols that landed alongside the Wisdom symbols."),
            ("free-spinning", "The free reels spin", ""),
            ("free-landing", "A Free Game lands", ""),
            ("free-ready", "The paired outer reels", "A later Free Game, shown after its reels have settled."),
            ("free-sage-expansion", "Confucius expands during Free Games", ""),
            ("free-spin-award", "The Free Game award appears", ""),
            ("free-spin-award-settle", "The award settles", ""),
            ("free-free-counter", "The Free Games counter", ""),
            ("free-collect", "Collecting within the feature", ""),
            ("free-win-transfer", "The win transfers", ""),
            ("free-return-fire", "The return transition", ""),
        ]:
            text += self.screen(name, title, caption)
        text += self.section("7. Phoenix Hold & Spin begins",
                             "Phoenix symbols stay in place while the remaining positions respin. The full-screen fire leads into the feature.")
        text += self.screen("hold-hold-fire", "The full-screen fire transition")
        text += self.screen("hold-hold-intro", "Entering Hold & Spin")
        hold_round = next((item for item in self.complete.values()
                           if item.get("session_id") == "loops"
                           and "action-000011-clip-011" in item.get("event_id", "")), None)
        if hold_round:
            text += "<details>\n<summary>Watch an entire Hold & Spin round, from the triggering spin to the final review</summary>\n\n"
            text += self.image(hold_round["file"], "A complete Hold and Spin round") + "\n\n</details>\n\n"
        text += self.section("8. Revival brings more respins",
                             "A revival appeared during the recorded feature. Watch the effect and the play that follows it.")
        text += self.screen("hold-phoenix-revival", "Phoenix revival")
        text += self.section("9. Collect the Phoenix values and return",
                             "The Phoenix collection, running total and return fire are separate stages of the same recorded collection.")
        for name, title in [
            ("ordinary-collect", "Collecting the Phoenix values"),
            ("ordinary-win-transfer", "Moving the win into the total"),
            ("ordinary-return-fire", "The return fire"),
        ]:
            text += self.screen(name, title)
        text += self.section("10. Features within features, and the finish",
                             "Hold & Spin can occur while Free Games are in progress. These captures also show the collection, coin effects and end of a feature reached in other recorded rounds.")
        for name, title in [
            ("hold-spinning", "Respins during Free Games"),
            ("hold-landing", "More Phoenix symbols land"),
            ("hold-ready", "The settled Hold & Spin board"),
            ("base-collect", "Another recorded collection"),
            ("base-win-transfer", "The win celebration"),
            ("base-feature-end", "The feature ends"),
        ]:
            text += self.screen(name, title)
        text += self.menus()
        text += self.jackpots()
        return text

    def menus(self) -> str:
        text = self.section("11. Menus and artwork",
                            "The menus are shown as they appeared during actual navigation. Editor screens retain their existing values; viewing them did not change those values.",
                            "menus-and-artwork")
        for name, title in [
            ("menu", "The game menu"), ("balance", "Balance and credits"),
            ("settings", "Settings"), ("sound", "Sound options"),
            ("volume", "Volume"), ("display", "Display options"),
            ("game-values", "Game values"),
            ("counter-editor-unmodified", "Collection counter editor, unchanged"),
        ]:
            text += self.screen(name, title, session="menus", moving=False)
        text += "### The six in-game rules pages\n\n"
        for page in range(1, 7):
            text += f"**Rules page {page}**\n\n" + self.image(
                PREFIX + f"screens/menus/rules-{page}.png", f"Rules page {page}"
            ) + "\n\n"
        text += "### Every payline chart\n\nAll 37 chart pages are grouped by line setting. [How paylines work](paylines.md).\n\n"
        for lines, pages in [(5, 1), (10, 2), (15, 3), (25, 5), (50, 9), (100, 17)]:
            text += f"<details>\n<summary>{lines} lines — {pages} chart {'page' if pages == 1 else 'pages'}</summary>\n\n"
            for page in range(1, pages + 1):
                text += f"**Page {page}**\n\n" + self.image(
                    PREFIX + f"screens/menus/paylines-{lines}-{page}.png",
                    f"{lines} lines, chart page {page}"
                ) + "\n\n"
            text += "</details>\n\n"
        text += "<details>\n<summary>The simulator return screens and their six detail pages</summary>\n\n"
        for name, title in [
            ("simulator-return-list", "Simulator return list"),
            ("return-help", "Return-screen help"),
        ] + [(f"return-detail-{page}", f"Return detail {page}") for page in range(1, 7)]:
            text += self.screen(name, title, session="menus", moving=False)
        text += "</details>\n\n### Artwork around the reels\n\nThese close-ups are cropped from the actual starting screen and funded game.\n\n"
        for name, title in [
            ("confucius-and-temple", "Confucius and the temple"),
            ("header", "The game header"), ("footer", "The credit and win displays"),
            ("collection-meter", "The collection meter"),
        ]:
            file = PREFIX + f"artwork/primary/{name}.png"
            text += self.pair(file, title, moving=file in self.previews)
        return text

    def jackpots(self) -> str:
        text = self.section("12. Jackpots",
                            "The named Phoenix prizes and recorded jackpot awards are collected here, after the ordinary game and menus. Grand is the final sequence.",
                            "12-jackpots")
        text += self.screen("jackpot-board", "The jackpot board", moving=False)
        board_motion = next((item for item in self.images.values()
                             if "jackpot-board-menu" in item.get("tags", [])
                             and item.get("role") == "complete-event"), None)
        if board_motion:
            text += "**Animation**\n\n" + self.image(
                board_motion["file"], "The animated jackpot information board"
            ) + "\n\n"
        text += "<details>\n<summary>Jackpot value menus, viewed without changing their values</summary>\n\n"
        text += self.screen("jackpot-picker", "Choose a jackpot value to view", session="menus", moving=False)
        for name in ("mini", "minor", "major", "grand"):
            text += self.screen(f"jackpot-editor-{name}-unmodified", name.title() + " value editor, unchanged", session="menus", moving=False)
        text += "</details>\n\n<a id=\"named-phoenix-prizes\"></a>\n\n### Every named Phoenix prize\n\n"
        text += ("All nine Mini, Minor and Major variants were found during actual play. "
                 "The still and animation in each row show that named prize with its surrounding reel pixels.\n\n"
                 "| Prize | Still | Animation |\n|---|---|---|\n")
        for name in ("mini", "minor", "major"):
            for multiplier in (1, 2, 3):
                key = f"prize-{name}-{multiplier}x"
                label = f"{name.title()} {multiplier}×"
                text += f"| **{label}** | " + self.symbol_image(self.symbol(key, full=True), label)
                text += " | " + self.symbol_image(self.symbol(key, motion=True, full=True), label + " animation") + " |\n"
        text += "\n<details>\n<summary>Open the original full-screen boards for these nine prizes</summary>\n\n"
        for name in ("mini", "minor", "major"):
            for multiplier in (1, 2, 3):
                text += self.screen(f"prize-{name}-{multiplier}x", f"{name.title()} {multiplier}× in the full game", session="prizes")
        text += "</details>\n\n### A Major award, from presentation to return\n\n"
        for suffix, title in [
            ("award-intro", "The Major award begins"),
            ("jackpot-award", "Major award presentation"),
            ("collect", "Collecting after Major"),
            ("win-transfer", "The Major win transfers"),
            ("return-fire", "Returning after Major"),
            ("ready", "The final Major result"),
        ]:
            text += self.screen("major-" + suffix, title, session="major")
        text += "### Grand — the final award sequence\n\nThis Grand was reached through continued play. Follow its presentation, collection, total and return.\n\n"
        for suffix, title in [
            ("award-intro", "The Grand award begins"),
            ("jackpot-award", "Grand award presentation"),
            ("collect", "Collecting after Grand"),
            ("win-transfer", "The Grand win transfers"),
            ("return-fire", "Returning after Grand"),
            ("ready", "The final Grand result"),
        ]:
            text += self.screen("grand-" + suffix, title)
        return text


def write(root: Path) -> dict:
    catalog = json.loads((root / "docs/gameplay-media.json").read_text())
    gallery = Gallery(catalog)
    text = gallery.build()
    (root / "docs/graphics.md").write_text(text.rstrip() + "\n")
    (root / "docs/symbol-gallery.md").write_text(
        "# Symbols and animations\n\nThe symbol gallery is now part of the complete "
        "[Gallery](graphics.md).\n\n"
        '<a id="reel-symbols"></a>\n\n'
        "[All twelve reel symbols, with their recorded animations](graphics.md#reel-symbols)\n\n"
        '<a id="confucius-merges"></a>\n\n'
        "[Two- and three-row Confucius merges](graphics.md#confucius-merges)\n\n"
        '<a id="named-phoenix-prizes"></a>\n\n'
        "[All nine named Phoenix prizes](graphics.md#named-phoenix-prizes)\n\n"
        '<a id="visible-cabinet-artwork"></a>\n\n'
        "[Menus and surrounding artwork](graphics.md#menus-and-artwork)\n"
    )
    (root / "docs/menu-gallery.md").write_text(
        "# Menus and artwork\n\nThe menu gallery is now part of the complete "
        "[Gallery](graphics.md#menus-and-artwork), including the settings, six rules "
        "pages, all 37 payline chart pages, return screens and artwork.\n\n"
        "[Jackpot value menus](graphics.md#12-jackpots) appear in the final jackpot section.\n"
    )
    readme = (root / "README.md").read_text()
    start = readme.index("## Take a look around")
    end = readme.index("\nGameplay pictures", start)
    table = (
        "## Take a look around\n\n"
        "| What would you like to see? | Open |\n|---|---|\n"
        "| The game in playing order: screenshots, animated symbols, merges, bonus features, menus and jackpots | [Gallery](docs/graphics.md) |\n"
        "| How to start playing, understand the screen and explore every feature | [Full player guide](docs/README.md) |\n"
    )
    (root / "README.md").write_text(readme[:start] + table + readme[end:])
    return {"catalog_images": len(catalog["images"]), "gallery_image_references": text.count("<img "),
            "unique_gallery_images": len(gallery.used), "warnings": sorted(gallery.warnings)}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    print(json.dumps(write(args.root), indent=2))


if __name__ == "__main__":
    main()
