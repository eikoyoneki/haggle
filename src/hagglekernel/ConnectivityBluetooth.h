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
#ifndef _CONNECTIVITYBLUETOOTH_H_
#define _CONNECTIVITYBLUETOOTH_H_

#include <libcpphaggle/Platform.h>

#if defined(ENABLE_BLUETOOTH)
#include <haggleutils.h>

/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/
class ConnectivityBluetooth;

#include "Connectivity.h"
#include "Interface.h"

// 593C0DD8-2399-4E97-AFD8-3828A781B89D
#define HAGGLE_BLUETOOTH_SDP_UUID \
	{ \
		0x59, 0x3C, 0x0D, 0xD8,	\
		0x23, 0x99, \
		0x4E, 0x97, \
		0xAF, 0xD8, \
		0x38, 0x28, 0xA7, 0x81, 0xB8, 0x9D \
	}

// All these are in seconds:
#define BASE_TIME_BETWEEN_SCANS	(120) //(80)
#define RANDOM_TIME_AMOUNT (60)

// The time to wait. (BASE_TIME_BETWEEN_SCANS +- RANDOM_TIME_AMOUNT)
#define TIME_TO_WAIT				\
	(BASE_TIME_BETWEEN_SCANS - RANDOM_TIME_AMOUNT + RANDOM_INT(2*RANDOM_TIME_AMOUNT))

#define TIME_TO_WAIT_MSECS (TIME_TO_WAIT * 1000)

#define _IN_CONNECTIVITYBLUETOOTH_H

#if defined(OS_LINUX)
#include "ConnectivityBluetoothLinux.h"
#elif defined(OS_MACOSX)
#include "ConnectivityBluetoothMacOSX.h"
#elif defined(OS_WINDOWS_DESKTOP)
#include "ConnectivityBluetoothWindowsXP.h"
#elif defined(OS_WINDOWS_MOBILE)
#if defined(WIDCOMM_BLUETOOTH)
#include "ConnectivityBluetoothWindowsMobileWIDCOMM.h"
#else
#include "ConnectivityBluetoothWindowsMobile.h"
#endif
#else
#error "Bad OS - Not supported by ConnectivityBluetooth.h"
#endif

#undef _IN_CONNECTIVITYBLUETOOTH_H

#endif /* ENABLE_BLUETOOTH */

#endif