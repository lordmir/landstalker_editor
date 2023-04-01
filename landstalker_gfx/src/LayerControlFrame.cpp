#include "LayerControlFrame.h"
#include <wx/statline.h>
#include <array>
#include "Utils.h"

static const std::vector<std::string> LABELS = { "Background 1", "Background 2", "Foreground", "Sprites", "Heightmap" };
static const std::array<double, 7> ZOOM_LEVELS = { 0.25, 0.5, 0.75, 1.0, 1.5, 2.0, 3.0 };

wxDEFINE_EVENT(EVT_ZOOM_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_OPACITY_CHANGE, wxCommandEvent);

LayerControlFrame::LayerControlFrame(wxWindow* parent)
    : wxWindow(parent, wxID_ANY, wxDefaultPosition, { 220, 200 }),
    m_visibilities{ true, true, true, true, false },
    m_opacities{ 0xFF, 0xFF, 0xFF, 0xFF, 0x80 },
    m_zoom(1.0)
{
    wxBoxSizer* vboxsizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* hboxsizer = new wxBoxSizer(wxHORIZONTAL);
    this->SetSizer(vboxsizer);
    vboxsizer->Add(hboxsizer, 0, wxALL | wxEXPAND);

    auto zoom_label = new wxStaticText(this, wxID_ANY, _("Zoom"));
    m_zoom_slider = new wxSlider(this, 3000, 3, 0, ZOOM_LEVELS.size() - 1, wxDefaultPosition);
    m_zoom_label = new wxStaticText(this, wxID_ANY, _("100%"));
    auto line = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL);
    m_zoom_slider->Bind(wxEVT_SLIDER, &LayerControlFrame::OnZoomValueChanged, this, 3000);

    hboxsizer->Add(zoom_label, 0, wxALL, 5);
    hboxsizer->Add(m_zoom_slider, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    hboxsizer->Add(m_zoom_label, 0, wxALL, 5);
    vboxsizer->Add(line, 0, wxALL | wxEXPAND, 5);

    wxFlexGridSizer* sizer = new wxFlexGridSizer(3, 0, 0);
    sizer->SetFlexibleDirection(wxHORIZONTAL);
    sizer->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);
    sizer->AddGrowableCol(0, 1);
    sizer->AddGrowableCol(3, 1);
    vboxsizer->Add(sizer, 0, wxALL | wxEXPAND);


    for (int i = 0; i < LABELS.size(); ++i)
    {
        auto label = new wxStaticText(this, wxID_ANY, LABELS[i]);
        m_visible_ctrls.push_back(new wxCheckBox(this, i + 1000, ""));
        m_opacity_ctrls.push_back(new wxSlider(this, i + 2000, 0xFF, 0, 0xFF, wxDefaultPosition, { 80, -1 }, wxSL_HORIZONTAL));

        m_visible_ctrls.back()->SetValue(true);
        m_visible_ctrls.back()->Bind(wxEVT_CHECKBOX, &LayerControlFrame::OnVisibilityChecked, this, 1000 + i);
        m_opacity_ctrls.back()->Bind(wxEVT_SLIDER, &LayerControlFrame::OnOpacityChanged, this, 2000 + i);

        sizer->Add(label, 1, wxALL | wxEXPAND | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 5);
        sizer->Add(m_visible_ctrls.back(), 1, wxALL | wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 5);
        sizer->Add(m_opacity_ctrls.back(), 1, wxALL | wxEXPAND | wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    }
    SetUI();
}

LayerControlFrame::~LayerControlFrame()
{
}

uint8_t LayerControlFrame::GetLayerOpacity(Layer layer)
{
    UpdateUI();
    if (m_visibilities[static_cast<uint8_t>(layer)])
    {
        return m_opacities[static_cast<uint8_t>(layer)];
    }
    else
    {
        return false;
    }
}

void LayerControlFrame::SetLayerOpacity(Layer layer, uint8_t opacity)
{
    m_opacities[static_cast<uint8_t>(layer)] = opacity;
    SetUI();
}

void LayerControlFrame::SetLayerVisible(Layer layer, bool visible)
{
    m_visibilities[static_cast<uint8_t>(layer)] = visible;
    SetUI();
}

double LayerControlFrame::GetZoom()
{
    UpdateUI();
    return m_zoom;
}

void LayerControlFrame::SetZoom(double zoom)
{
    m_zoom = zoom;
    SetUI();
}

