# F4SE Common

This directory contains files related to functionality which is shared across xSE .dll files. There are a few files to take note of:

- **Branch Trampoline**: In order to get around a limitation in the x86_64 which limits us to relative JMPs of +/- 2GB, we create a bit of code within range which will get us to our desired destination further away. "Branch Trampolines" are xSE's name for this construct.
- **Relocation**: Among other things, the base address the game loads in at in virtual memory can change due to something called ASLR (Address Space Layout Randomization). In order for us to figure out where important parts of the game now reside in memory, we must figure out where they ended up, by calculating the new Base Address + Offset. Ideally, we only figure out what that base address is once, and then reuse it, so that we save on overhead. The relocation files handle this.
- **SafeWrite**: In order to write JMP or CALL instructions safely, we must take care of the permissions of the virtual memory page. This code provides some helper functions which modify the page to have READ/WRITE/EXECUTE permissions, write the JMP or CALL, then restore the previous permissions.
- **Utilities**: #TODO
- **Version**: #TODO (but probably just version information)
