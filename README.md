# TCP/IP - Sending Images
C++ Windows compiler version of a TCP/IP Protocol for sending and displaying images on a local network

[img-folder](/img-folder): sends and displays a folder of images one by one. The amount of time spent on each image can be adjusted <br>
[indiv-imgs](/indiv-imgs): sends and displays a singular image

# Usage
- Fill in the IP Address of the receiving PC in sender.cpp
- Adjust the number of connections in receiver.cpp if applicable (current code only allows 1 connection)
- Run receiver.cpp on the pc receiving/displaying the images first, then run sender.cpp on the pc sending
- sender.cpp takes the path to the image or folder of images as a commandline argument.

