#include "resourcemanager.hpp"
#include <iostream>
#include <random>
#include <chrono>

// magic number
#define RS_VERSION 0x52503031 // "RP01"

ResourceManager::ResourceManager()
{
    def = "";
}

ResourceManager::~ResourceManager()
{
    mutex.lock();
    for(auto &i: root)
        for(auto &j: *i.second)
            delete j.second;
    mutex.unlock();
}

void ResourceManager::clearPack(Pack& pack)
{
    for(auto &i: pack)
        delete i.second;
}

bool ResourceManager::loadResources(const std::string& file, const bool &isDefault)
{
    std::ifstream f(file, std::ios::in | std::ios::binary);
    if(!f) return false;

    size_t tmp;
    int32_t nfile;
    Keys keys, seed;
    std::string name;
    char c;
    std::shared_ptr<Pack> pack(new Pack());
    std::streampos file_off = 0;
    Root::iterator it;

    keys = readKeys(f); // read the encryption keys at the beginning
    seed = keys; // keep a copy
    f.read((char*)&tmp, 4);
    file_off = tmp ^ keys.second; // end of header offset
    if(cread(f, (char*)&tmp, 4, seed) != 4 || tmp != RS_VERSION) goto io_error; // key check
    if(cread(f, (char*)&nfile, 4, seed) != 4) goto io_error; // number of file

    for(int32_t i = 0; i < nfile; ++i) // read file headers
    {
        name = "";
        do
        {
            if(!cread(f, (char*)&c, seed)) goto io_error;
            if(c != 0) name += c;
        }while(c != 0);
        DataContainer* current = new DataContainer();
        (*pack)[name] = current;
        if(cread(f, (char*)&(current->size), 4, seed) != 4) goto io_error;
        current->off = file_off;
        file_off += current->size + 4;
    }

    // file data aren't loaded now, use loadFile()

    // add the pack in the root list
    mutex.lock();
    it = root.find(file);
    if(it != root.end())
        clearPack(*it->second);
    root[file] = pack;

    // set as default if the needed
    if(isDefault)
        def = file;
    mutex.unlock();
    return true;

io_error:

    // if error
    clearPack(*pack); // clear the unfinished pack
    return false;
}

