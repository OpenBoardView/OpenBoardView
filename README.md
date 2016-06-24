## Open Board Viewer

Linux SDL/ImGui edition software for viewing .brd files, intended as a drop-in replacement for the
"Test_Link" software.


[![Demo Video](https://github.com/inflex/OpenBoardView/blob/inflex-ui-features/asset/screenshot.png)](https://www.youtube.com/watch?v=1Pi5RGC-rJw)

### Installation

To clone this specific branch;

git clone -b inflex-ui-features https://github.com/inflex/OpenBoardView.git

Run ./build.sh followed by ./openboardview.sh

If you get issues with json11 not compiling then you will need to link in a copy manually to the ./src/json11 folder


### Usage

- Ctrl-O: Open a .brd file
- N: Search by power net
- C: Search by component name

- Numeric pad up/down/left/right: pan viewport over board
- Numeric pad +/-: zoom board
- Numeric pad 5: Reset zoom and center 
- Space: Flip board
- R|.: Rotate clockwise
- ,: Rotate counter-clockwise
- L: Show net list
- K: Show part list

