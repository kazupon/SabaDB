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


double saba_time();
int32_t saba_jetlag();
void saba_date_www_format(double t, int32_t jl, int32_t acr, char *buf);


#endif /* SABA_UTILS_H */

