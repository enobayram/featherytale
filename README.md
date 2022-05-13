Feed The Bird (Working Title)
============================
![Screenshot](https://bitbucket.org/enobayram/feedthebird/raw/screenshots/screenshot.png "Screenshot")

Building
--------

This project depends on the [Orx game engine](http://orx-project.org/), 
[Boost](http://www.boost.org/) and Scroll(a tiny utility library for ORX). 
It actually also depends on Eigen3, but it's already included in the source tree.

The first step is to get Orx, either:

* download ([from here](http://sourceforge.net/projects/orx/files/orx/nightly/)) 
all the orx-nightly development versions for the target platforms you're planning to 
compile the engine against. Extract the archives to a folder (all of the versions 
to the same folder) of your choosing.

or

* clone [the orx repo](https://bitbucket.org/orx/orx) and build the binaries from source.
As of writing this, the project compiles against the `9e303f3` commit.

Then create an environment variable called `$ORX_DIR` that points to the extracted 
`orx-nightly` folder or the folder where you've cloned orx. Then go to the `$ORX_DIR` folder 
and issue `hg clone ssh://hg@bitbucket.org/enobayram/scroll` to download Scroll.

If you don't have mercurial, just download the relevant commit snapshot and put it there under
the folder `scroll`.

Building for Desktop Targets
----------------------------

For building the desktop versions, you need the 
[cmake](http://www.cmake.org/cmake/resources/software.html) program. 

###Building Pancar Engine on Windows

The engine should compile with MSVC 2013. MinGW segfaulted the last time I checked.

You'll need to download, build and *install* boost. In a nutshell, do in the boost 
root folder:

    >> bootstrap
    >> b2
    >> b2 install

Now, in order to build the Pancar Engine, create a directory called `.build` under 
`REPO_ROOT\physics`, start a command shell there and enter `cmake ..\src`. This will 
generate the .proj and .sln files for your MSVC version. Just open `airhockey.sln` 
and compile. 

	
###Building Pancar Engine on Linux

To build Pancar Engine:

    >> cd REPO_ROOT/physics
    >> mkdir .build
    >> cd .build
    >> cmake ../src
    >> make 

In order to generate an Eclipse project:

    >> cd REPO_ROOT/physics
    >> mkdir .build_eclipse
    >> cd .build_eclipse
    >> ../GenerateEclipseProject

Now you can import the project in Eclipse.

Building Pancar Engine for Android under Linux
----------------------------------------------

You first need to prepare an Android development environment:

* [Download the Android SDK](http://developer.android.com/sdk/index.html#download) 
and extract it somewhere. Create an environment variable `$ANDROID_HOME` pointing to 
that folder. Use the `tools/android` package manager to download the Android API 
version 21 `SDK Platform`, the `Extras/Android Support Repository` and 
`Extras/Android Support Library`. I also recommend you to add the `platform-tools` 
folder to your `PATH`.
* [Download the Android NDK](http://developer.android.com/tools/sdk/ndk/index.html) 
and extract it somewhere. Add that folder to your `PATH`.

Go to `REPO_ROOT/physics/android` and issue `./gradlew build` to compile the Android 
application. You can use `.gradlew installDebug` to install the `.apk` to a connected 
device, and `.gradlew runDebug` to run it on the device, if you've [set your device 
for debugging directly on it](http://developer.android.com/tools/device.html).

