#include <sprites/SpriteManagerDialog.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <wx/filedlg.h>

#include <landstalker/misc/Labels.h>
#include <landstalker/misc/Utils.h>
#include <main/ImageBufferWx.h>

namespace
{

const wxSize LIST_MIN_SIZE(240, 320);
const wxSize PREVIEW_SIZE(240, 240);

// First unused name of the form "<prefix><nn>", two digits to match the stock naming.
std::string SuggestName(const Landstalker::SpriteData& sprites, const std::string& prefix)
{
	for (unsigned int i = 1; ; ++i)
	{
		const auto candidate = Landstalker::StrPrintf("%s%02u", prefix.c_str(), i);
		if (!sprites.IsSpriteNameInUse(candidate))
		{
			return candidate;
		}
	}
}

std::string ReadTextFile(const std::string& path)
{
	std::ifstream file(path, std::ios::binary);
	std::ostringstream contents;
	contents << file.rdbuf();
	return contents.str();
}

// Prompts for a sprite name, re-prompting until it is usable or the user cancels.
bool PromptForName(wxWindow* parent, const wxString& title, const Landstalker::SpriteData& sprites,
	const std::string& initial, std::string& name)
{
	wxTextEntryDialog dlg(parent, "Sprite name (assembly label: starts with a letter, then\n"
		"letters, digits and underscores, at most 30 characters):", title, wxString::FromUTF8(initial));
	while (dlg.ShowModal() == wxID_OK)
	{
		const auto candidate = dlg.GetValue().ToStdString();
		if (!Landstalker::SpriteData::IsValidSpriteName(candidate) || sprites.IsSpriteNameInUse(candidate))
		{
			wxMessageBox("The name must be unique, start with a letter, only contain A-Z, a-z, "
				"0-9 and _, and be at most 30 characters.", title, wxOK | wxICON_ERROR, parent);
			continue;
		}
		name = candidate;
		return true;
	}
	return false;
}

}

