How to build for Linux {#HowToBuildLinux}
================

Prerequisites
-------------

You will need to install the following libraries:
  * uuid-dev
  * SFML-2.5.1
  * Qt 5.11 (optional)

A good way to do so is via apt-get on supported distros:
```
sudo apt-get install -y cmake uuid-dev libx11-dev qt5-default build-essential
```
If SFML is too old in your distro you can use the following steps to install the required version:
```
wget https://www.sfml-dev.org/files/SFML-2.5.1-linux-gcc-64-bit.tar.gz
sudo mkdir /usr/SFML
sudo tar xf SFML-2.5.1-linux-gcc-64-bit.tar.gz -C /usr/SFML/
```
If the same is true for your distro's Qt version, you can either use Qt's web installer or use one of the many pre-build qt package sources, e.g. https://launchpad.net/~beineri.
```
sudo add-apt-repository ppa:beineri/opt-qt-5.11.1-xenial
sudo apt update
sudo apt install qt511-meta-full
```


Using the command line
----------------
Run CMake with `CMAKE_PREFIX_PATH` pointing to the dependencies listed above. In this example, a `build` folder is created under the root of the repo and cmake is executed in it:
```
cmake -DCMAKE_PREFIX_PATH=/opt/qt511/;/usr/SFML/SFML-2.5.1;/usr/SFML/SFML-2.5.1/lib/cmake/SFML -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_COMPILER=g++-7 -DCMAKE_C_COMPILER=gcc-7 -G "Unix Makefiles" ../
```

Using Qt Creator
----------------
The root of the repository can also be opened in Qt Creator which will generally do a good job at finding the Qt location on it one but the path to SFML will probably still have to be provided manually.