#ifndef RESOURCESTREAM_HPP
#define RESOURCESTREAM_HPP

#include <fstream>
#include "resourcemanager.hpp"

class ResourceStream
{
    public:
        virtual ~ResourceStream() {}
        virtual std::streampos read(void* data, size_t size) = 0;
        virtual std::streampos seek(std::streampos position) = 0;
        virtual std::streampos tell() = 0;
        virtual std::streampos getSize() = 0;
};

class MemoryStream: public ResourceStream // stream file from memory
{
    public:
        MemoryStream();
        virtual ~MemoryStream();
        bool open(const std::string& pack, const std::string& file, ResourceManager& rm);
        bool open(PackPtr& ptr, const std::string& file, ResourceManager& rm);
        virtual std::streampos read(void* data, size_t size);
        virtual std::streampos seek(std::streampos position);
        virtual std::streampos tell();
        virtual std::streampos getSize();
    protected:
        size_t pos;
        RawPtr raw;
};

class FileStream: public ResourceStream // stream file from disk (packed or unpacked)
{
    public:
        FileStream();
        virtual ~FileStream();
        bool open(const std::string& pack, const std::string& file, ResourceManager& rm);
        bool open(PackPtr& ptr, const std::string& file, ResourceManager& rm);
        virtual std::streampos read(void* data, size_t size);
        virtual std::streampos seek(std::streampos position);
        virtual std::streampos tell();
        virtual std::streampos getSize();
    protected:
        size_t pos;
        DataContainer data;
        std::ifstream in;
        Keys keys;
        Keys seed;
};

#endif // RESOURCESTREAM_HPP