SpriteManagerDialog::SpriteManagerDialog(wxWindow* parent,
	std::shared_ptr<Landstalker::GameData> gd, int select_id, InitialAction initial_action)
	: wxDialog(parent, wxID_ANY, "Sprites", wxDefaultPosition, { 640, 460 }),
	  m_gd(gd),
	  m_changed(false),
	  m_to_open(-1),
	  m_previewed(-1),
	  m_list(nullptr),
	  m_preview(nullptr),
	  m_preview_message(nullptr),
	  m_add(nullptr),
	  m_import(nullptr),
	  m_export(nullptr),
	  m_remove(nullptr),
	  m_move_up(nullptr),
	  m_move_down(nullptr),
	  m_rename(nullptr),
	  m_close(nullptr)
{
	auto* outer = new wxBoxSizer(wxVERTICAL);
	SetSizer(outer);

	auto* panes = new wxBoxSizer(wxHORIZONTAL);
	outer->Add(panes, 1, wxEXPAND, 5);

	auto* list_box = new wxStaticBoxSizer(wxVERTICAL, this, "Sprites");
	m_list = new wxListBox(list_box->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
		0, nullptr, wxLB_SINGLE);
	m_list->SetMinSize(LIST_MIN_SIZE);
	m_list->SetToolTip("Sprites are listed in graphics-id order. Double-click one to close this "
		"dialog and open it in the sprite editor.");
	list_box->Add(m_list, 1, wxALL | wxEXPAND, 5);
	panes->Add(list_box, 1, wxALL | wxEXPAND, 5);

	auto* right = new wxBoxSizer(wxVERTICAL);
	panes->Add(right, 1, wxEXPAND, 0);

	auto* detail_box = new wxStaticBoxSizer(wxVERTICAL, this, "Selected Sprite");
	auto* details = new wxFlexGridSizer(2, 4, 12);
	details->AddGrowableCol(1, 1);
	for (std::size_t row = 0; row < DETAIL_ROWS; ++row)
	{
		m_detail_captions[row] = new wxStaticText(detail_box->GetStaticBox(), wxID_ANY, wxEmptyString);
		details->Add(m_detail_captions[row], 0, wxALIGN_CENTER_VERTICAL);
		auto* value = new wxStaticText(detail_box->GetStaticBox(), wxID_ANY, wxEmptyString,
			wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
		value->SetMinSize(wxSize(160, -1));
		m_detail_values[row] = value;
		details->Add(value, 1, wxEXPAND);
	}
	detail_box->Add(details, 0, wxALL | wxEXPAND, 5);
	right->Add(detail_box, 0, wxALL | wxEXPAND, 5);

	auto* preview_box = new wxStaticBoxSizer(wxVERTICAL, this, "Preview");
	m_preview = new wxStaticBitmap(preview_box->GetStaticBox(), wxID_ANY, wxNullBitmap);
	m_preview->SetMinSize(PREVIEW_SIZE);
	m_preview_message = new wxStaticText(preview_box->GetStaticBox(), wxID_ANY, wxEmptyString,
		wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
	preview_box->Add(m_preview, 1, wxALL | wxALIGN_CENTER, 5);
	preview_box->Add(m_preview_message, 0, wxALL | wxALIGN_CENTER, 5);
	right->Add(preview_box, 1, wxALL | wxEXPAND, 5);

	auto* buttons = new wxBoxSizer(wxHORIZONTAL);
	outer->Add(buttons, 0, wxALL | wxEXPAND, 5);

	m_add = new wxButton(this, wxID_ANY, "Add...");
	m_import = new wxButton(this, wxID_ANY, "Import...");
	m_export = new wxButton(this, wxID_ANY, "Export...");
	m_remove = new wxButton(this, wxID_ANY, "Remove");
	m_move_up = new wxButton(this, wxID_ANY, "Move Up");
	m_move_down = new wxButton(this, wxID_ANY, "Move Down");
	m_rename = new wxButton(this, wxID_ANY, "Rename...");
	m_close = new wxButton(this, wxID_CANCEL, "Close");
	for (auto* button : { m_add, m_import, m_export, m_remove, m_move_up, m_move_down, m_rename })
	{
		buttons->Add(button, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5);
	}
	buttons->AddStretchSpacer();
	buttons->Add(m_close, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5);
	m_close->SetDefault();

	GetSizer()->Fit(this);
	SetMinSize(GetSize());
	CentreOnParent(wxBOTH);

	PopulateList(select_id);

	m_list->Connect(wxEVT_LISTBOX, wxCommandEventHandler(SpriteManagerDialog::OnSelected), nullptr, this);
	m_list->Connect(wxEVT_LISTBOX_DCLICK, wxCommandEventHandler(SpriteManagerDialog::OnActivated), nullptr, this);
	m_add->Connect(wxEVT_BUTTON, wxCommandEventHandler(SpriteManagerDialog::OnAdd), nullptr, this);
	m_import->Connect(wxEVT_BUTTON, wxCommandEventHandler(SpriteManagerDialog::OnImport), nullptr, this);
	m_export->Connect(wxEVT_BUTTON, wxCommandEventHandler(SpriteManagerDialog::OnExport), nullptr, this);
	m_remove->Connect(wxEVT_BUTTON, wxCommandEventHandler(SpriteManagerDialog::OnRemove), nullptr, this);
	m_move_up->Connect(wxEVT_BUTTON, wxCommandEventHandler(SpriteManagerDialog::OnMoveUp), nullptr, this);
	m_move_down->Connect(wxEVT_BUTTON, wxCommandEventHandler(SpriteManagerDialog::OnMoveDown), nullptr, this);
	m_rename->Connect(wxEVT_BUTTON, wxCommandEventHandler(SpriteManagerDialog::OnRename), nullptr, this);

	if (initial_action != InitialAction::NONE)
	{
		// Deferred so the operation's own dialogs open over a fully shown manager.
		CallAfter([this, initial_action]() { RunInitialAction(initial_action); });
	}
}

void SpriteManagerDialog::RunInitialAction(InitialAction action)
{
	wxCommandEvent dummy;
	if (action == InitialAction::ADD)
	{
		OnAdd(dummy);
		if (m_changed)
		{
			// Close and hand back the new sprite, as if it had been double-clicked.
			m_to_open = GetSelectedSprite();
			EndModal(wxID_OK);
		}
	}
	else if (action == InitialAction::REMOVE)
	{
		OnRemove(dummy);
		if (m_changed)
		{
			EndModal(wxID_OK);
		}
	}
}

SpriteManagerDialog::~SpriteManagerDialog()
{
	m_list->Disconnect(wxEVT_LISTBOX, wxCommandEventHandler(SpriteManagerDialog::OnSelected), nullptr, this);
	m_list->Disconnect(wxEVT_LISTBOX_DCLICK, wxCommandEventHandler(SpriteManagerDialog::OnActivated), nullptr, this);
	m_add->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(SpriteManagerDialog::OnAdd), nullptr, this);
	m_import->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(SpriteManagerDialog::OnImport), nullptr, this);
	m_export->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(SpriteManagerDialog::OnExport), nullptr, this);
	m_remove->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(SpriteManagerDialog::OnRemove), nullptr, this);
	m_move_up->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(SpriteManagerDialog::OnMoveUp), nullptr, this);
	m_move_down->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(SpriteManagerDialog::OnMoveDown), nullptr, this);
	m_rename->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(SpriteManagerDialog::OnRename), nullptr, this);
}

