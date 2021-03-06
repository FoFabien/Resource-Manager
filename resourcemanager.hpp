#ifndef RESOURCEMANAGER_HPP
#define RESOURCEMANAGER_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <memory>
#include <fstream>
#include <utility>
#include <functional>

// definitions
typedef std::vector<uint8_t> RawVector; // binary data container
typedef std::shared_ptr<RawVector> RawPtr; // shared ptr to binary data
struct DataContainer // pretty much define a file in memory
{
    size_t size = 0; // its size
    std::streampos off = -1; // offset in the data pack
    RawPtr data; // binary data ptr (if loaded)
};
typedef std::map<std::string, DataContainer*> Pack; // a data pack
typedef std::map<std::string, std::shared_ptr<Pack> > Root; // multiple data pack
typedef std::lock_guard<std::mutex> Lock; // to make lock_guard less verboose
typedef std::pair<size_t, size_t> Keys; // encrytion keys
struct PackPtr // only ResourceManager should modify these values (keeping them public for more control)
{
    std::string name;
    std::shared_ptr<Pack> pack;
};


class ResourceManager
{
    public:
        ResourceManager();
        ~ResourceManager();

        // data pack manipulation
        bool loadResources(const std::string& file, const bool &isDefault = false); // load a pack. if isDefault is true and the load is successful, the pack will be the default one when not specifying a pack name
        static bool saveResources(const std::string& file, const std::vector<std::string> &filelist); // read the file paths stored in filelist and create a new data pack on the disk
        bool packExist(const std::string& pack) const; // check if the data pack exists in memory
        bool getPackPtr(const std::string& pack, PackPtr& ptr); // check if the pack exists and put a ptr in 'ptr'. It's a shared ptr so make sure to manage it properly.

        // file manipulation
        bool loadFile(const std::string& pack, const std::string& file); // access the file data in the pack and load everything into the RawPtr (not recommended for big files)
        bool loadFile(const std::string& pack, const std::string& file, std::function<bool(DataContainer)> callback); // same but call the callback if successful. The function return true on success. The loaded file is passed as parameter
        bool loadFile(const std::string& file); // same but use the pack defined as default
        bool loadFile(const std::string& file, std::function<bool(DataContainer)> callback); // same but use the pack defined as default + call the callback
        bool loadFile(PackPtr& ptr, const std::string& file); // same but use the pack in the pack ptr
        bool loadFile(PackPtr& ptr, const std::string& file, std::function<bool(DataContainer)> callback); // same but use the pack in the pack ptr + call the callback
        bool fileExist(const std::string& pack, const std::string& file) const; // check if the file exists in the specified pack
        bool fileExist(const std::string& file) const; // check if the file exists in the default pack
        bool fileExist(const PackPtr& ptr, const std::string& file) const; // check if the file exists in the pack ptr

        // garbage collection
        void trash(const std::string& pack, const std::string& file);
        void trash(const std::string& file);
        void trash(PackPtr& ptr, const std::string& file);
        void garbageCollector();
        bool restore(const std::string& pack, const std::string& file);
        bool restore(const std::string& file);
        bool restore(PackPtr& ptr, const std::string& file);

        // data manipulation
        RawPtr getData(const std::string& pack, const std::string& file); // return the shared ptr of the specified file in the pack
        RawPtr getData(const std::string& file); // same but with the default pack
        RawPtr getData(PackPtr& ptr, const std::string& file); // same but with the pack ptr
        DataContainer getDataContainer(const std::string& pack, const std::string& file); // same but return a copy of the data container (which has a shared ptr instance too)
        DataContainer getDataContainer(const std::string& file); // same but with the default pack
        DataContainer getDataContainer(PackPtr& ptr, const std::string& file); // same but with the pack ptr

        // encrypted file manipulation (pretty explicit)
        static bool cread(std::istream& f, char* c, Keys& seed);
        static std::streamsize cread(std::istream& f, char* buf, std::streamsize n, Keys& seed);
        static bool cwrite(std::ostream& f, char* c, Keys& seed);
        static std::streamsize cwrite(std::ostream& f, const char* buf, std::streamsize n, Keys& seed);
        // used if you need to go forward in the file without reading. CHANGE THIS if you changed the way the data is encrypted
        static void forwardKeys(Keys& seed, size_t n);
        // read the keys in the data pack (the stream must be positioned at the right emplacement)
        static Keys readKeys(std::istream& f);

    protected:
        // functions, internal use only
        static void clearPack(Pack &pack); // clear the content
        bool loadFile(std::shared_ptr<Pack> &pack, const std::string& pack_name, const std::string& file, std::function<bool(DataContainer)> callback); // the shared ptr isn't null checked
        bool fileExist(const std::shared_ptr<Pack> &pack, const std::string& file, const bool &disk) const; // the shared ptr isn't null checked
        void trash(std::shared_ptr<Pack> &pack, const std::string& file, const bool &disk); // the shared ptr isn't null checked
        bool restore(std::shared_ptr<Pack> &pack, const std::string& file, const bool &disk); // the shared ptr isn't null checked
        RawPtr getData(std::shared_ptr<Pack> &pack, const std::string& file); // the shared ptr isn't null checked
        DataContainer getDataContainer(std::shared_ptr<Pack> &pack, const std::string& file); // the shared ptr isn't null checked

        // members
        std::string def; // default pack name
        std::shared_ptr<Pack> def_pack; // default pack
        Root root; // contains all the pack
        std::set<DataContainer*> trashbin; // for the garbage collector
        std::set<DataContainer*> disk_trashbin; // for the garbage collector (on disk files)
        std::mutex mutex; // the used mutex
};

#endif // RESOURCEMANAGER_HPP
