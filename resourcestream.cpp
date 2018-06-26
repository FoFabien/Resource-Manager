#include "resourcestream.hpp"

MemoryStream::MemoryStream()
{
    pos = 0;
}

MemoryStream::~MemoryStream()
{

}

bool MemoryStream::open(const std::string& pack, const std::string& file, ResourceManager& rm)
{
    PackPtr ptr;
    if(!rm.getPackPtr(pack, ptr)) return false;
    return open(ptr, file, rm);
}

bool MemoryStream::open(PackPtr& ptr, const std::string& file, ResourceManager& rm)
{
    if(!rm.loadFile(ptr, file)) return false;
    raw = rm.getData(ptr, file);
    pos = 0;
    if(raw != nullptr) return true;
    return false;
}

std::streampos MemoryStream::read(void* data, size_t size)
{
    if(raw == nullptr) return false;

    RawVector &ref = *raw;
    size_t start, end;
    start = pos;

    for(end = pos; end < ref.size() && end-pos < size; ++end)
    {
        *((uint8_t*)data) = ref[end];
        data = static_cast<char*>(data) + 1;
    }
    pos = end;
    return end - start;
}

std::streampos MemoryStream::seek(std::streampos position)
{
    if(raw == nullptr) return -1;
    size_t size = raw.get()->size();
    pos = (position < (std::streampos)size) ? position : (std::streampos)size;
    return pos;
}

std::streampos MemoryStream::tell()
{
    if(raw == nullptr) return -1;
    return pos;
}

std::streampos MemoryStream::getSize()
{
    if(raw == nullptr) return -1;
    return raw.get()->size();
}


FileStream::FileStream()
{
    pos = 0;
}

FileStream::~FileStream()
{

}

bool FileStream::open(const std::string& pack, const std::string& file, ResourceManager& rm)
{
    PackPtr ptr;
    if(!rm.getPackPtr(pack, ptr)) return false;
    return open(ptr, file, rm);
}

bool FileStream::open(PackPtr& ptr, const std::string& file, ResourceManager& rm)
{
    in.close();
    data = rm.getDataContainer(ptr, file);
    if(ptr.name.empty())
    {
        if(!rm.fileExist(ptr, file)) return false;

        in.open(file, std::ios::in | std::ios::binary);
        if(!in) return false;

        keys = {0, 0};

        data.size = in.tellg();
        in.seekg(0, std::ios::end);
        data.size = (size_t)in.tellg() - data.size;
        in.seekg(0, std::ios::beg);
    }
    else
    {
        if(data.off == -1) return false;

        in.open(ptr.name, std::ios::in | std::ios::binary);
        if(!in) return false;

        keys = ResourceManager::readKeys(in);
        in.seekg(data.off);
        in.read((char*)&keys.first, 4);
        if(!in.good()) return false;
        keys.first = keys.first ^ keys.second;
    }
    seed = keys;
    pos = 0;

    return true;
}

std::streampos FileStream::read(void* data, size_t size)
{
    if(!in) return -1;

    std::streampos read = ResourceManager::cread(in, (char*)data, size, seed);
    pos += read;
    return read;
}

std::streampos FileStream::seek(std::streampos position)
{
    if(!in || position < 0) return -1;
    if(position >= (std::streampos)data.size)
        position = data.size;

    if(pos < position)
    {
        ResourceManager::forwardKeys(seed, (size_t)position-pos);
        pos = position;
        in.seekg(position);
    }
    else if(pos > position)
    {
        seed = keys;
        ResourceManager::forwardKeys(seed, (size_t)position);
        pos = position;
        in.seekg(position);
    }
    return pos;
}

std::streampos FileStream::tell()
{
    if(!in) return -1;
    return pos;
}

std::streampos FileStream::getSize()
{
    return data.size;
}
