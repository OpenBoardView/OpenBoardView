## Open Board Viewer

Linux SDL/ImGui edition software for viewing .brd files, intended as a drop-in
replacement for the "Test_Link" software.


[![Demo Video](https://github.com/inflex/OpenBoardView/blob/inflex-ui-features/asset/screenshot.png)](https://www.youtube.com/watch?v=rObeatsf660)


[![Demo Video](https://github.com/inflex/OpenBoardView/blob/inflex-ui-features/asset/screenshot-halo.png)](https://www.youtube.com/watch?v=rObeatsf660)


### Features

- Part and pin sizes better represented
- Better outlining of irregular objects (such as connectors)
- Drag and drop
- Recently used file history
- Non-orthagonally orientated caps/resistors/diodes now drawn more realistically
- Works with multiple concurrent instances


### TODO

- Decode more board formats
- Compound project/file format


### Prerequisites

For Ubuntu developers, you'll need the following packages at a minium;

	$ apt-get install build-essential cmake libsdl2-dev libgtk-3-dev

### Installation

1. Clone the project

    $ git clone --recursive 'https://github.com/inflex/OpenBoardView'

2. Build it

    $ ./build.sh

3. Run it!

	$ ./openboardview.sh


### Usage

- N: Search by power net
- C: Search by component name

- w/a/s/d: pan viewport over board
- x: Reset zoom and center
- Mouse scroll, -/=: Zoom out/in
- Mouse click-hold-drag, Numeric pad up/down/left/right: pan viewport over board
- Numeric pad +/-: zoom board
- Numeric pad 5: Reset zoom and center
- Space, Middle mouse click: Flip board
- R/./Numpad-Del: Rotate clockwise
- ,/Numpad-Ins: Rotate counter-clockwise
- L: Show net list
- K: Show part list


### Discussion

IRC Freenode channel: **#openboardview**
