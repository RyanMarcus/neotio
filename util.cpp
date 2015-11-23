// Copyright 2015 Ryan Marcus
// This file is part of neotio

#include "util.hpp"
#include <time.h>

void msleep(int milli) {
	struct timespec ts;
	ts.tv_sec = milli / 1000;
	ts.tv_nsec = (milli % 1000) * 1000000;
	nanosleep(&ts, NULL);
}

