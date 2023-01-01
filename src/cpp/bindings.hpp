#ifndef _BINDINGS_HPP_
#define _BINDINGS_HPP_

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif


#endif /* _BINDINGS_HPP_ */
