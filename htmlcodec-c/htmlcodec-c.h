// htmlcodec-c.h : Include file for standard system include files,
// or project specific include files.

#ifndef HTMLCODEC_C_H
#define HTMLCODEC_C_H

#include <stdio.h>
#include <stddef.h>

/* DLL import/export visibility macro.
 *
 *  HTMLCODEC_BUILDING_DLL — defined when compiling the shared library itself;
 *                           expands to dllexport / visibility("default").
 *  HTMLCODEC_STATIC       — defined (PUBLIC) by the static-library CMake target
 *                           and propagates to all targets that link it;
 *                           expands to nothing (plain C linkage).
 *  (neither)              — consumer of the shared library; expands to dllimport
 *                           on Windows so the linker uses the import stub.
 */
#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(HTMLCODEC_BUILDING_DLL)
#    define HTMLCODEC_API __declspec(dllexport)
#  elif defined(HTMLCODEC_STATIC)
#    define HTMLCODEC_API
#  else
#    define HTMLCODEC_API __declspec(dllimport)
#  endif
#elif defined(__GNUC__) || defined(__clang__)
#  define HTMLCODEC_API __attribute__((visibility("default")))
#else
#  define HTMLCODEC_API
#endif

#endif // HTMLCODEC_C_H
