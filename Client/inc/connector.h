#ifndef _CONNECTOR_H
#define _CONNECTOR_H

#include "SessionClient.h"

SessionClient* initSession();
void releaseSession(SessionClient* session);

#endif