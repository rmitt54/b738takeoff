# 737-800 Takeoff Performance: X-Plane 12 plugin (C++)

V1, VR and V2 with flex (assumed temperature) and TO / TO 1 / TO 2 derates for a 737-800 (CFM56-7B26),
with runway, weather and engine-out climb checks. It is a native C++ port of the Python plugin and gives
the same numbers (500 random cases match). The window uses Dear ImGui.

Approximate model for simulation and study only. Not an AFM, QRH or airline tool. Not for real flight.

## Status

Written without access to X-Plane, so it has been checked in these ways only:
- the model and airport lookup match the Python version exactly (`test/test_model.cpp`)
- the interface runs headless and handles empty weight, airport lookup and all layouts (`test/test_ui.cpp`)
- the plugin compiles and links against stand-in X-Plane headers

It has not been run inside X-Plane 12, and it has not been built against the real X-Plane SDK.
If something fails, send me `Log.txt` from the X-Plane folder.

## Get the plugin files (.xpl)

You need one file per operating system: `win.xpl`, `mac.xpl`, `lin.xpl`.

### Easiest: let GitHub build it
1. Make a free GitHub account and create a new repository. Upload this whole folder to it.
2. Open the **Actions** tab, choose **Build plugin**, press **Run workflow**.
3. When it finishes, download the artifact for your system (Windows, macOS or Linux).
   If the SDK download step fails, get the current link from
   https://developer.x-plane.com/sdk/plugin-sdk-downloads/ and put it in `.github/workflows/build.yml` (`SDK_URL`).

### Or build it yourself
1. Download the X-Plane SDK from https://developer.x-plane.com/sdk/plugin-sdk-downloads/ and unzip it.
2. Install CMake and a C++ compiler (Visual Studio on Windows, Xcode tools on macOS, `g++` and `libgl1-mesa-dev` on Linux).
3. From this folder:

       cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DXPLANE_SDK=/path/to/SDK
       cmake --build build --config Release

   The result is in `build/b738_takeoff_perf/64/`.

## Install

Put the built file in X-Plane like this (only the files for your system are needed):

    X-Plane 12/Resources/plugins/b738_takeoff_perf/64/win.xpl
                                               /64/mac.xpl
                                               /64/lin.xpl

If you installed the Python version, delete `PI_B738TakeoffPerformance.py` first, otherwise both use the same
command and menu name.

## Use

Start X-Plane, then **Plugins > 737 Takeoff Performance**. The command `b738_takeoff_perf/toggle` can be bound to a key or joystick button.
The window can be resized and popped out like other X-Plane windows.

- Leave Weight empty to see only the runway and climb weight limits.
- Type an ICAO or IATA code. The airport is found as soon as the code is recognized, or when you press Enter.
- Pick a runway from the list. Length, slope and heading fill in.
- Weather comes from the sim unless you switch off "Use sim weather".

## Files

- `src/model.*` performance model
- `src/airports.*` airport and runway lookup (built-in data, then X-Plane apt.dat)
- `src/airportdb_data.cpp` built-in airport data (OurAirports, about 4,700 airports), made by `tools/make_db.py`
- `src/ui.*` the window contents
- `src/imgui_xp.*` ImGui drawing and input inside an X-Plane window
- `src/plugin.cpp` plugin entry points
- `third_party/imgui` Dear ImGui 1.91.5 (MIT license)

Airport data: OurAirports (public domain).
