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
#ifndef _CONNECTIVITYBLUETOOTHLINUX_H_
#define _CONNECTIVITYBLUETOOTHLINUX_H_

#ifndef  _IN_CONNECTIVITYBLUETOOTH_H
#error "Do not include this file directly, include ConnectivityBluetooth.h"
#endif

#include <libcpphaggle/PlatformDetect.h>

#if defined(ENABLE_BLUETOOTH)
#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

/**
	Bluetooth connectivity module.

	Scans a bluetooth interface to find nearby haggle nodes. Reports any found
	to the connectivity manager.
*/
class ConnectivityBluetooth : public Connectivity
{
private:
	InterfaceRef rootInterface;
        uint32_t service;
	sdp_session_t *session;
        bool run();
        void hookCleanup();
        friend void bluetoothDiscovery(ConnectivityBluetooth *conn);
public:
	/**
	   Tells the connectivity to finish. The connectivity will not 
	   neccesarily have finished by the time cancelDiscovery() returns.
	   
	   This function is intended to be the exact same as 
	   Connectivity::cancelDiscovery(), but with one specific exception:
	   that it does NOT call Thread::cancel() either directly or 
	   indirectly, but instead signals the condition variable above.
	   
	   The reason for this is explained in haggle trac system, ticket #98.
	*/
	virtual void cancelDiscovery(void);
	virtual void handleInterfaceDown(const InterfaceRef &iface);
        ConnectivityBluetooth(ConnectivityManager *m, const InterfaceRef& _iface);
        ~ConnectivityBluetooth();
};
#endif

#endif