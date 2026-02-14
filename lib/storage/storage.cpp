#include "storage.hpp"

#include <LittleFS.h>

#include <algorithm>

namespace
{
  bool ensureDirTree(const String &dirPath)
  {
    if (dirPath.isEmpty() || dirPath == "/")
    {
      return true;
    }

    String normalized = dirPath;
    if (!normalized.startsWith("/"))
    {
      normalized = "/" + normalized;
    }

    int slashPos = normalized.indexOf('/');
    while (slashPos >= 0)
    {
      slashPos = normalized.indexOf('/', slashPos + 1);
      String cur = (slashPos < 0) ? normalized : normalized.substring(0, slashPos);
      if (cur.isEmpty())
      {
        continue;
      }
      if (!LittleFS.exists(cur) && !LittleFS.mkdir(cur))
      {
        return false;
      }
    }

    return true;
  }
} // namespace

bool Storage::begin()
{
  return LittleFS.begin();
}

Storage::FsInfo Storage::info()
{
  FsInfo out;
  out.totalBytes = LittleFS.totalBytes();
  out.usedBytes = LittleFS.usedBytes();
  return out;
}

bool Storage::exists(const String &path)
{
  if (path.isEmpty())
  {
    return false;
  }
  return LittleFS.exists(path);
}

bool Storage::remove(const String &path)
{
  if (path.isEmpty())
  {
    return false;
  }
  if (!LittleFS.exists(path))
  {
    return true;
  }
  return LittleFS.remove(path);
}

bool Storage::ensureDirs(const String &path)
{
  if (path.isEmpty())
  {
    return false;
  }

  String normalized = path;
  if (!normalized.startsWith("/"))
  {
    normalized = "/" + normalized;
  }

  const int lastSlash = normalized.lastIndexOf('/');
  if (lastSlash <= 0)
  {
    return true;
  }

  const String dirPath = normalized.substring(0, lastSlash);
  return ensureDirTree(dirPath);
}

bool Storage::FileWriter::open(const String &path, uint32_t total_size)
{
  close(false);

  if (path.isEmpty())
  {
    return false;
  }

  if (!Storage::ensureDirs(path))
  {
    return false;
  }

  finalPath_ = path;
  tempPath_ = path + ".part";
  totalSize_ = total_size;
  bytesWritten_ = 0;
  opened_ = false;

  if (LittleFS.exists(tempPath_))
  {
    LittleFS.remove(tempPath_);
  }

  file_ = LittleFS.open(tempPath_, "w");
  if (!file_)
  {
    finalPath_ = String();
    tempPath_ = String();
    totalSize_ = 0;
    bytesWritten_ = 0;
    return false;
  }

  opened_ = true;
  return true;
}

bool Storage::FileWriter::writeAt(uint32_t offset, const uint8_t *data, uint16_t len)
{
  if (!opened_)
  {
    return false;
  }

  if (len == 0)
  {
    return true;
  }
  if (data == nullptr)
  {
    return false;
  }

  if (!file_.seek(offset, SeekSet))
  {
    return false;
  }

  const size_t wrote = file_.write(data, len);
  if (wrote != len)
  {
    return false;
  }

  const uint32_t newHighWater = offset + static_cast<uint32_t>(wrote);
  if (newHighWater > bytesWritten_)
  {
    bytesWritten_ = newHighWater;
  }

  return true;
}

void Storage::FileWriter::close(bool commit)
{
  if (opened_)
  {
    file_.flush();
    file_.close();
    opened_ = false;
  }

  if (commit)
  {
    if (LittleFS.exists(finalPath_))
    {
      LittleFS.remove(finalPath_);
    }
    LittleFS.rename(tempPath_, finalPath_);
  }
  else
  {
    if (LittleFS.exists(tempPath_))
    {
      LittleFS.remove(tempPath_);
    }
  }

  finalPath_ = String();
  tempPath_ = String();
  totalSize_ = 0;
  bytesWritten_ = 0;
}

uint32_t Storage::FileWriter::totalSize() const
{
  return totalSize_;
}

uint32_t Storage::FileWriter::bytesWritten() const
{
  return bytesWritten_;
}

Storage::FileWriter::~FileWriter()
{
  close(false);
}
