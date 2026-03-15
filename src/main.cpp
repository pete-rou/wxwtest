#include <wx/wx.h>
#include "RadarWindow.h"

class RadarApp : public wxApp
{
public:
    bool OnInit() override;
};

bool RadarApp::OnInit()
{
    RadarWindow *frame = new RadarWindow(wxT("UDP Radar Plot - wxWidgets POC"));
    frame->Show(true);
    return true;
}

wxIMPLEMENT_APP(RadarApp);