std::size_t SpriteManagerDialog::GetSpriteCount() const
{
	const auto sprite_data = m_gd->GetSpriteData();
	std::size_t count = 0;
	while (count < Landstalker::SpriteData::MAX_SPRITES &&
		sprite_data->IsSprite(static_cast<uint8_t>(count)))
	{
		++count;
	}
	return count;
}

void SpriteManagerDialog::PopulateList(int select_id)
{
	const auto sprite_data = m_gd->GetSpriteData();
	const auto count = GetSpriteCount();
	m_list->Freeze();
	m_list->Clear();
	for (std::size_t i = 0; i < count; ++i)
	{
		const auto id = static_cast<uint8_t>(i);
		const auto internal = sprite_data->GetSpriteName(id);
		const auto display = sprite_data->GetSpriteDisplayName(id);
		// The display name falls back to the internal label, so only append it when it says
		// something different.
		auto label = wxString::Format("%03d: ", static_cast<int>(id)) + wxString::FromUTF8(internal);
		if (display != std::wstring(internal.cbegin(), internal.cend()))
		{
			label += " (" + wxString(display) + ")";
		}
		m_list->Append(label);
	}
	m_list->Thaw();

	if (count > 0)
	{
		int row = (select_id >= 0 && select_id < static_cast<int>(count)) ? select_id : 0;
		m_list->SetSelection(row);
	}
	// Adding, deleting or reordering can leave a different sprite at the id the preview last
	// showed, so force a redraw rather than trusting the cached id to still mean the same
	// sprite.
	m_previewed = -1;
	PopulateDetails();
	PopulatePreview();
	UpdateUI();
}

int SpriteManagerDialog::GetSelectedSprite() const
{
	const int row = m_list->GetSelection();
	if (row == wxNOT_FOUND || row >= static_cast<int>(GetSpriteCount()))
	{
		return -1;
	}
	// The list is built in id order, so the row is the id.
	return row;
}

std::shared_ptr<Landstalker::Palette> SpriteManagerDialog::PreviewPalette(uint8_t id) const
{
	return m_gd->GetSpriteData()->GetSpriteDisplayPalette(id);
}

void SpriteManagerDialog::PopulatePreview()
{
	const int id = GetSelectedSprite();
	if (id == m_previewed)
	{
		return;
	}
	m_previewed = id;

	const auto show_message = [this](const wxString& message)
	{
		m_preview->SetBitmap(wxNullBitmap);
		m_preview->Hide();
		m_preview_message->SetLabel(message);
		m_preview_message->Show();
		Layout();
	};

	if (id < 0)
	{
		show_message("Nothing selected.");
		return;
	}

	const auto sprite_data = m_gd->GetSpriteData();
	// Animation 0 faces away from the camera; when a sprite has a second animation that one
	// faces the viewer - the same choice GetDefaultEntityAnimationId makes for entities. Show
	// the front so the preview reads the way the sprite does in game.
	const auto sid = static_cast<uint8_t>(id);
	const uint8_t anim = sprite_data->GetSpriteAnimationCount(sid) > 1 ? 1 : 0;
	const auto frame = sprite_data->GetSpriteFrame(sid, anim, 0);
	const auto palette = PreviewPalette(sid);
	if (!frame || !frame->GetData() || !palette)
	{
		show_message("Preview unavailable.");
		return;
	}

	const auto sf = frame->GetData();
	const auto box = sf->GetBoundingBox();
	const int width = box.GetWidth();
	const int height = box.GetHeight();
	if (width <= 0 || height <= 0)
	{
		show_message("This sprite's frame is empty.");
		return;
	}

	ImageBufferWx buffer(width, height);
	// The bounding box can start left of and above the origin, so shift the frame back into
	// the buffer by its top-left.
	buffer.InsertSprite(-box.GetLeft(), -box.GetTop(), 0, *sf);
	wxImage image = buffer.MakeImage({ palette });

	// Sprites are small, so only ever scale up to a whole multiple that fits, keeping the
	// pixels crisp rather than blurring them.
	const int scale = std::max(1, std::min(PREVIEW_SIZE.GetWidth() / width,
		PREVIEW_SIZE.GetHeight() / height));
	if (scale > 1)
	{
		image.Rescale(width * scale, height * scale, wxIMAGE_QUALITY_NORMAL);
	}

	wxImage canvas(PREVIEW_SIZE.GetWidth(), PREVIEW_SIZE.GetHeight());
	canvas.SetRGB(wxRect(0, 0, canvas.GetWidth(), canvas.GetHeight()), 0, 0, 0);
	canvas.Paste(image, (canvas.GetWidth() - image.GetWidth()) / 2,
		(canvas.GetHeight() - image.GetHeight()) / 2);

	m_preview_message->Hide();
	m_preview->SetBitmap(wxBitmap(canvas));
	m_preview->Show();
	Layout();
}

