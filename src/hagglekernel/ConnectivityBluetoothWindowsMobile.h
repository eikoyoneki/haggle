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
#ifndef _CONNECTIVITYBLUETOOTHWINDOWSMOBILE_H_
#define _CONNECTIVITYBLUETOOTHWINDOWSMOBILE_H_

#ifndef  _IN_CONNECTIVITYBLUETOOTH_H
#error "Do not include this file directly, include ConnectivityBluetooth.h"
#endif

#include <libcpphaggle/Platform.h>

#if defined(ENABLE_BLUETOOTH) && !defined(WIDCOMM_BLUETOOTH)

/**
	Bluetooth connectivity module.

	Scans a bluetooth interface to find nearby haggle nodes. Reports any found
	to the connectivity manager.
*/
class ConnectivityBluetooth : public ConnectivityBluetoothBase
{
private:
	InterfaceRef rootInterface;
        ULONG sdpRecordHandle;
        friend void bluetoothDiscovery(ConnectivityBluetooth *conn);
        bool run();
        void hookCleanup();
public:
	virtual void handleInterfaceDown(const InterfaceRef &iface);
        ConnectivityBluetooth(ConnectivityManager *m, const InterfaceRef& iface);
        ~ConnectivityBluetooth();
};
#endif

#endif
