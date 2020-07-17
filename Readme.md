Behind Jaggy Lines/BJL Loader
=============================

This is a modified version of Joe Britt's, 42Bastian Schick's and John Sohn's BJL 
uploader which should run fine under Windows Win95/98/ME/NT/2000/XP (Vista not
tested yet), as well as Linux.  The source has also been made 64-bit safe by using
fixed-size types for all non-internal data structures.

*Note: The windows binaries checked in here are from ZeroSquare's latest version
posted on Jagware here:*

  *<http://www.jagware.org/index.php?showtopic=484>*

*Modifications have been made to the source here since that version which are
not be reflected in the binaries.*

Usage is the same as the original loader program;

    lo_inp [-s skipbytes] [-w wait] [-b baseaddr] [-p lptbase] [-n] [-8] <filename>

       skipbytes is # bytes in header to skip.   default = 0
       baseaddr is addr to upload to.            default = 0x00004000
       lptbase specifies base addr of LPT port.  default = 0x378
       wait - counter between longs.             default = 1
       -n => don`t send switch-command
       -8 => use 8Bit transmission (4Bit is default)
       -d => Data upload, loader restarts
       -x => do not use high-priority mode.

For example, to load the program "jaghello.cof", you might run:

    $ lo_inp -8 jaghello.cof

Notable changes in ZeroSquare's version:
----------------------------------------

* The -w parameter effect has been tweaked, it should be a little more reliable.
* A progress indicator has been added.
* By default, the loader runs with a very high priority to avoid timing problems
  with other applications (which often result in a failed download). Use the -x 
  option if you want the normal-priority mode.
* Various tweaks and bugfixes ;)

Building - Linux
----------------

* Type "make".

Building - Windows
------------------

* ZeroSquare used Bloodshed Software's Dev-C++ 4.9.9.2
(http://www.bloodshed.net/devcpp.html) and Logix4u's inpout32.dll

General Notes
-------------

* If the "Waiting for upload..." screen does not appear, or if strange things happen 
  (red border, Jaguar crashes, etc.), add the -n option and select the upload type 
  manually by pressing A or C before uploading. 
* Protector:SE users : Use the -n option.
* If uploading with -8 doesn't work, try removing it to use the 4-bit mode which
  is slightly more reliable.
* If all else fails, add the -w x option.  x is a number; start with 1, and go
  upwards until things improve.
* Extensive testing has yet to be done. Use at your own risk. :-)

Linux Notes
-----------

* As this program uses low-level parallel port access, you need to run it with
  root permissions.
* If the program complains about I/O port accesses even tough you are root, there 
  is probably a conflict with parallel drivers (it didn't happen to me, but it's
  possible in theory). The likely culprit is the parport driver and its submodules
  (parport_pc, etc.) ; try to unload it with a "rmmod parport" before running the
  program. Unfortunately, parport is often built into the kernel image, so you'll 
  probably have to reconfigure and recompile your kernel to compile it as a module
  instead. Not fun :/

Windows Notes
-------------

* Under Windows XP (may also apply to WinNT and 2000), change the properties of
  your parallel port in Device Manager to disable the interrupt and Plug-and-Play
  detection. It has been reported that it works better that way. You may also need
  to disable EPP/ECP support in the BIOS.
  As this program use low-level parallel port access, you need to run it with
  Administrator privileges.
* You can try this registry patch to disable parallel port polling:
  <http://melabs.com/support/patches.htm>

This file was adapted from ZeroSquare's release notes in lo_inp.txt and postings
on the AtariAge forums in this thread:

  <https://atariage.com/forums/topic/168179-jaguar-dev-system>
