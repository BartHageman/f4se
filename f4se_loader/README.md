# F4SE_Loader

## Main functionality
This is the code responsible for creating the f4se_loader.exe.
It is what starts the game up in a hibernated state, then injects F4SE functionality into the game. It utilizes code from both f4se_loader_common and f4se_common.
The bulk of its functionality is described in `main.cpp`, in its `main` function. It is quite large, coming in at a hefty *392 lines*.

The exact lifecycle of F4SE_loader and the F4SE library will be explained in a different section #TODO

## Signature Checking
In order for the loader.exe not to be abused by malicious actors, the production version of the loader executable and f4se library are both signed. DLL injection is extremely powerful functionality, which is often also abused by malware. Because these signatures are checked, malware authors will not be able to use the signed loader in order to inject malicious payloads using their own library. However, if you build the loader executable from source and have an unsigned copy, these checks are skipped. That way, you're not hindered by the checks during development.
This functionality can be verified in `SigCheck.cpp/.h`

## Command line options
The loader also features several command line switches. These are handled in Options.cpp/.h.
A listing of the options is found below. This description is obtained from running `f4se_loader.exe -h` in a terminal. 
>[!NOTE]
>The version used at the time of writing was 0.7.2. The options available may have changed since then. Please run the command again on your version in order to see the most up-to-date version of the text.
```
usage: f4se_loader [options]

options:
  -h, -help - print this options list
  -editor - launch the construction set
  -priority <level> - set the launched program's priority
    above_normal
    below_normal
    high
    idle
    normal
    realtime
  -altexe <path> - set alternate exe path
  -altdll <path> - set alternate dll path
  -crconly - just identify the EXE, don't launch anything
  -waitforclose - wait for the launched program to close
  -v - print verbose messages to the console
  -noskiplauncher - does not skip the default Bethesda launcher window
                    note: specifying this option may cause compatibility problems
  -launchsteam - attempt to launch steam if it is not running
  -notimeout - don't automatically terminate the process if the proxy takes too long
  -affinity <mask> - set the processor affinity mask
  -forcesteamloader - does nothing, ignored for backwards compatibility
  -waitfordebugger - wait for a debugger to attach before beginning execution
```
