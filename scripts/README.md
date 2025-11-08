# Papyrus Scripting Workflow
This directory contains a few helper scripts written in python, in order to do the following things:

- `update_scripts.py`: get the vanilla scripts from the game's directory (assuming you've extracted them from the zip file when you downloaded the Creation Kit).
- `build_multithread.py`: merge the added code specified in the `modified` directory with the vanilla files, then compile them into `.pex` files which the game can read, using the papyrus compiler.
- `sanitize.py`: By default, the papyrus compiler includes potentially sensitive information such as the build path, windows username, and PC name in each compiled script. This .py file sanitizes those away and replaces them with empty strings.

This order would generally be the workflow for building the scripts yourself as well. Import vanilla scripts, build, sanitize, then ship.

# One puzzling file
There is one more script here `generate_cmake.py`, which is used to update the lists of .h files and .cpp files which cmake uses in order to build the project. Explicitly declaring these is considered good practice, but tedious. This is could be why it has been automated here.
[See this file for an output example](f4se/cmake/headerlist.cmake)

As for why you shouldn't use cmake's GLOB functionality, see [this stackoverflow thread](https://stackoverflow.com/questions/32411963/why-is-cmake-file-glob-evil)
However, this post also mentions that there is new functionality as of 2020 which would take care of this.

The second, more problematic detail is that the script is actually currently bugged due to an incorrectly named variable: 
https://github.com/BartHageman/f4se/blob/e379bb194c2693809330bf640b7b8e6bbeb7c065/scripts/generate_cmake.py#L19-L23

This script hasn't been updated since it was added 5 years ago (in commit 8972565) which would suggest that it is currently unused.
