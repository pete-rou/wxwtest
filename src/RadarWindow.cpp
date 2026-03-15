#include "RadarWindow.h"
#include <wx/thread.h>
#include <wx/event.h>
#include <algorithm>
#include <iostream>
#include <cstring>

wxDEFINE_EVENT(wxEVT_THREAD_RADAR_DATA, wxThreadEvent);

wxBEGIN_EVENT_TABLE(RadarWindow, wxFrame)
    EVT_PAINT(RadarWindow::OnPaint)
    EVT_SIZE(RadarWindow::OnSize)
    EVT_CLOSE(RadarWindow::OnClose)
wxEND_EVENT_TABLE()

class ListenThread : public wxThread
{
private:
    RadarWindow *window;
    wxDatagramSocket *socket;

public:
    ListenThread(RadarWindow *w, wxDatagramSocket *s)
        : wxThread(wxTHREAD_DETACHED), window(w), socket(s) {}

    virtual ExitCode Entry()
    {
        uint8_t buffer[8];
        wxIPV4address addr;
        
        while (!TestDestroy())
        {
            socket->RecvFrom(addr, buffer, sizeof(buffer));
            
            if (socket->GetLastError() == wxSOCKET_NOERROR)
            {
                // Parse two IEEE754 floats from the datagram
                float *data = (float *)buffer;
                float distance = data[0];
                float angle = data[1];
                
                // Create a custom event and post it to the main window
                wxThreadEvent *event = new wxThreadEvent(wxEVT_THREAD_RADAR_DATA);
                event->SetPayload<RadarPoint>({distance, angle});
                wxQueueEvent(window, event);
            }
        }
        return nullptr;
    }
};

RadarWindow::RadarWindow(const wxString &title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600)),
      maxDistance(100.0f),
      listenThread(nullptr)
{
    // Create main panel
    drawPanel = new wxPanel(this, wxID_ANY);
    drawPanel->SetBackgroundColour(*wxBLACK);
    drawPanel->Bind(wxEVT_PAINT, &RadarWindow::OnPaint, this);
    drawPanel->Bind(wxEVT_SIZE, &RadarWindow::OnSize, this);
    Bind(wxEVT_CLOSE_WINDOW, &RadarWindow::OnClose, this);
    
    // Custom event for radar data
    Bind(wxEVT_THREAD_RADAR_DATA, [this](wxThreadEvent &event)
    {
        RadarPoint point = event.GetPayload<RadarPoint>();
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            pointQueue.push(point);
        }
        drawPanel->Refresh();
    });

    // Create UDP socket
    socket = new wxDatagramSocket(wxIPV4address(), wxSOCKET_NOWAIT);
    wxIPV4address addr;
    addr.AnyAddress();
    addr.Service(5000);  // Listen on port 5000
    
    if (!socket->Bind(addr))
    {
        wxLogError(wxT("Failed to bind UDP socket to port 5000"));
        delete socket;
        socket = nullptr;
        return;
    }

    std::cout << "UDP Socket listening on port 5000" << std::endl;

    // Update center point and scaling
    OnSize(wxSizeEvent());

    // Start listening thread
    StartListening();
}

RadarWindow::~RadarWindow()
{
    if (listenThread)
    {
        listenThread->Delete();
    }
    if (socket)
    {
        socket->Destroy();
    }
}

void RadarWindow::StartListening()
{
    if (socket)
    {
        listenThread = new ListenThread(this, socket);
        if (listenThread->Create() != wxTHREAD_NO_ERROR)
        {
            wxLogError(wxT("Could not create listen thread"));
            delete listenThread;
            listenThread = nullptr;
        }
        else if (listenThread->Run() != wxTHREAD_NO_ERROR)
        {
            wxLogError(wxT("Could not run listen thread"));
            delete listenThread;
            listenThread = nullptr;
        }
    }
}

