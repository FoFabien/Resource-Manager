#include <iostream>

#include "resourcemanager.hpp"
#include "resourcestream.hpp"
#include <random>

int main()
{
    std::cout << "Hello world!" << std::endl;

    // building a new pack
    ResourceManager::saveResources("test.pack", {"main.cpp", "resourcemanager.cpp", "resourcemanager.hpp", "resourcestream.cpp", "resourcestream.hpp"});

    // instance of the manager
    ResourceManager rm;

    // loading the pack
    if(rm.loadResources("test.pack"))
        std::cout << "loading success\n";
    else std::cout << "loading failure\n";

    // pack demo
    PackPtr myPack;
    if(rm.getPackPtr("test.pack", myPack))
    {
        // creating a memory stream
        MemoryStream ms;
        if(ms.open("test.pack", "resourcemanager.hpp", rm)) // open
        {
            std::cout << "file success\n";
            std::string test;
            test.resize(ms.getSize());
            std::cout << "read " << ms.read(&test[0], test.size()) << " byte(s) / " << ms.getSize() << " byte(s)" << std::endl;
        }
        else std::cout << "file failure\n";
    }
    else std::cout << "pack not found\n";

    // disk demo 1
    FileStream fs;
    if(fs.open("", "test.pack", rm)) // open a file on disk
    {
        std::cout << "file success\n";
        char c;
        size_t count = 0;
        while(fs.read(&c, 1) == 1)
            ++count;
        std::cout << "read " << count << " byte(s) / " << fs.getSize() << " byte(s)" << std::endl;
    }
    else std::cout << "file failure\n";

    // disk demo 2
    MemoryStream ds;
    if(ds.open("", "main.cpp", rm)) // open a file on disk
    {
        std::cout << "file success\n";
        // 1
        std::string test;
        test.resize(ds.getSize());
        std::cout << "read " << ds.read(&test[0], test.size()) << " byte(s) / " << ds.getSize() << " byte(s)" << std::endl;
        // 2
        ds.seek(0);
        char c;
        size_t count = 0;
        while(ds.read(&c, 1) == 1)
            ++count;
        std::cout << "read " << count << " byte(s) / " << ds.getSize() << " byte(s)" << std::endl;
    }
    else std::cout << "file failure\n";
    return 0;
}
