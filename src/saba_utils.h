/*
 * SabaDB: utility module
 * Copyright (C) 2012 kazuya kawaguchi & frapwings. See Copyright Notice in main.h
 */

#ifndef SABA_UTILS_H
#define SABA_UTILS_H


#include <inttypes.h>
#include <stddef.h>


#define ARRAY_SIZE(a)   (sizeof(a) / sizeof( (a)[0] ))

#define container_of(ptr, type, member) \
    ( (type *)( (char *)(ptr) - offsetof(type, member) ) )


#endif /* SABA_UTILS_H */