void RadarWindow::OnSize(wxSizeEvent &event)
{
    if (drawPanel)
    {
        wxSize size = drawPanel->GetClientSize();
        centerPoint = wxPoint(size.GetWidth() / 2, size.GetHeight() / 2);
        
        // Calculate pixels per nautical mile
        // Use the smaller dimension to ensure radar fits on screen
        int radarRadius = std::min(size.GetWidth(), size.GetHeight()) / 2 - 20;
        pixelsPerNmi = static_cast<float>(radarRadius) / maxDistance;
    }
    event.Skip();
}

void RadarWindow::OnPaint(wxPaintEvent &event)
{
    wxPaintDC dc(drawPanel);
    DrawRadar(dc);
    
    // Draw all points in queue
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        while (!pointQueue.empty())
        {
            RadarPoint point = pointQueue.front();
            pointQueue.pop();
            DrawPoint(dc, point);
        }
    }
}

void RadarWindow::DrawRadar(wxDC &dc)
{
    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();
    
    dc.SetPen(wxPen(*wxGREEN, 1));
    dc.SetTextForeground(*wxGREEN);
    
    // Draw concentric circles at 20 nmi intervals
    int numRings = static_cast<int>(maxDistance / 20.0f);
    for (int i = 1; i <= numRings; ++i)
    {
        int radius = static_cast<int>(i * 20.0f * pixelsPerNmi);
        dc.DrawCircle(centerPoint, radius);
        
        // Draw distance label
        wxString label = wxString::Format(wxT("%d nmi"), i * 20);
        dc.DrawText(label, centerPoint.x + radius + 5, centerPoint.y - 10);
    }
    
    // Draw cardinal direction lines and labels
    int axisLength = static_cast<int>(maxDistance * pixelsPerNmi);
    
    // North (0°) - up
    dc.DrawLine(centerPoint.x, centerPoint.y - axisLength,
                centerPoint.x, centerPoint.y);
    dc.DrawText(wxT("N"), centerPoint.x - 10, centerPoint.y - axisLength - 20);
    
    // South (180°) - down
    dc.DrawLine(centerPoint.x, centerPoint.y,
                centerPoint.x, centerPoint.y + axisLength);
    dc.DrawText(wxT("S"), centerPoint.x - 10, centerPoint.y + axisLength + 10);
    
    // East (90°) - right
    dc.DrawLine(centerPoint.x, centerPoint.y,
                centerPoint.x + axisLength, centerPoint.y);
    dc.DrawText(wxT("E"), centerPoint.x + axisLength + 10, centerPoint.y - 10);
    
    // West (270°) - left
    dc.DrawLine(centerPoint.x, centerPoint.y,
                centerPoint.x - axisLength, centerPoint.y);
    dc.DrawText(wxT("W"), centerPoint.x - axisLength - 20, centerPoint.y - 10);
    
    // Draw center point
    dc.SetPen(wxPen(*wxGREEN, 2));
    dc.DrawCircle(centerPoint, 3);
}

void RadarWindow::DrawPoint(wxDC &dc, const RadarPoint &point)
{
    // Validate distance
    if (point.distance < 0 || point.distance > maxDistance)
    {
        return;  // Out of range, don't draw
    }
    
    // Convert angle and distance to pixel coordinates
    // Angle 0° = North (up), increases clockwise
    // In screen coordinates: angle 0° = -90° in standard math
    float angleRad = (90.0f - point.angle) * M_PI / 180.0f;
    float pixelDistance = point.distance * pixelsPerNmi;
    
    int x = centerPoint.x + static_cast<int>(pixelDistance * std::cos(angleRad));
    int y = centerPoint.y - static_cast<int>(pixelDistance * std::sin(angleRad));
    
    // Draw point in red
    dc.SetPen(wxPen(*wxRED, 2));
    dc.SetBrush(*wxRED_BRUSH);
    dc.DrawCircle(wxPoint(x, y), 5);
}

void RadarWindow::OnClose(wxCloseEvent &event)
{
    if (listenThread)
    {
        listenThread->Delete();
        listenThread = nullptr;
    }
    Destroy();
}