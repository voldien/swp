# simplewallpaper
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

simplewallpaper is a simple wallpaper application that uses FIFO (file in file out) for updating the current image. It is designed to be lightweight. Where it will only update the screen when changing the image or when the screen is being resized.

## Examples

.1 Update the current image with the command 'cat'. The second command will redirect STDOUT from the cat to write to the wallfifo0 file. Which in succession will be redirected to the application and loaded into memory to create an image to display.
```bash
swp -p ~/wallfifo0 -V
cat image.png > ~/wallfifo0 
```

## Installation
The software can be easily installed with invoking the following command.
```bash
mkdir build && cd build
cmake ..
cmake --build .
make install
```

## Dependencies
In order to compile the program, the following Debian packages have to be installed.
It also depends on the OpenGL libraries.
```bash
apt-get install libfreeimage-dev libsdl2-dev
```
## License

This project is licensed under the GPL+3 License - see the [LICENSE](LICENSE) file for details