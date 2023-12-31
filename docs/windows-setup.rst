Step 1: Toolchain for Windows: Quick Steps
==================================

Windows doesn't have a built-in "make" environment, so as well as installing the toolchain you will need a GNU-compatible environment. We use the MSYS2_ environment to provide.
 You don't need to use this environment all the time (you can use Eclipse_ or some other front-end), but it runs behind the scenes.

The quick setup is to download the Windows all-in-one toolchain & MSYS zip file from dl.espressif.com:

http://dl.espressif.com/dl/esp32_win32_msys2_environment_and_toolchain-20160816.zip

Unzip the zip file to C:\ and it will create an "msys32" directory with a pre-prepared environment.


Alternative Step 1: Configure toolchain & environment from scratch
==================================================================

As an alternative to getting a pre-prepared environment, you can set up the environment from scratch:

* Navigate to the MSYS2_ installer page and download the ``msys2-i686-xxxxxxx.exe`` installer executable (we only support a 32-bit MSYS environment, it works on both 32-bit and 64-bit Windows.)

* Run through the installer steps, and accept the "Run MSYS2 now" option at the end. A window will open with a MSYS2 terminal.

* The SDK repository on github contains a script in the tools directory titled ``windows_install_prerequisites.sh``. If you haven't downloaded the SDK yet, that's OK - you can just `download that one file in Raw format from here <http://github.com/espressif/esp-idf>`_. Save it somewhere on your computer.

* Type the path to the shell script into the MSYS2 terminal window. You can type it as a normal Windows path, but use forward-slashes instead of back-slashes. ie: ``C:/Users/myuser/Downloads/windows_install_prerequisites.sh``. You can read the script beforehand to check what it does.

* If you use the 201602 MSYS2 installer, the first time you run ``windows_install_prerequisites.sh`` it will update the MSYS2 core system. At the end of this update, you will be prompted to close the MSYS2 terminal and re-open. When you re-open after the update, re-run ``windows_install_prerequisites.sh``. The next version of MSYS2 (after 201602) will not need this interim step.

* The ``windows_install_prerequisites.sh`` script will download and install packages for ESP32 SDK support, and the ESP32 toolchain.

Note: You may encounter a bug where svchost.exe uses 100% CPU in Windows after setup is finished, resulting in the SDK building very slowly. Terminating svchost.exe or restarting Windows will solve this problem.

Another Alternative Step 1: Just download a toolchain
=====================================================

If you already have an MSYS2 install or want to do things differently, you can download just the toolchain here:

http://dl.espressif.com/dl/xtensa-esp32-elf-win32-1.22.0-59.zip

If you followed one of the above options for Step 1, you won't need this download.

Important: Just having this toolchain is *not enough* to use ESP-IDF on Windows. You will need GNU make, bash, and sed at minimum. The above environments provide all this, plus a host compiler (required for menuconfig support).

Step 2: Getting the SDK from github
===================================

Option 1: Direct Download
-------------------------

This is the quick option to get up and running with the SDK, but you'll have to re-download in order to update the SDK.

Navigate to the github repository and click Download. Unzip the SDK somewhere that you need to use it.

Option 2: Using git
-------------------

It takes longer to check out the SDK directly from git, but you'll have the option of using ``git pull`` to update the SDK sources.

Open an MSYS2 terminal window by running ``C:\msys32\msys2_shell.cmd``. The environment in this window is a bash shell.

Change to the directory you want to clone the SDK into by typing a command like this one: ``cd "C:/path/to/dir"`` (note the forward-slashes in the path). Then type ``git clone https://github.com/espressif/esp-idf.git``

If you'd rather use a Windows UI tool to manage your git repositories, this is also possible. A wide range are available.

Step 3: Starting a project
==========================

The SDK by itself does not build a binary to run on the ESP32. The binary "app" comes from a project in a different directory. Multiple projects can share the same ESP32 SDK.

The easiest way to start a project is to download the Getting Started project from github_.

The process is the same as for checking out the SDK from github. Change to the parent directory and run ``git clone https://github.com/espressif/esp-idf-template.git``.

Step 4: Configuring the project
===============================

Open an MSYS2 terminal window by running ``C:\msys32\msys2_shell.cmd``. The environment in this window is a bash shell.

Type a command like this to set the path to ESP-IDF directory: ``export SDK_PATH="C:/path/to/esp-idf"`` (note the forward-slashes not back-slashes for the path). If you don't want to run this command every time you open an MSYS2 window, create a new file in ``C:/msys32/etc/profile.d/`` and paste this line in - then it will be run each time you open an MYS2 terminal.

Use ``cd`` to change to the project directory (not the SDK directory.) Type ``make menuconfig`` to configure your project, then ``make`` to build it, ``make clean`` to remove built files, and ``make flash`` to flash (use the menuconfig to set the serial port for flashing.)

If you'd like to use the Eclipse IDE instead of running ``make``, check out the Eclipse setup guide in this directory.


.. _MSYS2: https://msys2.github.io/
.. _github: https://github.com/espressif/esp-idf-template
