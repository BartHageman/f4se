# F4SE Loader Common
The code in F4SE Loader Common [compiles to a static library](https://github.com/BartHageman/f4se/blob/8984f1af18d077d908b2e08ddfb7259e379eb7ed/f4se_loader_common/CMakeLists.txt#L42) which is utilized by f4se_loader, the executable file that loads the game and adds F4SE functionality using DLL injection.
The functionality in f4se_loader_common is not *fully* game-agnostic, as the name might imply. The common appears to refer to the architectural pattern which is repeated across the xSE loaders. Each of the files do a couple of things:
- **IdentifyEXE.cpp/.h** - Identify the executable file, and scan for incompatibilities. (e.g. Because you're playing on an unsupported patch, you're playing on a windows store version, you've renamed the launcher and are trying to play that instead, etc.)
- **Inject.cpp/.h** - Inject the code to load xSE into the game (More on this later. #TODO)
- **Steam.cpp/.h** - Handle detecting the presence of Steam and loading the game through Steam.
- **LoaderError.cpp/.h** - Small file in order to display errors using a Windows message box, in case something went wrong during loading.