bool ResourceManager::saveResources(const std::string& file, const std::vector<std::string> &filelist)
{
    std::ofstream f(file, std::ios::out | std::ios::trunc | std::ios::binary);
    if(!f) return false;

    size_t tmp;
    std::mt19937 mt(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<size_t> dist(0x0, 0xFFFFFFFF);
    Keys keys = {dist(mt), dist(mt)};
    Keys seed = keys;
    char c;
    std::map<std::string, size_t> fsizes;
    std::streampos pos;

    std::cout << "[ResourceManager] Building resource pack '" << file << "'" << std::endl;

    tmp = keys.first ^ RS_VERSION;
    f.write((char*)&tmp, 4);
    tmp = RS_VERSION - (keys.second ^ keys.first);
    f.write((char*)&tmp, 4);
    tmp = 0;
    f.write((char*)&tmp, 4); // placeholder
    tmp = RS_VERSION;
    if(cwrite(f, (char*)&tmp, 4, seed) != 4 || !f.good()) goto io_error;
    tmp = filelist.size();
    if(cwrite(f, (char*)&tmp, 4, seed) != 4 || !f.good()) goto io_error;

    std::cout << "[ResourceManager] File count: " << std::to_string(tmp) << std::endl;

    // header
    for(auto &i: filelist)
    {
        tmp = i.size();
        if(cwrite(f, i.c_str(), tmp, seed) != (std::streamsize)tmp || !f.good()) goto io_error;
        c = 0;
        if(!cwrite(f, &c, seed)) goto io_error;

        std::ifstream in(i, std::ios::in | std::ios::binary);
        if(!in)
        {
            tmp = 0;
            std::cout << "[ResourceManager] Error: Can't read '" << i << "', size will be set to 0" << std::endl;
        }
        else
        {
            tmp = in.tellg();
            in.seekg(0, std::ios::end);
            tmp = (size_t)in.tellg() - tmp;
            if(tmp == 0)
                std::cout << "[ResourceManager] Warning: File size of '" << i << "' is 0" << std::endl;
        }
        fsizes[i] = tmp;
        if(cwrite(f, (char*)&tmp, 4, seed) != 4 || !f.good()) goto io_error;
    }
    // writing header end
    pos = f.tellp();
    f.seekp(8, std::ios::beg);
    tmp = pos ^ keys.second;
    f.write((char*)&tmp, 4);
    f.seekp(pos);

    std::cout << "[ResourceManager] Header complete, now writing data..." << std::endl;

    // content
    for(auto &i: filelist)
    {
        tmp = seed.first ^ seed.second;
        f.write((char*)&tmp, 4);

        std::ifstream in(i, std::ios::in | std::ios::binary);
        if(!in) continue;
        size_t &ref = fsizes[i];
        for(size_t j = 0; j < ref; ++j)
        {
            in.read(&c, 1);
            if(!cwrite(f, &c, seed)) goto io_error;
        }
    }
    tmp = 0;
    cwrite(f, (char*)&tmp, 4, seed); // padding
    std::cout << "[ResourceManager] Process complete" << std::endl;

    return true;

io_error:

    std::cout << "[ResourceManager] Write Error, process aborted." << std::endl;
    return false;
}

bool ResourceManager::loadFile(const std::string& pack, const std::string& file)
{
    return loadFile(pack, file, nullptr);
}

bool ResourceManager::loadFile(const std::string& pack, const std::string& file, std::function<bool(DataContainer)> callback)
{
    std::ifstream f;
    Keys keys;
    size_t tmp;
    RawVector *raw;
    Root::iterator it;
    Pack::iterator file_it;

    Lock lock(mutex);

    it = root.find(pack);
    if(it == root.end()) return false;
    file_it = it->second->find(file);
    if(file_it == it->second->end()) return false;
    if(file_it->second->data != nullptr) return true; // already loaded

    f.open(pack, std::ios::in | std::ios::binary);
    if(!f) return false;
    keys = readKeys(f);

    f.seekg(file_it->second->off);
    f.read((char*)&tmp, 4);
    if(!f.good()) return false;
    keys.first = tmp ^ keys.second;

    if(file_it->second->size > 0)
    {
        file_it->second->data = std::make_shared<RawVector>(RawVector());
        raw = file_it->second->data.get();
        raw->resize(file_it->second->size);
        if(cread(f, (char*)&((*raw)[0]), file_it->second->size, keys) != (std::streampos)file_it->second->size)
        {
            file_it->second->data = nullptr;
            return false;
        }
    }
    if(callback)
        return callback(*(file_it->second));
    return true;
}

bool ResourceManager::loadFile(const std::string& file)
{
    return loadFile(def, file, nullptr);
}

bool ResourceManager::loadFile(const std::string& file, std::function<bool(DataContainer)> callback)
{
    return loadFile(def, file, callback);
}

bool ResourceManager::loadFile(const PackPtr& ptr, const std::string& file)
{
    return loadFile(ptr.name, file, nullptr);
}

bool ResourceManager::loadFile(const PackPtr& ptr, const std::string& file, std::function<bool(DataContainer)> callback)
{
    return loadFile(ptr.name, file, callback);
}

bool ResourceManager::packExist(const std::string& pack) const
{
    return (root.find(pack) != root.end());
}

bool ResourceManager::fileExist(const std::string& pack, const std::string& file) const
{
    auto it = root.find(pack);
    if(it == root.end()) return false;
    return fileExist(it->second, file);
}

bool ResourceManager::fileExist(const std::string& file) const
{
    return fileExist(def, file);
}

bool ResourceManager::fileExist(const PackPtr& ptr, const std::string& file) const
{
    if(ptr.pack == nullptr) return false;
    return fileExist(ptr.pack, file);
}

bool ResourceManager::fileExist(const std::shared_ptr<Pack> &pack, const std::string& file) const
{
    return (pack->find(file) != pack->end());
}

void ResourceManager::trash(const std::string& pack, const std::string& file)
{
    auto it = root.find(pack);
    if(it == root.end()) return;
    trash(it->second, file);
}

void ResourceManager::trash(const std::string& file)
{
    trash(def, file);
}

void ResourceManager::trash(PackPtr& ptr, const std::string& file)
{
    if(ptr.pack == nullptr) return;
    fileExist(ptr.pack, file);
}

void ResourceManager::trash(std::shared_ptr<Pack> &pack, const std::string& file)
{
    Lock lock(mutex);
    auto it = pack->find(file);
    if(it == pack->end()) return;
    if(it->second->data.use_count() == 1)
    {
        it->second->data = nullptr;
        return;
    }
    trashbin.insert(it->second);
}

void ResourceManager::garbageCollector()
{
    Lock lock(mutex);
    auto it = trashbin.begin();
    while(it != trashbin.end())
    {
        auto current = it++;
        if((*current)->data.use_count() == 1)
        {
            (*current)->data = nullptr;
            trashbin.erase(current);
        }
    }
}

bool ResourceManager::getPackPtr(const std::string& pack, PackPtr& ptr)
{
    auto it = root.find(pack);
    if(it == root.end()) return false;
    ptr.pack = it->second;
    ptr.name = pack;
    return true;
}

RawPtr ResourceManager::getData(const std::string& pack, const std::string& file)
{
    auto it = root.find(pack);
    if(it == root.end()) return nullptr;
    return getData(it->second, file);
}

RawPtr ResourceManager::getData(const std::string& file)
{
    return getData(def, file);
}

RawPtr ResourceManager::getData(PackPtr& ptr, const std::string& file)
{
    if(ptr.pack == nullptr) return nullptr;
    return getData(ptr.pack, file);
}

RawPtr ResourceManager::getData(std::shared_ptr<Pack> &pack, const std::string& file)
{
    Lock lock(mutex);
    auto file_it = pack->find(file);
    if(file_it == pack->end()) return nullptr;
    return file_it->second->data;
}

DataContainer ResourceManager::getDataContainer(const std::string& pack, const std::string& file)
{
    auto it = root.find(pack);
    if(it == root.end()) return DataContainer();
    return getDataContainer(it->second, file);
}

DataContainer ResourceManager::getDataContainer(const std::string& file)
{
    return getDataContainer(def, file);
}

DataContainer ResourceManager::getDataContainer(PackPtr& ptr, const std::string& file)
{
    if(ptr.pack == nullptr) return DataContainer();
    return getDataContainer(ptr.pack, file);
}

DataContainer ResourceManager::getDataContainer(std::shared_ptr<Pack> &pack, const std::string& file)
{
    Lock lock(mutex);
    auto file_it = pack->find(file);
    if(file_it == pack->end()) return DataContainer();
    return *(file_it->second);
}

bool ResourceManager::cread(std::istream& f, char* c, Keys& seed)
{
    f.read(c, 1);
    *c = (*c + seed.second) ^ seed.first;
    seed.first = (seed.first << 4) + seed.second;
    return f.good();
}

std::streamsize ResourceManager::cread(std::istream& f, char* buf, std::streamsize n, Keys& seed)
{
    f.read((char*)buf, n);
    if(!f) n = f.gcount();
    for(std::streamsize i = 0; i < n; ++i)
    {
        buf[i] = (buf[i] + seed.second) ^ seed.first;
        seed.first = (seed.first << 4) + seed.second;
    }
    return n;
}

Keys ResourceManager::readKeys(std::istream& f)
{
    Keys keys;
    size_t tmp;
    f.read((char*)&tmp, 4);
    keys.first = tmp ^ RS_VERSION;
    f.read((char*)&tmp, 4);
    keys.second = (RS_VERSION - tmp) ^ keys.first;
    return keys;
}

bool ResourceManager::cwrite(std::ostream& f, char* c, Keys& seed)
{
    *c = (*c ^ seed.first) - seed.second;
    f.write(c, 1);
    seed.first = (seed.first << 4) + seed.second;
    return f.good();
}

std::streamsize ResourceManager::cwrite(std::ostream& f, const char* buf, std::streamsize n, Keys& seed)
{
    char c;
    for(std::streamsize i = 0; i < n; ++i)
    {
        c = (buf[i] ^ seed.first) - seed.second;
        f.write((char*)&c, 1);
        if(!f.good()) return i;
        seed.first = (seed.first << 4) + seed.second;
    }
    return n;
}


void ResourceManager::forwardKeys(Keys& seed, size_t n)
{
    for(size_t i = 0; i < n; ++i)
        seed.first = (seed.first << 4) + seed.second;
}
