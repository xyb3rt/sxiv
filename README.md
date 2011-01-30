sxiv: Simple (or small or suckless) X Image Viewer

sxiv is a really simple alternative to feh and qiv. Its only dependency is
imlib2. The primary goal for writing sxiv is to create an image viewer, which
only implements the most basic features required for fast image viewing. It
works nicely with tiling window managers and its code base should be kept small
and clean to make it easy for you to dig into it and customize it for your
needs.

Installation
------------
sxiv is built using the commands:

    $ make
    # make install

Please note, that the latter one requires root privileges.
By default, sxiv is installed using the prefix "/usr/local", so the full path
of the executable will be "/usr/local/bin/sxiv".

You can install it into a directory of your choice by changing the second
command to:

    # PREFIX="/your/dir" make install

All build-time specific settings can be found in the file "config.h". Please
check and change them, so that they fit your needs.

Usage
-----
sxiv supports the following command-line options:

    -d       scale all images to 100%, but fit large images into window
    -f       start in fullscreen mode
    -p       pixelize, i.e. turn off image anti-aliasing
    -s       scale all images to fit into window
    -v       print version information and exit
    -w WIDTHxHEIGHT
             set window width to WIDTH and height to HEIGHT
             (if HEIGHT is omitted, height is also set to WIDTH)
    -Z       same as `-z 100'
    -z ZOOM  scale all images to current zoom level, use ZOOM at startup

Use the following keys to control sxiv:

    q            Quit sxiv
    Escape       Quit sxiv and return an exit value of 2 (useful for scripting)
    n,Space      Go to the next image
    p,Backspace  Go to the previous image
    g/G          Go to first/last image
    [/]          Go 10 images backward/forward
    +,=          Zoom in
    -            Zoom out
    h,j,k,l      Pan image left/down/up/right (also with arrow keys)
    <,>          Rotate image (counter-)clockwise by 90 degrees
    f            Toggle fullscreen mode (requires an EWMH/NetWM compliant
                 window manager)
    a            Toggle anti-aliasing

Additionally, sxiv can be controlled via the following mouse commands:

    Button1           Go to the next image
    Button3           Go to the previous image
    ScrollUp          Pan image up
    ScrollDown        Pan image down
    Shift+ScrollUp    Pan image left
    Shift+ScrollDown  Pan image right
    Ctrl+ScrollUp     Zoom in
    Ctrl+ScrollDown   Zoom out
