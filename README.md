# Cab bus / Loconet / LCC utilities

Directories here:

CabBusCommunications - Talks to the CAB bus.  Handles the protocol conversions.
  All in C code.
LocoNetCommunications - Talks to LocoNet.  Handles all the protocol
 conversions that need to happen.
CabBusToLoconet.X - MPLABX project for PIC32.  Uses both CabBusCommunications
 and LocoNetCommunications to talk to both sides.
CabBusToLoconetComputer - A Linux/Unix program which uses both 
 CabBusCommunications and LocoNetCommunications to do the conversions.
Protocol - folder which contains protocol information for both LocoNet and the CabBus
LCC - Contains LCC related code
LoconetTCPUtil - Utilities related to using Loconet over TCP
GUI - Train GUI that lets you do things

## Notes:
* If you are using CabBusToLoconetComputer, make sure to set your latency timer to 1
for the USB to serial connection!  It won't work well otherwise:
`echo 1 > /sys/devices/pci0000:00/0000:00:14.0/usb1/1-3/1-3:1.0/ttyUSB0/latency_timer`

You can also make a udev rule like this:
```
ACTION=="add", SUBSYSTEM=="usb-serial", DRIVER=="ftdi_sio", ATTRS{serial}=="FTU7E2W0", ATTR{latency_timer}="1"
```
Change the serial that you are looking for to be the serial number of your FTDI cable.

## Building

Not all dependencies are available thru apt for debian/ubuntu based systems.  Support for APT is TBD.

Configuration files for vcpkg are available, you can try those.

```
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/home/robert/vcpkg/scripts/buildsystems/vcpkg.cmake -DBUILD_GUI=ON
```

### Known issue: vcpkg manifest install fails on libpq

A full vcpkg manifest install (letting CMake run it automatically off `vcpkg.json`) can end up
trying to build `libpq` from source -- pulled in transitively by Qt's SQL support -- which fails
during `configure` with:

```
configure: error:
When cross-compiling, either use the option --with-system-tzdata to use
existing time-zone data, or set the environment variable ZIC to a zic
program to use during the build.
```

This is unrelated to anything in this repo and isn't fixed here. If your build otherwise has
`Qt6`, `fmt`, `log4cxx` and `qt-advanced-docking-system` available already (e.g. installed under
`/usr/local` rather than through vcpkg), the only packages you actually need vcpkg for are
`imgui` and `imgui-node-editor` (used by the GUI's track panel). Install just those two into
whichever `vcpkg_installed` directory your build points at (check `VCPKG_INSTALLED_DIR` in that
build's `CMakeCache.txt` if you're not sure, e.g. Qt Creator gives each kit's build directory its
own `<build-dir>/vcpkg_installed`), then reconfigure with manifest install turned off so CMake
doesn't try -- and fail -- to redo it itself:

```bash
mkdir -p /tmp/imgui-scratch-manifest
cat > /tmp/imgui-scratch-manifest/vcpkg.json <<'EOF'
{
  "name": "imgui-scratch",
  "version-string": "0.1",
  "dependencies": ["imgui", "imgui-node-editor"]
}
EOF
cp vcpkg-configuration.json /tmp/imgui-scratch-manifest/  # keeps the same pinned versions

/path/to/vcpkg/vcpkg install \
  --x-manifest-root=/tmp/imgui-scratch-manifest \
  --x-install-root=<build-dir>/vcpkg_installed \
  --triplet=x64-linux

cmake -B <build-dir> -S . \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DBUILD_GUI=ON \
  -DVCPKG_INSTALLED_DIR=<build-dir>/vcpkg_installed \
  -DVCPKG_MANIFEST_INSTALL=OFF
```

For a Qt Creator-managed build this last `cmake` step already happens for you on reconfigure --
just run the `vcpkg install` step above pointed at that kit's `<build-dir>/vcpkg_installed` and
reconfigure the project.

## License:
 
 GPL v2 ONLY
