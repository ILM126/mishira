Version history
===============

v1.0.0 (23 Aug 2014)
------------------------

No changes since v1.0.0-rc3.

v1.0.0-rc3 (16 Aug 2014)
------------------------

Bug fixes:
- Multiple stability fixes relating to accelerated window capture. Mishira should now not cause other applications to crash as much as they were before. These fixes may cause some applications to no longer be capturable.
- Stopped spamming the log file with "cannot execute command" messages when running on a 32-bit operating system.

v1.0.0-rc2 (12 Aug 2014)
------------------------

Bug fixes:
- Potentially fixed the audio mixer not mixing the audio from some audio devices that have incorrect sample timestamps
- Potentially fixed a crash that occurred after 5-10 minutes of execution for some users
- Potentially fixed the image layer not being able to open images that were selected with the "Browse..." button
- Fixed the "Stop broadcasting" button not working when 2 or more targets are enabled at once

New features:
- Added a help menu item for quickly opening the log file directory

v1.0.0-rc1 (10 Aug 2014)
------------------------

Initial release.