void SpriteManagerDialog::SetDetail(std::size_t row, const wxString& caption, const wxString& value)
{
	if (row >= DETAIL_ROWS)
	{
		return;
	}
	m_detail_captions[row]->SetLabel(caption);
	m_detail_values[row]->SetLabel(value);
}

wxString SpriteManagerDialog::DescribeEntities(const std::vector<uint8_t>& entities) const
{
	wxString described;
	std::size_t named = 0;
	for (; named < entities.size() && named < 3; ++named)
	{
		if (!described.IsEmpty())
		{
			described += ", ";
		}
		described += wxString(Landstalker::SpriteData::GetEntityDisplayName(entities[named]));
	}
	if (entities.size() > named)
	{
		described += wxString::Format(" and %d more", static_cast<int>(entities.size() - named));
	}
	return described;
}

void SpriteManagerDialog::PopulateDetails()
{
	const int id = GetSelectedSprite();
	const auto sprite_data = m_gd->GetSpriteData();
	for (std::size_t row = 0; row < DETAIL_ROWS; ++row)
	{
		SetDetail(row, wxEmptyString, wxEmptyString);
	}
	if (id < 0)
	{
		Layout();
		return;
	}

	const auto sid = static_cast<uint8_t>(id);
	SetDetail(0, "Graphics id", wxString::Format("%d of %d", id, static_cast<int>(GetSpriteCount()) - 1));
	SetDetail(1, "Animations", wxString::Format("%u", sprite_data->GetSpriteAnimationCount(sid)));
	SetDetail(2, "Frames", wxString::Format("%d", static_cast<int>(sprite_data->GetSpriteFrames(sid).size())));
	SetDetail(3, "Max Tile Count", wxString::Format("%u", sprite_data->GetSpriteMaxTileCount(sid)));

	const auto entities = sprite_data->GetEntitiesFromSprite(sid);
	SetDetail(4, "Used by entities", entities.empty()
		? wxString("None")
		: wxString::Format("%d (%s)", static_cast<int>(entities.size()), DescribeEntities(entities)));
	Layout();
}

void SpriteManagerDialog::UpdateUI()
{
	const int id = GetSelectedSprite();
	const int count = static_cast<int>(GetSpriteCount());
	const auto sprite_data = m_gd->GetSpriteData();
	const bool valid = id >= 0;

	m_add->Enable(count < static_cast<int>(Landstalker::SpriteData::MAX_SPRITES));
	m_import->Enable(count < static_cast<int>(Landstalker::SpriteData::MAX_SPRITES));
	m_export->Enable(valid);
	m_rename->Enable(valid);
	// A sprite can only go once nothing points at it, and the last one can never go.
	m_remove->Enable(valid && count > 1 &&
		!sprite_data->IsSpriteUsedByEntities(static_cast<uint8_t>(id)));
	m_move_up->Enable(valid && id > 0);
	m_move_down->Enable(valid && id < count - 1);
}

