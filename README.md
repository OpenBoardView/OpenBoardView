## Open Board Viewer [![Build Status](https://travis-ci.org/OpenBoardView/OpenBoardView.svg?branch=master)](https://travis-ci.org/OpenBoardView/OpenBoardView)

Linux SDL/ImGui edition software for viewing .brd files, intended as a drop-in
replacement for the "Test_Link" software and "Landrex".


[![Walkthrough of OpenBoardView R7.2](http://img.youtube.com/vi/6CrNRo1UP5g/0.jpg)](http://www.youtube.com/watch?v=6CrNRo1UP5g "OpenBoardView R7.2 demonstration, with voice-over")

[![Demo Video](https://github.com/OpenBoardView/OpenBoardView/blob/master/asset/screenshot.png)]()


![Common net pin halo](https://github.com/OpenBoardView/OpenBoardView/blob/master/asset/screenshot-halo.png)

![Part searching](https://github.com/OpenBoardView/OpenBoardView/blob/master/asset/screenshot-partsearch.png)


### Features

- Annotations (per board database file)
- Part and pin sizes better represented
- Better outlining of irregular objects (such as connectors)
- Drag and drop
- Recently used file history
- Non-orthagonally orientated caps/resistors/diodes now drawn more realistically
- Adjustable DPI (for working on 2K/4K screens)
- Works with multiple concurrent instances


### TODO

- Decode more board formats
- Compound project/file format


### Prerequisites

#### Ubuntu 

For Ubuntu developers, you'll need the following packages at a minimum;

	$ apt-get install git build-essential cmake libsdl2-dev libgtk-3-dev
	
#### Fedora

For Fedora, you will need the following packages at a minimum;

	$ dnf groupinstall "Development Tools"

	$ dnf install SDL2-devel gtk3-devel fontconfig-devel 

### Installation

1. Clone the project

    $ git clone --recursive 'https://github.com/OpenBoardView/OpenBoardView'

2. Build it

    $ ./build.sh

3. Run it!

	$ ./bin/openboardview
	...or...
	$ ./openboardview.sh


### Usage

- Ctrl-O: Open file select dialog

- w/a/s/d: pan viewport over board
- x: Reset zoom and center
- Mouse scroll, -/=: Zoom out/in
- Mouse click-hold-drag, Numeric pad up/down/left/right: pan viewport over board
- Numeric pad +/-: zoom board
- Numeric pad 5: Reset zoom and center
- Space, Middle mouse click: Flip board
- R/./Numpad-Del: Rotate clockwise
- ,/Numpad-Ins: Rotate counter-clockwise

- /, Ctrl-F: Search
- ESC: Clear search results and selected parts

- p: Toggle pin display
- m: Mirror board across Y-axis

- L: Show net list
- K: Show part list


### Discussion

IRC Freenode channel: **#openboardview**
