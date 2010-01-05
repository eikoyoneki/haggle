/* Copyright 2008-2009 Uppsala University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _CONNECTIVITYLOCAL_H_
#define _CONNECTIVITYLOCAL_H_

/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/
#include <libcpphaggle/Platform.h>

#include "Interface.h"
#include "Connectivity.h"

#define _IN_CONNECTIVITYLOCAL_H

class ConnectivityLocal;

class ConnectivityLocalBase : public Connectivity {
    private:
        friend class ConnectivityLocal;
	InterfaceRef rootInterface;
        ConnectivityLocalBase(ConnectivityManager *m, const string& name);
        ~ConnectivityLocalBase();
};

#if defined(OS_ANDROID)
#include "ConnectivityLocalAndroid.h"
#elif defined(OS_LINUX)
#include "ConnectivityLocalLinux.h"
#elif defined(OS_MACOSX)
#include "ConnectivityLocalMacOSX.h"
#elif defined(OS_WINDOWS)
#if defined(OS_WINDOWS_MOBILE)
#include "ConnectivityLocalWindowsMobile.h"
#else
#include "ConnectivityLocalWindowsXP.h"
#endif
#else
#error "Bad OS - Not supported by ConnectivityLocal.h"
#endif
#undef _IN_CONNECTIVITYLOCAL_H

#endif
