# wxWTest - UDP Radar Plot Application

A cross-platform proof-of-concept GUI application that displays radar plots of data received over UDP sockets.

## Overview

This application receives data over a UDP socket consisting of two IEEE754 floating-point numbers:
- **Distance**: measured in nautical miles (nmi)
- **Angle**: measured in degrees (0° = true north, increases clockwise)

The application plots received data points on a graphical radar display showing concentric distance rings and cardinal direction markers.

## Features

- UDP socket listener on port 5000
- Real-time radar plot display
- Distance rings at 20 nmi intervals
- Cardinal direction markers (N, S, E, W)
- Cross-platform support (Linux, macOS, Windows)

## Requirements

### Build Requirements
- GCC or Clang with C++17 support
- wxWidgets 3.1+ development libraries
- Make

### Linux (Fedora 43)
```bash
sudo dnf install wxGTK-devel
```

### macOS
```bash
brew install wxwidgets
```

### Windows
Download and install wxWidgets from https://www.wxwidgets.org/

## Building

Simply run:
```bash
make
```

To clean build artifacts:
```bash
make clean
```

## Running

```bash
./wxwtest
```

The application will start listening for UDP datagrams on port 5000.

## UDP Data Format

Send data to `localhost:5000` (or the machine's IP address) with a datagram containing two IEEE754 float values:

**Byte Layout:**
```
Bytes 0-3:   float (distance in nmi)
Bytes 4-7:   float (angle in degrees, 0-360)
```

### Example (Python)
```python
import socket
import struct

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
distance = 45.5  # 45.5 nautical miles
angle = 135.0    # 135 degrees (southeast)
data = struct.pack('ff', distance, angle)
sock.sendto(data, ('localhost', 5000))
```

## Architecture

- **main.cpp**: Application entry point and wxApp implementation
- **RadarWindow.h/cpp**: Main window, radar display rendering, UDP socket handling
- **ListenThread**: Detached thread for non-blocking UDP reception

## Coordinate System

- 0° = True North (top of display)
- 90° = East (right)
- 180° = South (bottom)
- 270° = West (left)
- Angles increase clockwise

## Future Enhancements

- Windows 11 native support
- Configuration dialog for listening port
- Data logging
- Display zoom controls
- Multiple target tracking
- Compass rose overlay

## License

POC - Use as needed

## Author

Pete Rou