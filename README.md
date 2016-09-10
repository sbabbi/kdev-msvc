# kdev-msvc
KDevelop plugin to read visual studio solution files.
This is a work in progress, so far it only works with _Visual Studio 2008_ solution files.

**What works**:
 * Open a solution file and get the project tree.
 * Build an entire solution (no output).
 * Includes/defines for the parser.

**What does not work**:
 * Debugging.
 * Editing the project/solution files in any way.
 * Solution file must have the same name as its parent directory.

**Notes**

To build your solution from KDevelop you need to set the path to the MSVC IDE executable in the project configuration page.
This is _devenv.com_ or _VCExpress.exe_ (if you have the express edition), and it is usually located under _C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\IDE_.

**Installation**

Build and copy _kdevmsvcmanager.dll_ to your KDevPlatform plugin directory (usually _/usr/lib/plugins/kdevplatform/26_ on linux).
