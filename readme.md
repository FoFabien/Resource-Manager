# Resource Manager  

Simple C++11 class to manage files.  
Files have to be packed preemptively into a data pack.  
Data packs have a minimalistic XOR encryption.  

# How-to  

  - Load a data pack.  
  - (for small files) Load a file into memory from the data pack.  
  
  - You can also load non-packed files from the disk by calling the pack "" (still experimental)  
  - A basic garbage collector is available, you can mark a file with trash() and it will be cleaned out by garbageCollector() once the resource is unused.  
  - loadFile() supports callback functions, to directly process the data after a loading (lambda should work too).  

# Streams  

  - 'resourcestream.cpp' and 'resourcestream.hpp' contain stream examples.  
  - MemoryStream allows you to stream a file already loaded in the memory.  
  - FileStream allows you to stream a file from the data pack on the disk (useful for big files).  