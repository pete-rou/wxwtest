// RadarWindow.cpp

// Other code...

// Change line 35 from socket->GetLastError() to socket->LastError()
if (socket->LastError() != 0) {
    // Handle error
}

// Rest of the code...