sxiv: Simple (or small or suckless) X Image Viewer

sxiv is an alternative to feh and qiv. Its only dependency besides xlib is
imlib2. The primary goal for writing sxiv is to create an image viewer, which
only has the most basic features required for fast image viewing (the ones I
want). It works nicely with tiling window managers and its code base should be
kept small and clean to make it easy for you to dig into it and customize it
for your needs.

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
sxiv has two modes of operation: image and thumbnail mode. The default is image
mode, in which only the current image is shown. In thumbnail mode a grid of
small previews is displayed, making it easy to choose an image to open.

sxiv supports the following command-line options:

    -c           Remove all orphaned cache files from thumbnail cache and exit
    -d           Scale all images to 100%, but fit large images into window
    -F           Use size-hints to make the window fixed/floating
    -f           Start in fullscreen mode
    -g GEOMETRY  Set window position and size
                 (see section GEOMETRY SPECIFICATIONS of X(7))
    -p           Pixelize, i.e. turn off image anti-aliasing
    -q           Be quiet, disable warnings
    -r           Search given directories recursively for images
    -s           Scale all images to fit into window
    -t           Start in thumbnail mode
    -v           Print version information and exit
    -Z           Same as `-z 100'
    -z ZOOM      Scale all images to current zoom level, use ZOOM at startup

The following key mappings are available; differences between image view and
thumbnail mode are denoted via brackets:

    q            Quit sxiv
    Return       Switch to thumbnail mode [open selected image]

    n,Space      Go to the next image
    p,Backspace  Go to the previous image
    g/G          Go to [select] first/last image
    [/]          Go 10 images backward/forward

    +,=          Zoom in
    -            Zoom out
    0            Set zoom level to 100%
    w            Fit image into window

    h,j,k,l      Pan image [move selection] left/down/up/right
                 (also with arrow keys)
    H,J,K,L      Pan to left/bottom/top/right image edge

    <,>          Rotate image (counter-)clockwise by 90 degrees

    W            Resize window to fit image
    f            Toggle fullscreen mode (requires an EWMH/NetWM compliant
                 window manager)

    a            Toggle anti-aliasing
    A            Toggle visibility of alpha-channel, i.e. transparency
    D            Remove image from file list and go to [select] next image
    r            Reload image

Additionally, the following mouse mappings are available:

    Button1           Go to the next image
                      [select image/open image if it is already selected]
    Button2           Drag image with mouse while keeping it pressed
    Button3           Go to the previous image
    ScrollUp          Pan image up [scroll up one thumbnail row]
    ScrollDown        Pan image down [scroll down one thumbnail row]
    Shift+ScrollUp    Pan image left
    Shift+ScrollDown  Pan image right
    Ctrl+ScrollUp     Zoom in
    Ctrl+ScrollDown   Zoom out
