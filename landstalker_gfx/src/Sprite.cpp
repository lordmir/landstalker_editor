#include <set>
#include <map>

#include "SpriteFrame.h"
#include "Sprite.h"

static inline const std::unordered_map<uint8_t, std::string> SPRITE_NAME_LOOKUP =
{
	{  0, "Nigel"},                           {  1, "Old Man"},                    {  2, "Massan Male"},                      {  3, "Boy"},
	{  4, "Orc 1"},                           {  5, "Orc 2"},                      {  6, "Orc 3"},                            {  7, "Worm 1"},
	{  8, "Worm 2"},                          {  9, "Unused Worm 3"},              { 10, "Moneybag"},                         { 11, "Man 1"},
	{ 12, "Woman 1"},                         { 13, "Girl"},                       { 14, "Dog"},                              { 15, "Chicken"},
	{ 16, "Priest"},                          { 17, "Pot"},                        { 18, "Chest"},                            { 19, "Small Grey Boulder"},
	{ 20, "Large Grey Boulder"},              { 21, "Small Yellow Platform"},      { 22, "Crate"},                            { 23, "Ninja 1"},
	{ 24, "Ninja 2"},                         { 25, "Ninja 3"},                    { 26, "Lizardman 1"},                      { 27, "Lizardman 2"},
	{ 28, "Lizardman 3"},                     { 29, "Knight 1"},                   { 30, "Knight 2"},                         { 31, "Knight 3"},
	{ 32, "Ghost 1"},                         { 33, "Ghost 2"},                    { 34, "Ghost 3"},                          { 35, "Mummy 1"},
	{ 36, "Mummy 2"},                         { 37, "Mummy 3"},                    { 38, "Unicorn 1"},                        { 39, "Unicorn 2"},
    { 40, "Unicorn 3"},                       { 41, "Skeleton 1"},                 { 42, "Skeleton 2"},                       { 43, "Skeleton 3"},
	{ 44, "Mimic 1"},                         { 45, "Mimic 2"},                    { 46, "Mimic 3"},                          { 47, "Massan Child"},
	{ 48, "Male Dwarf"},                      { 49, "Gumi Female"},                { 50, "Soldier 1"},                        { 51, "Madame Yard"},
	{ 52, "Dexter"},                          { 53, "Raft"},                       { 54, "Mushroom 1"},                       { 55, "Mushroom 2"},
	{ 56, "Mushroom 3"},                      { 57, "Giant 1"},                    { 58, "Giant 2"},                          { 59, "Giant 3"},
	{ 60, "Bubble 1"},                        { 61, "Bubble 2"},                   { 62, "Bubble 3"},                         { 63, "Old Lady"},
	{ 64, "Gumi Male"},                       { 65, "Gumi Child"},                 { 66, "Fara Stake"},                       { 67, "Praying Gumi Native"},
	{ 68, "Skeleton Statue 1"},               { 69, "Praying Massan Native"},      { 70, "Skeleton Statue 2"},                { 71, "Massan Female"},
	{ 72, "Female Dwarf"},                    { 73, "Small Grey Spiked Ball"},     { 74, "Large Grey Spiked Ball"},           { 75, "Sacred Tree"},
	{ 76, "Large Wooden Platform"},           { 77, "Injured Dog"},                { 78, "Corpse"},                           { 79, "Woman 2"},
	{ 80, "Man 2"},                           { 81, "Invisible Cube"},             { 82, "Ink"},                              { 83, "Large Grey Block"},
	{ 84, "Square Switch"},                   { 85, "Round Switch"},               { 86, "Unused Small Yellow Platform"},     { 87, "Unused Small Grey Platform"},
	{ 88, "Unused Small Green Platform"},     { 89, "Small Thin Yellow Platform"}, { 90, "Small Thin Grey Platform"},         { 91, "Unused Small Thin Green Platform"},
	{ 92, "Unused Large Dark Grey Platform"}, { 93, "Large Thin Grey Platform"},   { 94, "Unused Large Thin Green Platform"}, { 95, "Unused Large Thin Yellow Platform"},
	{ 96, "Item Drop"},                       { 97, "Friday"},                     { 98, "Boulder"},                          { 99, "Nigel In Bed"},
	{100, "Goddess Statue"},                  {101, "2 Way Sign Post"},            {102, "3 Way Sign Post"},                  {103, "Gate - South"},
	{104, "Gate - North"},                    {105, "Kayla"},                      {106, "Wally"},                            {107, "Woman 3"},
	{108, "Locked Door"},                     {109, "Man In Bed"},                 {110, "Arthur"},                           {111, "Kayla In Bath"},
	{112, "Duke"},                            {113, "Small Wooden Platform"},      {114, "Small Thin Dark Grey Platform"},    {115, "Yellow Old Man"},
	{116, "Unused Man Blue Shirt"},           {117, "Chef"},                       {118, "Princess"},                         {119, "Golem Statue 1"},
	{120, "Mir Magic Barrier"},               {121, "Sailor"},                     {122, "Pockets"},                          {123, "Skeleton Priest"},
	{124, "Mir"},                             {125, "Unused Bird 1"},              {126, "Unused Bird 2"},                    {127, "Unused Bird 3"},
	{128, "Moralis"},                         {129, "Bubble 4"},                   {130, "Bubble 5"},                         {131, "Bubble 6"},
	{132, "Gnome"},                           {133, "Zak"},                        {134, "Mir Wall Barrier"},                 {135, "Small Fireball"},
	{136, "Reaper 1"},                        {137, "Reaper 2"},                   {138, "Unused Reaper 3"},                  {139, "Golem Statue 2"},
	{140, "Corrupted Round Switch"},          {141, "Small Green Spiked Ball"},    {142, "Large Green Spiked Ball"},          {143, "Ghost Generator 1"},
	{144, "Ghost Generator 2"},               {145, "Ghost Generator 3"},          {146, "Golem 1"},                          {147, "Golem 2"},
	{148, "Golem 3"},                         {149, "Spectre 1"},                  {150, "Spectre 2"},                        {151, "Spectre 3"},
	{152, "Unused Small Blue Ball"},          {153, "Small Red Ball"},             {154, "Unused Large Blue Ball"},           {155, "Large Red Ball"},
	{156, "Unused Mage Fox"},                 {157, "Spinner 1"},                  {158, "Duke In Chair"},                    {159, "Miro"},
	{160, "Ifrit"},                           {161, "Ifrit Fireball"},             {162, "Gola"},                             {163, "Pink Golem Statue"},
	{164, "Gold Golem Statue"},               {165, "Nole"},                       {166, "Nole Axe Projectile"},              {167, "Stone Warrior 1"},
	{168, "Zak Boomerang"},                   {169, "1 Way Sign Post"},            {170, "Spinner 2"},                        {171, "Stone Warrior 2"},
	{172, "Gola Fireball"},
	{192, "Eke Eke"},                         {193, "Magic Sword"},                {194, "Sword Of Ice"},                     {195, "Thunder Sword"},
	{196, "Sword Of Gaia"},                   {197, "Fireproof"},                  {198, "Iron Boots"},                       {199, "Healing Boots"},
	{200, "Snow Spikes"},                     {201, "Steel Breast"},               {202, "Chrome Breast"},                    {203, "Shell Breast"},
	{204, "Hyper Breast"},                    {205, "Mars Stone"},                 {206, "Moon Stone"},                       {207, "Saturn Stone"},
	{208, "Venus Stone"},                     {209, "Awakening Book"},             {210, "Detox Grass"},                      {211, "Statue Of Gaia"},
	{212, "Golden Statue"},                   {213, "Mind Repair"},                {214, "Casino Ticket"},                    {215, "Axe Magic"},
	{216, "Blue Ribbon"},                     {217, "Buyer Card"},                 {218, "Lantern"},                          {219, "Garlic"},
	{220, "Antiparalyze"},                    {221, "Statue Of Jypta"},            {222, "Sunstone"},                         {223, "Armlet"},
	{224, "Einstein Whistle"},                {225, "Detox Book"},                 {226, "Anticurse"},                        {227, "Record Book"},
	{228, "Spellbook"},                       {229, "Hotel Register"},             {230, "Island Map"},                       {231, "Lithograph"},
	{232, "Red Jewel"},                       {233, "Pawn Ticket"},                {234, "Purple Jewel"},                     {235, "Gola's Eye"},
	{236, "Death Statue"},                    {237, "Dahl"},                       {238, "Restoration"},                      {239, "Logs"},
	{240, "Oracle Stone"},                    {241, "Idol Stone"},                 {242, "Key"},                              {243, "Safety Pass"},
	{244, "No52"},                            {245, "Bell"},                       {246, "Shortcake"},                        {247, "Gola's Nail"},
	{248, "Gola's Horn"},                     {249, "Gola's Fang"},                {250, "Broadsword"},                       {251, "Leather Breast"},
	{252, "Leather Boots"},                   {253, "None"},                       {254, "Lifestock"},                        {255, "No63"}
};