void SpriteManagerDialog::Move(int delta)
{
	const int id = GetSelectedSprite();
	if (id < 0)
	{
		return;
	}
	const int new_id = id + delta;
	if (new_id < 0 || new_id >= static_cast<int>(GetSpriteCount()))
	{
		return;
	}
	// Swap the two sprites' content rather than renumbering references: an entity keeps the
	// sprite id it points at, so the two sprites change places in the entities that draw them.
	if (m_gd->GetSpriteData()->SwapSprites(static_cast<uint8_t>(id), static_cast<uint8_t>(new_id)))
	{
		m_changed = true;
		PopulateList(new_id);
	}
}

void SpriteManagerDialog::OnSelected(wxCommandEvent& /*evt*/)
{
	PopulateDetails();
	PopulatePreview();
	UpdateUI();
}

void SpriteManagerDialog::OnActivated(wxCommandEvent& /*evt*/)
{
	const int id = GetSelectedSprite();
	if (id < 0)
	{
		return;
	}
	m_to_open = id;
	EndModal(wxID_OK);
}

void SpriteManagerDialog::OnAdd(wxCommandEvent& /*evt*/)
{
	const auto sprite_data = m_gd->GetSpriteData();
	std::string name;
	if (!PromptForName(this, "New Sprite", *sprite_data, SuggestName(*sprite_data, "Sprite"), name))
	{
		return;
	}
	const auto added = sprite_data->AddSprite(name);
	if (!added)
	{
		wxMessageBox("Unable to create the sprite.", "New Sprite", wxOK | wxICON_ERROR, this);
		return;
	}
	m_changed = true;
	PopulateList(*added);
}

void SpriteManagerDialog::OnRename(wxCommandEvent& /*evt*/)
{
	const int id = GetSelectedSprite();
	if (id < 0)
	{
		return;
	}
	const auto sprite_data = m_gd->GetSpriteData();
	const auto sid = static_cast<uint8_t>(id);
	const auto old_name = sprite_data->GetSpriteName(sid);
	const auto old_display = sprite_data->GetSpriteDisplayName(sid);

	SpriteNameDialog dialog(this, old_name, old_display);
	while (dialog.ShowModal() == wxID_OK)
	{
		const auto name = dialog.GetInternalName();
		const auto display = dialog.GetDisplayName();
		const bool name_changed = name != old_name;
		const bool display_changed = display != old_display;
		if (!name_changed && !display_changed)
		{
			return;
		}

		// Check both names before changing either, so a bad display name cannot leave the
		// internal rename half-applied.
		if (name_changed &&
			(!Landstalker::SpriteData::IsValidSpriteName(name) || sprite_data->IsSpriteNameInUse(name)))
		{
			wxMessageBox("The internal name must be unique, start with a letter, only contain "
				"A-Z, a-z, 0-9 and _, and be at most 30 characters.",
				"Rename Sprite", wxOK | wxICON_ERROR, this);
			continue;
		}
		if (display_changed && !Landstalker::Labels::IsValid(display, Landstalker::Labels::C_SPRITES, id))
		{
			wxMessageBox("The display name must not be empty, must be unique, and must not contain "
				"non-printable characters.", "Rename Sprite", wxOK | wxICON_ERROR, this);
			continue;
		}

		if (name_changed && !sprite_data->RenameSprite(sid, name))
		{
			wxMessageBox("Unable to rename the selected sprite.", "Rename Sprite", wxOK | wxICON_ERROR, this);
			return;
		}
		if (display_changed)
		{
			Landstalker::Labels::Update(Landstalker::Labels::C_SPRITES, id, display);
		}
		m_changed = true;
		PopulateList(id);
		return;
	}
}

void SpriteManagerDialog::OnRemove(wxCommandEvent& /*evt*/)
{
	const int id = GetSelectedSprite();
	if (id < 0)
	{
		return;
	}
	const auto sprite_data = m_gd->GetSpriteData();
	const auto sid = static_cast<uint8_t>(id);
	const auto name = wxString::FromUTF8(sprite_data->GetSpriteName(sid));

	const auto entities = sprite_data->GetEntitiesFromSprite(sid);
	if (!entities.empty())
	{
		wxMessageBox(wxString::Format(
			"'%s' cannot be deleted: %d entit%s still use%s it.\n\n  %s\n\n"
			"Point those entities at another sprite first.",
			name, static_cast<int>(entities.size()), entities.size() == 1 ? "y" : "ies",
			entities.size() == 1 ? "s" : "", DescribeEntities(entities)),
			"Remove Sprite", wxOK | wxICON_ERROR, this);
		return;
	}
	if (wxMessageBox(wxString::Format(
		"Delete sprite '%s'?\n\n"
		"This cannot be undone. Its animations and frames go with it, every sprite above it "
		"moves down a graphics id, and the entities that name those ids are renumbered to "
		"follow.", name),
		"Remove Sprite", wxYES_NO | wxNO_DEFAULT | wxICON_WARNING, this) != wxYES)
	{
		return;
	}

	// Land on the next sprite, or the previous one when the last was removed.
	const int next = id + 1 < static_cast<int>(GetSpriteCount()) ? id : id - 1;
	if (!sprite_data->DeleteSprite(sid))
	{
		wxMessageBox("Unable to delete the selected sprite.", "Remove Sprite", wxOK | wxICON_ERROR, this);
		return;
	}
	m_changed = true;
	PopulateList(next);
}

