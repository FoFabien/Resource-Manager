# Resource Manager  

Simple class to manage files.  
Files have to be packed preemptively into a data pack.  
Data packs have a minimalistic XOR encryption.  

# How-to  

  - Load a data pack.  
  - (Optional for big files) Load a file into memory from the data pack.  

# Streams  

  - 'resourcestream.cpp' and 'resourcestream.hpp' contain stream examples.  
  - MemoryStream allows you to stream a file already loaded in the memory.  
  - FileStream allows you to stream a file from the data pack on the disk (useful for big files).  