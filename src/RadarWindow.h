#ifndef RADAR_WINDOW_H
#define RADAR_WINDOW_H

#include <wx/wx.h>
#include <wx/socket.h>
#include <queue>
#include <mutex>
#include <cmath>

struct RadarPoint
{
    float distance;  // in nautical miles
    float angle;     // in degrees (0 = north, clockwise)
};

class RadarWindow : public wxFrame
{
public:
    RadarWindow(const wxString& title);
    ~RadarWindow();

private:
    wxPanel *drawPanel;
    wxDatagramSocket *socket;
    wxThread *listenThread;
    
    std::queue<RadarPoint> pointQueue;
    std::mutex queueMutex;
    
    float maxDistance;      // Maximum distance to display (nmi)
    float pixelsPerNmi;     // Scaling factor
    wxPoint centerPoint;    // Center of radar display
    
    void OnPaint(wxPaintEvent &event);
    void OnSize(wxSizeEvent &event);
    void OnClose(wxCloseEvent &event);
    void StartListening();
    void ProcessDatagramQueue();
    void DrawRadar(wxDC &dc);
    void DrawPoint(wxDC &dc, const RadarPoint &point);
    
    wxDECLARE_EVENT_TABLE();
};

#endif