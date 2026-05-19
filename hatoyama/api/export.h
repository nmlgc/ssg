/*
 *   Export macros
 *
 */

#pragma once

#ifndef HATOYAMA_API
#ifdef HATOYAMA_API_LIB
#define HATOYAMA_API
#else
#ifdef _WIN32
#ifdef HATOYAMA_API_BUILD
#define HATOYAMA_API __declspec(dllexport)
#else
#define HATOYAMA_API __declspec(dllimport)
#endif
#else
#define HATOYAMA_API __attribute((visibility("default")))
#endif
#endif
#endif
