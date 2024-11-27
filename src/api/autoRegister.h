/*

This class template is used to automatically register an API interface with the API_Builder class. 
Classes that implements I_API_Interface should inherit from this class template as well. 
This automatically registers the interface with the API_Builder class when the class is instantiated.

When the builder class has its start() method called, it will wait until all the interfaces have been
added to the builder before it starts building the API. This is to ensure that all the interfaces are
added before the API is built.

To make this work, the CMakeLists file defines a function that counts all the references to the 
AutoRegisterAPI class template in the code. This count is then used to determine how many interfaces
are expected to be added to the API. This count is set as #define NUM_INTERFACES using the -D flag.

In order to ensure that this is updated, it is necessary to re-run configure and generate the build files
again.

*/


#ifndef AUTOREGISTER_H
#define AUTOREGISTER_H



#endif// AUTOREGISTER_H
