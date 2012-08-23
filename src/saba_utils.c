/*
 * SabaDB: utilities
 * Copyright (C) 2012 kazuya kawaguchi & frapwings. See Copyright Notice in main.h
 */

#include "saba_utils.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>


double saba_time() {
  struct timeval tv;
  if (gettimeofday(&tv, NULL) != 0) {
    return 0.0;
  }
  return tv.tv_sec + tv.tv_usec / 1000000.0;
}

int32_t saba_jetlag() {
  time_t t = 86400;
  struct tm *gts = gmtime(&t);
  if (gts == NULL) {
    return 0;
  }
  t = 86400;
  struct tm *lts = localtime(&t);
  if (lts == NULL) {
    return 0;
  }
  return mktime(lts) - mktime(gts);
}

void saba_date_www_format(double t, int32_t jl, int32_t acr, char *buf) {
  if (isnan(t)) {
    t = saba_time();
  }

  double tinteg, tfract;
  tfract = fabs(modf(t, &tinteg));
  if (jl == INT32_MAX) {
    jl = saba_jetlag();
  }

  if (acr > 12) {
    acr = 12;
  }

  time_t tt = (time_t)tinteg + jl;
  struct tm wts;
  struct tm *ts;
  ts = gmtime(&tt);
  if (ts == NULL) {
    memset(&wts, 0, sizeof(wts));
    ts = &wts;
  }

  ts->tm_year += 1900;
  ts->tm_mon += 1;
  jl /= 60;

  char tzone[16];
  if (jl == 0) {
    sprintf(tzone, "Z");
  } else if (jl < 0) {
    jl *= -1;
    sprintf(tzone, "-%02d:%02d", jl / 60, jl % 60);
  } else {
    sprintf(tzone, "+%02d:%02d", jl / 60, jl % 60);
  }

  if (acr < 1) {
    sprintf(
      buf, "%04d-%02d-%02dT%02d:%02d:%02d%s",
      ts->tm_year, ts->tm_mon, ts->tm_mday, ts->tm_hour, ts->tm_min, ts->tm_sec, tzone
    );
  } else {
    char dec[16];
    sprintf(dec, "%.12f", tfract);
    char* wp = dec;
    if (*wp == '0') {
      wp++;
    }
    wp[acr + 1] = '\0';
    sprintf(
      buf, "%04d-%02d-%02dT%02d:%02d:%02d%s%s",
      ts->tm_year, ts->tm_mon, ts->tm_mday, ts->tm_hour, ts->tm_min, ts->tm_sec, wp, tzone
    );
  }
}

