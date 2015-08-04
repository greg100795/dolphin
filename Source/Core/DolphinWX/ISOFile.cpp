// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <wx/app.h>
#include <wx/bitmap.h>
#include <wx/filefn.h>
#include <wx/image.h>
#include <wx/toplevel.h>

#include "Common/ChunkFile.h"
#include "Common/CommonPaths.h"
#include "Common/CommonTypes.h"
#include "Common/FileUtil.h"
#include "Common/Hash.h"
#include "Common/IniFile.h"
#include "Common/StringUtil.h"

#include "Core/ConfigManager.h"
#include "Core/Boot/Boot.h"

#include "DiscIO/CompressedBlob.h"
#include "DiscIO/Filesystem.h"
#include "DiscIO/Volume.h"
#include "DiscIO/VolumeCreator.h"

#include "DolphinWX/ISOFile.h"
#include "DolphinWX/WxUtils.h"

static const u32 CACHE_REVISION = 0x125; // Last changed in PR 2598

#define DVD_BANNER_WIDTH 96
#define DVD_BANNER_HEIGHT 32

static std::string GetLanguageString(DiscIO::IVolume::ELanguage language, std::map<DiscIO::IVolume::ELanguage, std::string> strings)
{
	auto end = strings.end();
	auto it = strings.find(language);
	if (it != end)
		return it->second;

	// English tends to be a good fallback when the requested language isn't available
	if (language != DiscIO::IVolume::ELanguage::LANGUAGE_ENGLISH)
	{
		it = strings.find(DiscIO::IVolume::ELanguage::LANGUAGE_ENGLISH);
		if (it != end)
			return it->second;
	}

	// If English isn't available either, just pick something
	if (!strings.empty())
		return strings.cbegin()->second;

	return "";
}

GameListItem::GameListItem(const std::string& _rFileName)
	: m_FileName(_rFileName)
	, m_emu_state(0)
	, m_FileSize(0)
	, m_Revision(0)
	, m_Valid(false)
	, m_BlobCompressed(false)
	, m_ImageWidth(0)
	, m_ImageHeight(0)
{
	if (LoadFromCache())
	{
		m_Valid = true;

		// Wii banners can only be read if there is a savefile,
		// so sometimes caches don't contain banners. Let's check
		// if a banner has become available after the cache was made.
		if (m_pImage.empty())
		{
			std::unique_ptr<DiscIO::IVolume> volume(DiscIO::CreateVolumeFromFilename(_rFileName));
			if (volume != nullptr)
			{
				ReadBanner(*volume);
				if (!m_pImage.empty())
					SaveToCache();
			}
		}
	}
	else
	{
		DiscIO::IVolume* pVolume = DiscIO::CreateVolumeFromFilename(_rFileName);

		if (pVolume != nullptr)
		{
			m_Platform = pVolume->GetVolumeType();

			m_names = pVolume->GetNames(true);
			m_descriptions = pVolume->GetDescriptions();
			m_company = pVolume->GetCompany();

			m_Country = pVolume->GetCountry();
			m_FileSize = pVolume->GetRawSize();
			m_VolumeSize = pVolume->GetSize();

			m_UniqueID = pVolume->GetUniqueID();
			m_BlobCompressed = DiscIO::IsCompressedBlob(_rFileName);
			m_disc_number = pVolume->GetDiscNumber();
			m_Revision = pVolume->GetRevision();

			ReadBanner(*pVolume);

			delete pVolume;

			m_Valid = true;
			SaveToCache();
		}
	}

	if (m_company.empty() && m_UniqueID.size() >= 6)
		m_company = DiscIO::GetCompanyFromID(m_UniqueID.substr(4, 2));

	if (IsValid())
	{
		IniFile ini = SConfig::LoadGameIni(m_UniqueID, m_Revision);
		ini.GetIfExists("EmuState", "EmulationStateId", &m_emu_state);
		ini.GetIfExists("EmuState", "EmulationIssues", &m_issues);
	}

	if (!m_pImage.empty())
	{
		wxImage Image(m_ImageWidth, m_ImageHeight, &m_pImage[0], true);
		double Scale = wxTheApp->GetTopWindow()->GetContentScaleFactor();
		// Note: This uses nearest neighbor, which subjectively looks a lot
		// better for GC banners than smooth scaling.
		Image.Rescale(DVD_BANNER_WIDTH * Scale, DVD_BANNER_HEIGHT * Scale);
#ifdef __APPLE__
		m_Bitmap = wxBitmap(Image, -1, Scale);
#else
		m_Bitmap = wxBitmap(Image, -1);
#endif
	}
	else
	{
		// default banner
		m_Bitmap.LoadFile(StrToWxStr(File::GetThemeDir(SConfig::GetInstance().theme_name)) + "nobanner.png", wxBITMAP_TYPE_PNG);
	}
}

