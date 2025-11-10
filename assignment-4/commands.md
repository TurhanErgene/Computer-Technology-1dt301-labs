# Commands

## General

Clear the build folder, and go into it:
`rm -rf build && mkdir build && cd build`

Initialize cmake (use after changing cmake file):
`cmake .. -DPICO_BOARD=pico_w -DPICO_SDK_FETCH_FROM_GIT=on`

Build:
`cmake --build .`

Copy the compiled program onto the Pico:
`cp main.uf2 /Volumes/RPI-RP2/`


## Minicom

List the connected devices (i think?):
`ls /dev/cu.usb* /dev/tty.usb*`

Open minicom for the pico:
`minicom -o -b 115200 -D /dev/cu.usbmodemXXXXX`