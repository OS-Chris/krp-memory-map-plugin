Kart Racing Pro Memory Map
=============

A plugin for Kart Racing Pro (http://www.kartracing-pro.com) that writes state data (i.e. telemetry & scoring information) to a memory mapped file (http://en.wikipedia.org/wiki/Memory-mapped_file) making it accessable to other applications.

CREDITS:
--------
**PiBoSo** - Kart Racing Pro & Example Plugin
**Christian Aylward** - KRP Memory Map Plugin

RELEASES:
--------
<a href="https://github.com/OS-Chris/krp-memory-map-plugin/downloads">1.0.0</a> - Works with KRP Beta 5

FEATURES:
--------
   * Incrementing IDs for each KRP data structure to track when updates have occured.

TODO:
-------
   * Update (if necessary) when the next KRP version (beta or final) is released.

USAGE:
--------
   * Get a pointer to the memory map using the the "named" file "Local\KartRacingProMemoryMap"
   * Make sure the plugin version is as expected (1st byte of the memory map)
   * Read data from the pointer (in a loop) using the SPluginsKartCombined_t structure as a guide
   * Release the memory map resources