GameListItem::~GameListItem()
{
}

bool GameListItem::LoadFromCache()
{
	return CChunkFileReader::Load<GameListItem>(CreateCacheFilename(), CACHE_REVISION, *this);
}

void GameListItem::SaveToCache()
{
	if (!File::IsDirectory(File::GetUserPath(D_CACHE_IDX)))
		File::CreateDir(File::GetUserPath(D_CACHE_IDX));

	CChunkFileReader::Save<GameListItem>(CreateCacheFilename(), CACHE_REVISION, *this);
}

void GameListItem::DoState(PointerWrap &p)
{
	p.Do(m_names);
	p.Do(m_descriptions);
	p.Do(m_company);
	p.Do(m_UniqueID);
	p.Do(m_FileSize);
	p.Do(m_VolumeSize);
	p.Do(m_Country);
	p.Do(m_BlobCompressed);
	p.Do(m_pImage);
	p.Do(m_ImageWidth);
	p.Do(m_ImageHeight);
	p.Do(m_Platform);
	p.Do(m_disc_number);
	p.Do(m_Revision);
}

std::string GameListItem::CreateCacheFilename()
{
	std::string Filename, LegalPathname, extension;
	SplitPath(m_FileName, &LegalPathname, &Filename, &extension);

	if (Filename.empty()) return Filename; // Disc Drive

	// Filename.extension_HashOfFolderPath_Size.cache
	// Append hash to prevent ISO name-clashing in different folders.
	Filename.append(StringFromFormat("%s_%x_%" PRIx64 ".cache",
		extension.c_str(), HashFletcher((const u8 *)LegalPathname.c_str(), LegalPathname.size()),
		File::GetSize(m_FileName)));

	std::string fullname(File::GetUserPath(D_CACHE_IDX));
	fullname += Filename;
	return fullname;
}

void GameListItem::ReadBanner(const DiscIO::IVolume& volume)
{
	std::vector<u32> Buffer = volume.GetBanner(&m_ImageWidth, &m_ImageHeight);
	u32* pData = Buffer.data();
	m_pImage.resize(m_ImageWidth * m_ImageHeight * 3);

	for (int i = 0; i < m_ImageWidth * m_ImageHeight; i++)
	{
		m_pImage[i * 3 + 0] = (pData[i] & 0xFF0000) >> 16;
		m_pImage[i * 3 + 1] = (pData[i] & 0x00FF00) >> 8;
		m_pImage[i * 3 + 2] = (pData[i] & 0x0000FF) >> 0;
	}
}

std::string GameListItem::GetDescription(DiscIO::IVolume::ELanguage language) const
{
	return GetLanguageString(language, m_descriptions);
}

std::string GameListItem::GetDescription() const
{
	bool wii = m_Platform != DiscIO::IVolume::GAMECUBE_DISC;
	return GetDescription(SConfig::GetInstance().GetCurrentLanguage(wii));
}

std::string GameListItem::GetName(DiscIO::IVolume::ELanguage language) const
{
	return GetLanguageString(language, m_names);
}

std::string GameListItem::GetName() const
{
	bool wii = m_Platform != DiscIO::IVolume::GAMECUBE_DISC;
	std::string name = GetName(SConfig::GetInstance().GetCurrentLanguage(wii));
	if (name.empty())
	{
		// No usable name, return filename (better than nothing)
		SplitPath(GetFileName(), nullptr, &name, nullptr);
	}
	return name;
}

std::vector<DiscIO::IVolume::ELanguage> GameListItem::GetLanguages() const
{
	std::vector<DiscIO::IVolume::ELanguage> languages;
	for (std::pair<DiscIO::IVolume::ELanguage, std::string> name : m_names)
		languages.push_back(name.first);
	return languages;
}

const std::string GameListItem::GetWiiFSPath() const
{
	DiscIO::IVolume *iso = DiscIO::CreateVolumeFromFilename(m_FileName);
	std::string ret;

	if (iso == nullptr)
		return ret;

	if (iso->GetVolumeType() != DiscIO::IVolume::GAMECUBE_DISC)
	{
		u64 title = 0;

		iso->GetTitleID((u8*)&title);
		title = Common::swap64(title);

		const std::string path = StringFromFormat("%s/title/%08x/%08x/data/",
				File::GetUserPath(D_WIIROOT_IDX).c_str(), (u32)(title>>32), (u32)title);

		if (!File::Exists(path))
			File::CreateFullPath(path);

		if (path[0] == '.')
			ret = WxStrToStr(wxGetCwd()) + path.substr(strlen(ROOT_DIR));
		else
			ret = path;
	}
	delete iso;

	return ret;
}

