﻿# README
Wii64 / Cube64
Beta 1.1

# LICENSE
    This software is licensed under the GNU General Public License v2
      which is available at: http://www.gnu.org/licenses/gpl-2.0.txt
    This requires any released modifications to be licensed similarly,
      and to have the source available.
    
    Wii64/Cube64 and their respective logos are trademarks of Team Wii64
      and should not be used in unofficial builds.

# QUICK USAGE
 * ROMs can be z64 (big-endian) or v64 (little endian), or .n64, of any size
 * To install: Extract the contents of wii64-beta1.1.zip to the root of your SD card
 * For SD/USB: Put ROMs in the directory named /wii64/roms,
    All save types will automatically be placed in /wii64/saves
 * For DVD: ROMs may be anywhere on the disc (requires DVDxV2 on Wii)
 * Load the executable from the HBC or in the loader of your choice
    Once loaded, select 'Load ROM' and choose the source and select the ROM to load
      (Note: to go up a directory select '..', B will exit the file browser)
 * Select 'Play Game' to play
   The game can be exited any time by pressing X and Y together on a GC pad or Classic Controller,
   1 and 2 together on a Wiimote (only with Nunchuck attached), or the reset button
     (Note: this must be done to save your game; it will not be done automatically)

# Controls
 * Controls are now fully configurable so any button on your controller can be mapped
 * The controller configuration screen presents each N64 button and allows you to toggle through sources
 * There are 4 configuration slots for each type of controller
   * To load a different, previously saved configuration, select the slot, and click 'Load'
   * After configuring the controls as desired, select the slot, and click 'Save'
   * After saving different configurations to the slots, be sure to save your configs in the input tab of the settings frame
 * Clicking 'Next Pad' will cycle through the N64 controllers assigned
 * There is an option to invert the Y axis of the N64's analog stick; by default this is 'Normal Y'
 * The 'Menu Combo' configuration allows you to select a button combination to return to the menu

# Settings
 - General
   * Native Saves Device: Choose where to load and save native game saves
   * Save States Device: Choose where to load and save save states
   * Select CPU Core: Choose whether to play games with pure interpreter
     (better compatibility) or dynarec (better speed)
   * Save settings.cfg: Save all of these settings either SD or USB (to be loaded automatically next time)
 - Video
   * Show FPS: Display the framerate in the top-left corner of the screen
   * Screen Mode: Select the aspect ratio of the display; 'Force 16:9' will pillar-box the in-game display
   * CPU Framebuffer: Enable for games which only draw directly to the
     framebuffer (this will only need to be set for some homebrew demos)
   * 2xSaI Tex: Scale and Interpolate in-game textures (unstable on GC)
   * FB Textures: Enable framebuffer textures (necessary for some games to
     render everything correctly (e.g. Zelda Subscreen), but can impact performance; unstable on GC)
 - Input
   * Configure Input: Select controllers to use in game
   * Configure Paks: Select which controller paks to use in which controllers
   * Configure Buttons: Enter the controller configuration screen described above
   * Save Button Configs: Save all of the controller configuration slots to SD or USB
   * Auto Load Slot: Select which slot to automatically be loaded for each type of controller
 - Audio
   * Disable Audio: Select to mute the sound
 - Saves
   * Auto Save Native Saves: When enabled, the emulator will automatically load
     saves from the selected device on ROM load and save when returning to the menu or
     turning off the console
   * Copy Saves: Not yet implemented
   * Delete Saves: Not yet implemented

# COMPATIBILITY LIST
 Please visit http://emulatemii.com/wii64/compatList/ to see what runs
 Report any issues to http://code.google.com/p/mupen64gc/issues/list

# CREDITS
 * Core Coder: tehpola
 * Graphics & Menu Coder: sepp256
 * General Coder: emu_kidid
 * Original mupen64: Hactarux
 * Artwork: drmr
 * Wii64 Demo ROM: marshallh
 * Compiled using devKitPro r19 and libOGC
     ( http://sourceforge.net/projects/devkitpro )
 * Visit us on www.emulatemii.com and http://code.google.com/p/mupen64gc

# CHANGELOG
Beta 1.1:
   * Dynarec improvements
     + Function linking
     + Recompiling more instructions (LWC1/LDC1/FP rounding/partially MTC0)
     * Execution from ROM
     * Branch comparisons compare 64-bits when necessary
   * glN64_GX improvements
     + Pillar-boxing 'Force 16:9' mode
     * Fixed projection matrix for Rects
     * Fixed viewport and scissoring edge cases
     * Fixed matrix transforms for several microcodes
     * Improved EFB handling wrt VI timing
     * Added upper limit to texture meta data
     * Improved FPS display and loading icon stability
   * Menu improvements
     + Reconfigurable button mapping
       + Save/Load button mapping from file
     + ROMs displayed in alphabetical order
     + Navigate filebrowser pages with R/L and +/- buttons
     + Navigate settings tabs with R/L and +/- buttons
     + Invalid ROM type detection
     + Error message if /wii64/roms doesn't exist
     * Autosave is now the default setting
   * Changed C-Stick deadzone
   * Compiled with devkitPPC r19 / libOGC 1.8.1
   * Fixed resetting flashrom on ROM reset
 Wii:
   * Compiled with new libDI / DVDx V2
   * DVD reads are now done with direct PowerPC access
   * Controller improvements
     + Rumble for Wiimote-based input
     + Wiimote-only controls
     + IR/Accelerometer analog input
   * Fixed ROM cache bug for ROMs > ROM cache size
 GC:
   * Settings.cfg saving fixed
   * Moved TLB & blocks array to ARAM
   * Increased recompiler code cache size
   - ARAM ROM cache
   + 2Mb MEM1 ROM cache
Beta 1:
   + Dynamic Recompiler
   + Expansion Pak Support (Wii only)
   + New menu system
      + Classic Controller support
      + Wiimote & nunchuck support
      + Settings saving
      + Auto load/save option for saves
   + rsp_hle RSP Plugin Port
   - rsp_hle-ppc RSP Plugin
   + glN64 features & bugfixes
      + 2xSaI
      + glN64 frame buffer textures (e.g. Zelda sub screen)
      + glN64 CPU Frame buffer (for homebrew demos)
   + libDI Wii DVD ROM loading support
   + Full TLB on Wii
   * Many many bug fixes
   * MEM2 ROM Cache for Wii improved (512MBiT ROM support)
   * Memory LUTs compacted
 r3xx:
   + glN64 Port
   + MEM2 ROM Cache for Wii (fits 32MB ROMs)
   + Save/Load on Wii Filesystem
   + Progressive video support
   + Embedded font support for Qoob users
   * Threaded audio
   * Various GX_gfx fixes
   * Reworked input plugin
     + Modular controller input
   + Developer Features submenu
     * Toggle FPS/Debug display
 r200:
   * Replaced GUI
     + Menu system
     + Menu file browser
   * Replaced file handling system
     + Modular file read/writes
   * Gameplay resumable after exiting to menu
   + Game reset option
   * Multiple ROMs may be loaded without crashing/freezing
   + Fancy splash screen and credits
   * Fixed software graphics support
   + Working audio (choppy)
   + Partial support for little-endian ROMs
   + Rumble pak support
 r46:
   + DVD loading
 r43:
   + GX graphics
   - Software graphics
 r40:
   * Any size ROM support from SD card
   * Pure Interpreter support only for now
   * Software GFX
   * Controller/Mempak support
   * Saving to SD card
   * Text UI
   * 4 MB RAM support (no expansion pack)