template <typename T>
static void PopulateLUT(const std::vector<uint8_t>& input, T& output, bool reverse = false)
{
	assert(input.size() % 2 == 0); // Ensure even number of elements
	for (size_t i = 0; i < input.size(); i += 2)
	{
		output.insert({input[i + reverse], input[i + 1 - reverse] });
	}
}

Sprite::Sprite()
	:m_sprite_id(-1)
{
}

Sprite::Sprite(const Rom& rom, uint8_t id)
	: m_sprite_id(id),
	  m_high_palette(rom, 0, PaletteO::Type::SPRITE_HIGH),
	  m_low_palette(rom, 0, PaletteO::Type::SPRITE_LOW)
{
	if (!m_cache_init)
	{
		PopulateLUT(rom.read_array<uint8_t>("sprite_gfx_lookup"), m_sprite_gfx_lookup, true);
		PopulateLUT(rom.read_array<uint8_t>("sprite_palette_lookup"), m_sprite_palette_lookup);
		const uint32_t start_of_anim_table = rom.read<uint32_t>("sprite_table_ptr");
		const uint32_t start_of_frame_table = rom.read<uint32_t>(start_of_anim_table);
		const uint32_t start_of_frames = rom.read<uint32_t>(start_of_frame_table);

		std::set<uint32_t> frame_offsets;
		for (uint32_t frame_offset = start_of_frame_table; frame_offset < start_of_frames; frame_offset += 4)
		{
			frame_offsets.insert(rom.read<uint32_t>(frame_offset));
		}

		std::vector<uint32_t> frames(frame_offsets.cbegin(), frame_offsets.cend());
		std::map<uint32_t, uint32_t> frame_offset_to_frame_num;
		for (size_t i = 0; i < frames.size(); ++i)
		{
			frame_offset_to_frame_num[frames[i]] = i;
			std::size_t frame_size;
			if (i < frames.size() - 2)
			{
				frame_size = frames[i + 1] - frames[i];
			}
			else
			{
				auto r = rom.get_address("sprite_data_end");
				frame_size = r - frames[i];
			}
			m_sprite_frames.emplace_back(SpriteFrame(rom.read_array<uint8_t>(frames[i], frame_size)));
		}

		std::vector<uint32_t> sprite_frames;
		size_t i = 0;
		std::vector<uint16_t> sprite_offsets = rom.read_array<uint16_t>("sprite_gfx_offset_table");
		for (size_t s = 0; s < sprite_offsets.size(); s += 2)
		{
			m_sprite_graphics.emplace_back(i++);
			uint32_t start_anim_offset = sprite_offsets[s] * 4 + start_of_anim_table;
			uint32_t end_anim_offset;
			if (s + 2 >= sprite_offsets.size())
			{
				end_anim_offset = start_of_frame_table;
			}
			else
			{
				end_anim_offset = sprite_offsets[s + 2] * 4 + start_of_anim_table;
			}
			for (uint32_t aoffset = start_anim_offset; aoffset < end_anim_offset; aoffset += 4)
			{
				uint32_t start_frame_offset = rom.read<uint32_t>(aoffset);
				uint32_t end_frame_offset;
				if (aoffset + 4 >= start_of_frames)
				{
					end_frame_offset = start_of_frames;
				}
				else
				{
					end_frame_offset = rom.read<uint32_t>(aoffset + 4);
				}
				std::vector<uint32_t> sframes = rom.read_array<uint32_t>(start_frame_offset, (end_frame_offset - start_frame_offset) / 4);
				for (auto& frame : sframes)
				{
					frame = frame_offset_to_frame_num[frame];
				}
				m_sprite_graphics.back().AddAnimation(sframes);
			}
		}
		m_cache_init = true;
	}
}

