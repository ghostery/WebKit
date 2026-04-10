#pragma once

#ifdef HAVE_CONFIG_H
#include "cmakeconfig.h"
#endif

#include <wtf/Platform.h>
#include <wtf/PlatformEnable.h>
#include <wtf/PlatformHave.h>
#include <wtf/PlatformUse.h>

#undef ENABLE_CONTENT_EXTENSIONS
#define ENABLE_CONTENT_EXTENSIONS 1

#ifndef WEBCORE_EXPORT
#define WEBCORE_EXPORT
#endif
