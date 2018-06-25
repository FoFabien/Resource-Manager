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
    if(!rm.loadFile(pack, file)) return false;
    raw = rm.getData(pack, file);
    pos = 0;
    if(raw != nullptr) return true;
    return false;
}

std::streampos MemoryStream::read(void* data, size_t size)
{
    if(raw == nullptr) return false;

    RawVector &ref = *raw;
    if(pos + size >= ref.size())
    {
        std::copy(ref.begin()+pos, ref.end(), (uint8_t*)data);
        return (ref.size()-pos);
    }
    else
    {
        std::copy(ref.begin()+pos, ref.begin()+pos+size, (uint8_t*)data);
        return size;
    }
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
    in.close();
    data = rm.getDataContainer(pack, file);
    if(data.off == 0) return false;

    in.open(pack, std::ios::in | std::ios::binary);
    if(!in) return false;

    keys = ResourceManager::readKeys(in);
    in.seekg(data.off);
    in.read((char*)&keys.first, 4);
    if(!in.good()) return false;
    keys.first = keys.first ^ keys.second;
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
    if(!in) return -1;
    return data.size;
}