int Sprite::GetID() const
{
	return m_sprite_id;
}

int Sprite::GetGraphicsIdx() const
{
	auto result = m_sprite_gfx_lookup.find(m_sprite_id);
	if (result != m_sprite_gfx_lookup.end())
	{
		return result->second;
	}
	return -1;
}

const SpriteGraphic& Sprite::GetGraphics() const
{
	auto result = m_sprite_gfx_lookup.find(m_sprite_id);
	if (result != m_sprite_gfx_lookup.end())
	{
		return m_sprite_graphics[result->second];
	}
	throw std::runtime_error("Error! Sprite " + std::to_string(m_sprite_id) + " does not have any associated graphics.");
}

int Sprite::GetDefaultAnimationId() const
{
	if (IsItem())
	{
		return GetItemId() >> 3;
	}
	return GetGraphics().GetAnimationCount() > 1 ? 1 : 0;
}

int Sprite::GetDefaultFrameId() const
{
	if (IsItem())
	{
		return GetItemId() & 0x07;
	}
	return 0;
}

bool Sprite::IsItem() const
{
	// Items always have an ID of 0xC0 or above
	return m_sprite_id >= 0xC0;
}

int Sprite::GetItemId() const
{
	if (IsItem())
	{
		return m_sprite_id & 0x3F;
	}
	return -1;
}

