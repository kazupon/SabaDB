/*
 * debug utilities.
 * Copyright (C) 2012 kazuya kawaguchi & frapwings. See Copyright Notice in main.h
 */

#ifndef DEBUG_H
#define DEBUG_H


#include <stdio.h>


#if defined(NDEBUG) && NDEBUG
#define TRACE(fmt, ...)     ((void)0)
#else
#define TRACE(fmt, ...)     do { fprintf(stderr, "%s: %d: (%p) %s: " fmt, __FILE__, __LINE__, pthread_self(), __func__, ##__VA_ARGS__); } while (0)
#endif /* DEBUG */


#endif /* DEBUG_H */

