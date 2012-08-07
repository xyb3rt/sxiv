sxiv: Simple (or small or suckless) X Image Viewer

sxiv is an alternative to feh and qiv. Its only dependencies besides xlib are
imlib2 and giflib. The primary goal for writing sxiv is to create an image
viewer, which only has the most basic features required for fast image viewing
(the ones I want). It has vi key bindings and works nicely with tiling window
managers.  Its code base should be kept small and clean to make it easy for you
to dig into it and customize it for your needs.

Features
--------

* Basic image operations, e.g. zooming, panning, rotating
* Customizable key and mouse button mappings (in *config.h*)
* Thumbnail mode: grid of selectable previews of all images
* Ability to cache thumbnails for fast re-loading
* Basic support for multi-frame images
* Load all frames from GIF files and play GIF animations
* Display image information in window title

Screenshots
-----------

Image mode:

  <img src="http://github.com/muennich/sxiv/raw/master/sample/image.png">

Thumbnail mode:

  <img src="http://github.com/muennich/sxiv/raw/master/sample/thumb.png">

Installation
------------
sxiv is built using the commands:

    $ make
    # make install

Please note, that the latter one requires root privileges.
By default, sxiv is installed using the prefix "/usr/local", so the full path
of the executable will be "/usr/local/bin/sxiv".

You can install sxiv into a directory of your choice by changing the second
command to:

    # make PREFIX="/your/dir" install

The build-time specific settings of sxiv can be found in the file *config.h*.
Please check and change them, so that they fit your needs.
If the file *config.h* does not already exist, then you have to create it with
the following command:

    $ make config.h

Usage
-----
sxiv has two modes of operation: image and thumbnail mode. The default is
image mode, in which only the current image is shown. In thumbnail mode a grid
of small previews is displayed, making it easy to choose an image to open.

sxiv supports the following command-line options:

    -b           Do not show info bar on bottom of window
    -c           Remove all orphaned cache files from thumbnail cache and exit
    -d           Scale all images to 100%, but fit large images into window
    -F           Use size-hints to make the window fixed/floating
    -f           Start in fullscreen mode
    -g GEOMETRY  Set window position and size
                 (see section GEOMETRY SPECIFICATIONS of X(7))
    -n NUM       Start at picture NUM
    -p           Pixelize, i.e. turn off image anti-aliasing
    -q           Be quiet, disable warnings
    -r           Search given directories recursively for images
    -s           Scale all images to fit into window
    -t           Start in thumbnail mode
    -v           Print version information and exit
    -Z           Same as `-z 100'
    -z ZOOM      Scale all images to current zoom level, use ZOOM at startup

The following general key commands are available:

    q            Quit sxiv
    Return       Switch to thumbnail mode / open selected image

    0-9          Prefix the next command with a number (denoted via [count])

    g            Go to first image
    G            Go to the last image, or image number [count]

    f            Toggle fullscreen mode (requires an EWMH/NetWM compliant
                 window manager)
    b            Toggle visibility of info bar on bottom of window
    A            Toggle visibility of alpha-channel, i.e. transparency

    r            Reload image
    R            Reload all thumbnails
    D            Remove image from file list and go to next image


The following additional key commands are available in *thumbnail mode*:

    h,j,k,l      Move selection left/down/up/right [count] times
    Ctrl-j,k     Scroll thumbnail grid one window height down/up

The following additional key commands are available in *image mode*:

    n,Space      Go [count] images forward
    p,Backspace  Go [count] images backward
    [,]          Go [count] * 10 images backward/forward

    Ctrl-n,p     Go to the next/previous frame of a multi-frame image
    Ctrl-Space   Play/pause animation of a multi-frame image

    +            Zoom in
    -            Zoom out
    =            Set zoom level to 100%, or [count]%
    w            Fit image into window
    e            Fit image width to window width
    E            Fit image height to window height

    h,j,k,l      Pan image 1/5 of window width/height or [count] pixels
                 left/down/up/right (also with arrow keys)
    H,J,K,L      Pan to left/bottom/top/right image edge
    Ctrl-h,j,k,l Pan image one window width/height left/down/up/right
                 (also with Ctrl-arrow keys)

    <,>          Rotate image (counter-)clockwise by 90 degrees
    \,|          Flip image horizontally/vertically

    a            Toggle anti-aliasing
    W            Resize window to fit image

Additionally, the following mouse commands are available in *image mode*:

    Button1      Go to the next image
    Button2      Drag image with mouse while keeping it pressed
    Button3      Go to the previous image
    Scroll       Pan image up/down
    Shift+Scroll Pan image left/right
    Ctrl+Scroll  Zoom in/out
