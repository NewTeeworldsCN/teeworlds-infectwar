/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <engine/map.h>
#include <engine/storage.h>
#include "datafile.h"

class CMap : public IEngineMap
{
	CDataFileReader m_DataFile;
public:
	CMap() {}

	void *GetData(int Index) override { return m_DataFile.GetData(Index); }
	void *GetDataSwapped(int Index) override { return m_DataFile.GetDataSwapped(Index); }
	void UnloadData(int Index) override { m_DataFile.UnloadData(Index); }
	void *GetItem(int Index, int *pType, int *pID) override { return m_DataFile.GetItem(Index, pType, pID); }
	void GetType(int Type, int *pStart, int *pNum) override { m_DataFile.GetType(Type, pStart, pNum); }
	void *FindItem(int Type, int ID) override { return m_DataFile.FindItem(Type, ID); }
	int NumItems() override { return m_DataFile.NumItems(); }

	void Unload() override
	{
		m_DataFile.Close();
	}

	bool Load(const char *pMapName) override
	{
		IStorage *pStorage = Kernel()->RequestInterface<IStorage>();
		if(!pStorage)
			return false;
		return m_DataFile.Open(pStorage, pMapName, IStorage::TYPE_ALL);
	}

	bool IsLoaded() override
	{
		return m_DataFile.IsOpen();
	}

	unsigned Crc() override
	{
		return m_DataFile.Crc();
	}
};

extern IEngineMap *CreateEngineMap() { return new CMap; }
