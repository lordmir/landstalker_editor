#ifndef _SPRITE_MANAGER_DIALOG_H_
#define _SPRITE_MANAGER_DIALOG_H_

#include <memory>
#include <string>
#include <vector>
#include <wx/wx.h>
#include <wx/statbmp.h>

#include <landstalker/main/GameData.h>

// Prompts for a sprite's two names: the internal assembly label and the editor's display name.
// Reused when renaming a sprite and when importing one that needs a fresh name.
class SpriteNameDialog : public wxDialog
{
public:
	SpriteNameDialog(wxWindow* parent, const std::string& internal_name,
		const std::wstring& display_name, const wxString& title = "Rename Sprite")
		: wxDialog(parent, wxID_ANY, title)
	{
		auto* outer = new wxBoxSizer(wxVERTICAL);
		outer->Add(new wxStaticText(this, wxID_ANY,
			"The internal name is the assembly label: a unique identifier of at most 30\n"
			"characters, starting with a letter. The display name is used only in the editor."),
			0, wxLEFT | wxRIGHT | wxTOP, 10);

		auto* fields = new wxFlexGridSizer(2, 6, 6);
		fields->AddGrowableCol(1, 1);
		fields->Add(new wxStaticText(this, wxID_ANY, "Internal name"), 0, wxALIGN_CENTER_VERTICAL);
		m_internal = new wxTextCtrl(this, wxID_ANY, wxString::FromUTF8(internal_name));
		m_internal->SetMaxLength(30);
		fields->Add(m_internal, 1, wxEXPAND);
		fields->Add(new wxStaticText(this, wxID_ANY, "Display name"), 0, wxALIGN_CENTER_VERTICAL);
		m_display = new wxTextCtrl(this, wxID_ANY, wxString(display_name));
		fields->Add(m_display, 1, wxEXPAND);

		outer->Add(fields, 1, wxALL | wxEXPAND, 10);
		outer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxALL | wxEXPAND, 10);
		SetSizerAndFit(outer);
		SetMinSize(wxSize(440, -1));
		CentreOnParent();
	}

	std::string GetInternalName() const { return m_internal->GetValue().ToStdString(); }
	std::wstring GetDisplayName() const { return m_display->GetValue().ToStdWstring(); }

private:
	wxTextCtrl* m_internal;
	wxTextCtrl* m_display;
};

// Manages the game's sprites: add, remove, reorder, rename, import and export.
//
// A sprite is keyed by a dense graphics id that entities reference directly, so the list is
// shown in id order and reordering renumbers those references. Frame- and animation-level
// editing stays in the sprite editor; this dialog works on whole sprites.
//
// SpriteData applies each operation immediately and offers no rollback, so the dialog closes
// with a single Close button rather than pretending to support Cancel.
class SpriteManagerDialog : public wxDialog
{
public:
	// Optional operation to run as soon as the dialog opens, for the browser's quick
	// add/delete buttons. On success the dialog closes itself (reporting the added sprite
	// as the one to open); on failure or cancellation it stays open so the user can see
	// why and continue by hand.
	enum class InitialAction
	{
		NONE,
		ADD,
		REMOVE
	};

	SpriteManagerDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd, int select_id,
		InitialAction initial_action = InitialAction::NONE);
	virtual ~SpriteManagerDialog();

	// True if any operation modified the game data, so the caller knows to refresh.
	bool HasChanges() const { return m_changed; }
	// Sprite id the user double-clicked to open, or -1 if the dialog was simply closed.
	int GetSpriteToOpen() const { return m_to_open; }

private:
	void RunInitialAction(InitialAction action);
	void PopulateList(int select_id);
	void PopulateDetails();
	void PopulatePreview();
	void UpdateUI();
	// Sprite id behind the current list selection, or -1 if nothing is selected.
	int GetSelectedSprite() const;
	std::size_t GetSpriteCount() const;
	// Palette a sprite is best previewed with: an entity's if one uses it, else a default.
	std::shared_ptr<Landstalker::Palette> PreviewPalette(uint8_t id) const;
	// Names the entities blocking a delete, for the refusal message.
	wxString DescribeEntities(const std::vector<uint8_t>& entities) const;
	void Move(int delta);

	void OnSelected(wxCommandEvent& evt);
	void OnActivated(wxCommandEvent& evt);
	void OnAdd(wxCommandEvent& evt);
	void OnImport(wxCommandEvent& evt);
	void OnExport(wxCommandEvent& evt);
	void OnRemove(wxCommandEvent& evt);
	void OnMoveUp(wxCommandEvent& evt);
	void OnMoveDown(wxCommandEvent& evt);
	void OnRename(wxCommandEvent& evt);

	std::shared_ptr<Landstalker::GameData> m_gd;
	bool m_changed;
	int m_to_open;
	// Sprite the preview currently shows, so re-selecting it costs nothing.
	int m_previewed;

	wxListBox* m_list;
	wxStaticBitmap* m_preview;
	wxStaticText* m_preview_message;
	static constexpr std::size_t DETAIL_ROWS = 5;
	wxStaticText* m_detail_captions[DETAIL_ROWS];
	wxStaticText* m_detail_values[DETAIL_ROWS];
	void SetDetail(std::size_t row, const wxString& caption, const wxString& value);
	wxButton* m_add;
	wxButton* m_import;
	wxButton* m_export;
	wxButton* m_remove;
	wxButton* m_move_up;
	wxButton* m_move_down;
	wxButton* m_rename;
	wxButton* m_close;
};

#endif // _SPRITE_MANAGER_DIALOG_H_
