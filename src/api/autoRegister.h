/*
 █████╗ ██╗   ██╗████████╗ ██████╗ ██████╗ ███████╗ ██████╗ ██╗███████╗████████╗███████╗██████╗    ██╗  ██╗
██╔══██╗██║   ██║╚══██╔══╝██╔═══██╗██╔══██╗██╔════╝██╔════╝ ██║██╔════╝╚══██╔══╝██╔════╝██╔══██╗   ██║  ██║
███████║██║   ██║   ██║   ██║   ██║██████╔╝█████╗  ██║  ███╗██║███████╗   ██║   █████╗  ██████╔╝   ███████║
██╔══██║██║   ██║   ██║   ██║   ██║██╔══██╗██╔══╝  ██║   ██║██║╚════██║   ██║   ██╔══╝  ██╔══██╗   ██╔══██║
██║  ██║╚██████╔╝   ██║   ╚██████╔╝██║  ██║███████╗╚██████╔╝██║███████║   ██║   ███████╗██║  ██║██╗██║  ██║
╚═╝  ╚═╝ ╚═════╝    ╚═╝    ╚═════╝ ╚═╝  ╚═╝╚══════╝ ╚═════╝ ╚═╝╚══════╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝╚═╝  ╚═╝
*/

/*
MIT License

Copyright (c) 2025 A-McD Technology LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

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
