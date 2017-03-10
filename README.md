# simplewallpaper #
---------------------------
simplewallpaper is a simple wallpaper application that uses FIFO (file in file out) for updating current image. Its designed to be lightweight. Where it will only update the screen when changing image or when screen is being resized.

## Examples ##
------------
.1 Update current image with the command 'cat'. The second command will redirect STDOUT from cat to write to the wallfifo0 file. Which in succession will be redirect to 
the application and loaded into memory to create image to display.
```bash
swp -p ~/wallfifo0 -V
cat image.png > ~/wallfifo0 
```



## Dependencies ##
----------------
In order to compile the program, the following Debian packages has to be installed.
```bash
apt-get install libfreeimage-dev libsdl2-dev
```