PaletteO Sprite::GetPalette() const
{
	PaletteO pal;
	auto it = m_sprite_palette_lookup.equal_range(m_sprite_id);
	for (auto itr = it.first; itr != it.second; ++itr)
	{
		if (itr->second & 0x80)
		{
			m_high_palette.Load(itr->second & 0x7F);
			pal.AddHighPalette(m_high_palette);
		}
		else
		{
			m_low_palette.Load(itr->second & 0x7F);
			pal.AddLowPalette(m_low_palette);
		}
	}
	return pal;
}

std::string Sprite::GetName() const
{
	auto result = SPRITE_NAME_LOOKUP.find(m_sprite_id);
	if (result != SPRITE_NAME_LOOKUP.end())
	{
		return result->second;
	}
	return "Unknown";
}

void Sprite::Draw(ImageBuffer& imgbuf, size_t animation, size_t frame, uint8_t palette_idx, size_t x, size_t y, float scale) const
{
	auto sprite_frame = m_sprite_frames[GetGraphics().RetrieveFrameIdx(animation, frame)];

	for (std::size_t i = 0; i < sprite_frame.GetSubSpriteCount(); ++i)
	{
		const auto& subs = sprite_frame.GetSubSprite(i);
		std::size_t index = subs.tile_idx;
		for (std::size_t xi = 0; xi < subs.w; ++xi)
			for (std::size_t yi = 0; yi < subs.h; ++yi)
			{
				int xx = subs.x + xi * 8 + x;
				int yy = subs.y + yi * 8 + y;
				imgbuf.InsertTile(xx, yy, palette_idx, Tile(index++), sprite_frame.GetTileset());
			}
	}
}

void Sprite::Reset()
{
	m_sprite_palette_lookup.clear();
	m_sprite_gfx_lookup.clear();
	m_sprite_frames.clear();
	m_sprite_graphics.clear();
	m_cache_init = false;
}

bool Sprite::m_cache_init = false;
std::unordered_multimap<uint8_t, uint8_t>  Sprite::m_sprite_palette_lookup;
std::unordered_map<uint8_t, uint8_t>  Sprite::m_sprite_gfx_lookup;
std::vector<SpriteFrame> Sprite::m_sprite_frames;
std::vector<SpriteGraphic> Sprite::m_sprite_graphics;