#pragma once

// Sticky for this boot. No reset or erase authorization is exposed at runtime.
// A blocked initialization erase must lead to the unknown-bank storage gate;
// this flag does not establish that Preferences is readable or writable.
bool startupNvsFullEraseBlocked();
