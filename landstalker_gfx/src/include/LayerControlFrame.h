#ifndef _LAYER_CONTROL_FRAME_
#define _LAYER_CONTROL_FRAME_

#include <wx/wx.h>
#include <array>

class LayerControlFrame : public wxWindow
{
public:
	enum Layer
	{
		BG1,
		BG2,
		FG,
		SPRITES,
		HM
	};

	LayerControlFrame(wxWindow* parent);
	virtual ~LayerControlFrame();

	uint8_t GetLayerOpacity(Layer layer);
	void SetLayerOpacity(Layer layer, uint8_t opacity);
	void SetLayerVisible(Layer layer, bool visible);
	double GetZoom();
	void SetZoom(double zoom);

	void EnableControls(bool enabled);
	void EnableZoom(bool enabled);
	void EnableLayers(bool enabled);
	void EnableLayer(Layer layer, bool enabled);

private:
	void UpdateUI();
	void SetUI();
	void OnZoomValueChanged(wxCommandEvent& evt);
	void OnVisibilityChecked(wxCommandEvent& evt);
	void OnOpacityChanged(wxCommandEvent& evt);
	void FireEvent(const wxEventType& e, intptr_t userdata = 0);

	std::vector<wxCheckBox*> m_visible_ctrls;
	std::vector<wxSlider*> m_opacity_ctrls;
	wxStaticText* m_zoom_label;
	wxSlider* m_zoom_slider;

	double m_zoom;
	std::array<uint8_t, 5> m_opacities;
	std::array<bool, 5> m_visibilities;
	bool m_zoom_enabled;
	std::array<bool, 5> m_layers_enabled;
};

wxDECLARE_EVENT(EVT_ZOOM_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_OPACITY_CHANGE, wxCommandEvent);

#endif // _LAYER_CONTROL_FRAME_