void LayerControlFrame::EnableControls(bool enabled)
{
    EnableZoom(enabled);
    EnableLayers(enabled);
}

void LayerControlFrame::EnableZoom(bool enabled)
{
    m_zoom_slider->Enable(enabled);
}

void LayerControlFrame::EnableLayers(bool enabled)
{
    for (int i = 0; i < m_opacity_ctrls.size(); ++i)
    {
        m_visible_ctrls[i]->Enable(enabled);
        if (m_visible_ctrls[i]->IsChecked())
        {
            m_opacity_ctrls[i]->Enable(enabled);
        }
        else
        {
            m_opacity_ctrls[i]->Enable(false);
        }
    }
}

void LayerControlFrame::EnableLayer(Layer layer, bool enabled)
{
    m_visible_ctrls[static_cast<int>(layer)]->Enable(enabled);
    if (m_visible_ctrls[static_cast<int>(layer)]->IsChecked())
    {
        m_opacity_ctrls[static_cast<int>(layer)]->Enable(enabled);
    }
    else
    {
        m_opacity_ctrls[static_cast<int>(layer)]->Enable(false);
    }
}

void LayerControlFrame::UpdateUI()
{
    m_zoom = ZOOM_LEVELS[std::clamp<int>(m_zoom_slider->GetValue(), 0, ZOOM_LEVELS.size() - 1)];
    for (int i = 0; i < m_opacity_ctrls.size(); ++i)
    {
        if (m_visible_ctrls[i]->IsChecked())
        {
            m_opacities[i] = m_opacity_ctrls[i]->GetValue();
            m_visibilities[i] = true;
        }
        else
        {
            m_visibilities[i] = false;
        }
    }
}

void LayerControlFrame::SetUI()
{
    int i = 0;
    for (const auto& level : ZOOM_LEVELS)
    {
        if (m_zoom < level)
        {
            i++;
        }
    }
    m_zoom_slider->SetValue(std::clamp<int>(i, 0, ZOOM_LEVELS.size() - 1));
    m_zoom_label->SetLabelText(StrPrintf("%d%%", static_cast<int>(m_zoom * 100.0)));

    for (int i = 0; i < m_opacity_ctrls.size(); ++i)
    {
        m_opacity_ctrls[i]->SetValue(m_opacities[i]);
        m_opacity_ctrls[i]->Enable(m_visibilities[i]);
        m_visible_ctrls[i]->SetValue(m_visibilities[i]);
    }
    Refresh();
}

void LayerControlFrame::OnZoomValueChanged(wxCommandEvent& evt)
{
    double new_zoom = ZOOM_LEVELS[std::clamp<int>(m_zoom_slider->GetValue(), 0, ZOOM_LEVELS.size() - 1)];
    if (new_zoom != m_zoom)
    {
        m_zoom = new_zoom;
        m_zoom_label->SetLabelText(StrPrintf("%d%%", static_cast<int>(m_zoom * 100.0)));
        FireEvent(EVT_ZOOM_CHANGE);
    }
}

void LayerControlFrame::OnVisibilityChecked(wxCommandEvent& evt)
{
    int id = evt.GetId() - 1000;
    if ((id >= 0) && (id < m_visible_ctrls.size()))
    {
        bool new_visibility = m_visible_ctrls[id]->GetValue();
        if (m_visibilities[id] != new_visibility)
        {
            m_visibilities[id] = new_visibility;
            m_opacity_ctrls[id]->Enable(m_visibilities[id]);
            FireEvent(EVT_OPACITY_CHANGE, id);
        }
    }
}

void LayerControlFrame::OnOpacityChanged(wxCommandEvent& evt)
{
    int id = evt.GetId() - 2000;
    if ((id >= 0) && (id < m_opacity_ctrls.size()))
    {
        if (m_visible_ctrls[id]->GetValue() == true)
        {
            uint8_t new_opacity = m_opacity_ctrls[id]->GetValue();
            if (m_opacities[id] != new_opacity)
            {
                m_opacities[id] = new_opacity;
                FireEvent(EVT_OPACITY_CHANGE, id);
            }
        }
    }
}

void LayerControlFrame::FireEvent(const wxEventType& e, intptr_t userdata)
{
    wxCommandEvent evt(e);
    evt.SetClientData(reinterpret_cast<void*>(userdata));
    wxPostEvent(this->GetParent(), evt);
}