void SpriteManagerDialog::OnMoveUp(wxCommandEvent& /*evt*/)
{
	Move(-1);
}

void SpriteManagerDialog::OnMoveDown(wxCommandEvent& /*evt*/)
{
	Move(1);
}

void SpriteManagerDialog::OnExport(wxCommandEvent& /*evt*/)
{
	const int id = GetSelectedSprite();
	if (id < 0)
	{
		return;
	}
	const auto sprite_data = m_gd->GetSpriteData();
	const auto sid = static_cast<uint8_t>(id);
	const auto stem = sprite_data->GetSpriteName(sid);

	wxFileDialog fd(this, _("Export Sprite"), "", wxString::FromUTF8(stem + ".yaml"),
		"Sprite metadata (*.yaml)|*.yaml", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}
	const std::filesystem::path chosen(fd.GetPath().ToStdString());
	const auto dir = chosen.parent_path();

	// The YAML is written under the chosen name; the frames sit beside it using the stem the
	// import expects: <stem>_frmNN.frm, in the order the metadata lists them.
	std::ofstream yaml(chosen.string(), std::ios::binary);
	yaml << sprite_data->GetSpriteMetadataYaml(sid);
	if (!yaml)
	{
		wxMessageBox("Unable to write the sprite metadata.", "Export Sprite", wxOK | wxICON_ERROR, this);
		return;
	}
	yaml.close();

	unsigned int index = 0;
	bool frames_ok = true;
	for (const auto& frame_name : sprite_data->GetSpriteFrames(sid))
	{
		const auto frame = sprite_data->GetSpriteFrame(frame_name);
		if (!frame || !frame->GetData())
		{
			frames_ok = false;
			continue;
		}
		const auto path = dir / Landstalker::StrPrintf("%s_frm%02u.frm", stem.c_str(), index++);
		Landstalker::WriteBytes(frame->GetData()->GetBits(), path.string());
	}
	if (!frames_ok)
	{
		wxMessageBox("The metadata was written, but one or more frames could not be exported.",
			"Export Sprite", wxOK | wxICON_WARNING, this);
	}
}

void SpriteManagerDialog::OnImport(wxCommandEvent& /*evt*/)
{
	const auto sprite_data = m_gd->GetSpriteData();
	wxFileDialog fd(this, _("Import Sprite"), "", "",
		"Sprite metadata (*.yaml)|*.yaml|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}
	const std::filesystem::path chosen(fd.GetPath().ToStdString());
	const auto yaml_data = ReadTextFile(chosen.string());
	if (yaml_data.empty())
	{
		wxMessageBox("Unable to read the selected file.", "Import Sprite", wxOK | wxICON_ERROR, this);
		return;
	}

	// Offer the file's own stem as the new name if it is usable, else a fresh suggestion.
	const auto file_stem = chosen.stem().string();
	const auto suggested = Landstalker::SpriteData::IsValidSpriteName(file_stem) &&
		!sprite_data->IsSpriteNameInUse(file_stem) ? file_stem : SuggestName(*sprite_data, "Sprite");
	std::string name;
	if (!PromptForName(this, "Import Sprite", *sprite_data, suggested, name))
	{
		return;
	}

	const auto imported = sprite_data->ImportSprite(name, yaml_data, chosen.parent_path());
	if (!imported)
	{
		wxMessageBox("Unable to import the sprite. Check that the frame files (named "
			"<sprite>_frmNN.frm) sit beside the metadata file.",
			"Import Sprite", wxOK | wxICON_ERROR, this);
		return;
	}
	m_changed = true;
	PopulateList(*imported);
}
