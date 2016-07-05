## Open Board Viewer

Linux SDL/ImGui edition software for viewing .brd files, intended as a drop-in
replacement for the "Test_Link" software.


[![Demo Video](https://github.com/inflex/OpenBoardView/blob/inflex-ui-features/asset/screenshot.png)](https://youtu.be/hR-3xZU3xQE)


### Features

- Part and pin sizes better represented
- Better outlining of irregular objects (such as connectors)
- Drag and drop
- Recently used file history
- Non-orthagonally orientated caps/resistors/diodes now drawn more realistically
- Works with multiple concurrent instances


### TODO

- Decode more board formats
- Adjustable colours
- Compound project/file format
- Configuration file


### Installation

1. Clone the project

    $ git clone --recursive 'https://github.com/inflex/OpenBoardView'

2. Build it!

    $ ./build.sh


### Usage

- Ctrl-O: Open a .brd file
- N: Search by power net
- C: Search by component name

- w/a/s/d: pan viewport over board
- -/=: Zoom out/in
- x: Reset zoom and center
- Numeric pad up/down/left/right: pan viewport over board
- Numeric pad +/-: zoom board
- Numeric pad 5: Reset zoom and center
- Space: Flip board
- R|.: Rotate clockwise
- ,: Rotate counter-clockwise
- L: Show net list
- K: Show part list


### Discussion

IRC Freenode channel: **#openboardview**
