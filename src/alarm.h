#pragma once
#include "linklayer.h"

extern int alarmOff;

void alarmHandler(int signal);
void installAlarm();
void stopAlarm();

int resend;
int retries;
int abort_send;
