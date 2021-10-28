#pragma once

#include "Core/Io/IVolume.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_CORE_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{

class FileSystem;

/*! Native Win32 file volume.
 * \ingroup Core
 */
class T_DLLCLASS NativeVolume : public IVolume
{
	T_RTTI_CLASS;

public:
	explicit NativeVolume(const Path& currentDirectory);

	virtual std::wstring getDescription() const override final;

	virtual Ref< File > get(const Path& path) override final;

	virtual int find(const Path& mask, RefArray< File >& out) override final;

	virtual bool modify(const Path& fileName, uint32_t flags) override final;

	virtual bool modify(const Path& fileName, const DateTime* creationTime, const DateTime* lastAccessTime, const DateTime* lastWriteTime) override final;

	virtual Ref< IStream > open(const Path& fileName, uint32_t mode) override final;

	virtual bool exist(const Path& fileName) override final;

	virtual bool remove(const Path& fileName) override final;

	virtual bool move(const Path& fileName, const std::wstring& newName, bool overwrite) override final;

	virtual bool copy(const Path& fileName, const std::wstring& newName, bool overwrite) override final;

	virtual bool makeDirectory(const Path& directory) override final;

	virtual bool removeDirectory(const Path& directory) override final;

	virtual bool renameDirectory(const Path& directory, const std::wstring& newName) override final;

	virtual bool setCurrentDirectory(const Path& directory) override final;

	virtual Path getCurrentDirectory() const override final;

	static void mountVolumes(FileSystem& fileSystem);

private:
	Path m_currentDirectory;

	std::wstring getSystemPath(const Path& path) const;
};

}

