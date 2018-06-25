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
        std::cout << "success\n";
    else std::cout << "failure\n";

    // creating a memory stream
    MemoryStream ms;
    if(ms.open("test.pack", "resourcemanager.hpp", rm)) // open
    {
        std::cout << "success\n";
        std::string test;
        test.resize(ms.getSize());
        ms.read(&test[0], test.size()); // read the data into the 'test' string
        std::cout << test << std::endl; // print
    }
    else std::cout << "failure\n";
    return 0;
}
