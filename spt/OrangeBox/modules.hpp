#pragma once

#include "modules\EngineDLL.hpp"
#include "modules\ClientDLL.hpp"
#include "modules\ServerDLL.hpp"
#include "modules\vguimatsurfaceDLL.hpp"

extern EngineDLL engineDLL;
extern ClientDLL clientDLL;
extern ServerDLL serverDLL;
#ifndef OE
extern VGui_MatSurfaceDLL vgui_matsurfaceDLL;
#endif