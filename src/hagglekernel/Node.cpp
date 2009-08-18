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
#include <string.h>
#include <libcpphaggle/Platform.h>
#include <libcpphaggle/Thread.h>
#include <openssl/sha.h>
#include <base64.h>

#ifdef DEBUG
#include <stdio.h>
#endif

#include "Node.h"
#include "Interface.h"
#include "Attribute.h"
#include "DataObject.h"
#include "Event.h"
#include "Filter.h"
#include "Utility.h"
#include "Trace.h"
#include "XMLMetadata.h"

using namespace haggle;

#if defined(OS_WINDOWS_MOBILE)
#include <GetDeviceUniqueId.h>
#elif defined(OS_WINDOWS_DESKTOP)
#include <iphlpapi.h>
#elif defined(OS_UNIX)
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
//#include <net/ethernet.h>
#include <arpa/inet.h>

#ifdef OS_MACOSX
#include <net/if_dl.h>
#endif

#endif /* OS_WINDOWS_MOBILE */

unsigned long Node::totNum = 0;
const char *Node::typestr[] = {
	"undefined",
	"this node",
	"application",
	"peer",
	"gateway",
	NULL
};

inline bool Node::init_node(const char *_id)
{
	memset(id, 0, NODE_ID_LEN);
	memset(idStr, 0, MAX_NODE_ID_STR_LEN);
	
	if(!doBF)
	{
		/* Init Bloomfilter */
		doBF = 
			new Bloomfilter(
#ifdef OS_WINDOWS_DESKTOP
				(float)
#endif
				0.01, 
				MAX_RECV_DATAOBJECTS);
	}

        if (createdFromNodeDescription) {
                struct base64_decode_context b64_ctx;
                size_t decodelen;
                const char *pval;

                Metadata *nm = dObj->getMetadata()->getMetadata(NODE_METADATA);
                
                if (!nm)
                        return false;
                
                pval = nm->getParameter(NODE_METADATA_ID_PARAM);
                
                if (pval) {
                        decodelen = NODE_ID_LEN;
                        base64_decode_ctx_init(&b64_ctx);
                        
                        if (!base64_decode(&b64_ctx, pval, strlen(pval), (char *)id, &decodelen))
                                return false;
                        
                        calcIdStr();
                }
                
                pval = nm->getParameter(NODE_METADATA_NAME_PARAM);
                
                if (pval)
                        name = pval;
                
                pval = nm->getParameter(NODE_METADATA_THRESHOLD_PARAM);
                
                if (pval)
					matchThreshold = strtol(pval, NULL, 10);
                
                pval = nm->getParameter(NODE_METADATA_MAX_DATAOBJECTS_PARAM);
                
                if (pval)
					numberOfDataObjectsPerMatch = strtol(pval, NULL, 10);
				
                Metadata *bm = nm->getMetadata(NODE_METADATA_BLOOMFILTER);
                
                if (bm) {
                        doBF->fromBase64(bm->getContent());
                }

                Metadata *im = nm->getMetadata(NODE_METADATA_INTERFACE);
                
                // A node without interfaces should not really be valid
                if (!im)
                        return false;
                
                while (im) {
                        Addresses addrs;
                        char *identifier = NULL;
                        const char *type;
                        
                        base64_decode_ctx_init(&b64_ctx);

                        type = im->getParameter(NODE_METADATA_INTERFACE_TYPE_PARAM);

                        if (!type)
                                return false;

                        pval = im->getParameter(NODE_METADATA_INTERFACE_IDENTIFIER_PARAM);

                        if (!pval || !base64_decode_alloc(&b64_ctx, pval, strlen(pval), &identifier, &decodelen))
                                return false;

                        Metadata *am = im->getMetadata(NODE_METADATA_INTERFACE_ADDRESS);

                        while (am) {
                                addrs.push_back(new Address(am->getContent().c_str()));
                                am = im->getNextMetadata();
                        }

                        addInterface(new Interface(Interface::strToType(type), identifier, &addrs));

                        free(identifier);

                        im = nm->getNextMetadata();
                }
        }
	if (type == NODE_TYPE_THIS_NODE) {
		calcId();
	} else if (type == NODE_TYPE_UNDEF) {
		if (_id) {
			HAGGLE_DBG("Attempted to create undefined node with ID. ID ignored.\n");
		}
		strncpy(idStr, "[Not yet set]", MAX_NODE_ID_STR_LEN);
	} else if (_id) {
		memcpy(id, _id, NODE_ID_LEN);
		calcIdStr();
	} else if (!createdFromNodeDescription) {
		HAGGLE_ERR("Created node without node ID.\n");
	}

	if (!doBF)
                return false;

        return true;
}

Node::Node(const NodeType_t _type, const DataObjectRef& dObj) : 
#ifdef DEBUG_LEAKS
                LeakMonitor(LEAK_TYPE_NODE),
#endif
                type(_type), num(totNum++), name("Unnamed node"), nodeDescExch(false), 
                dObj(dObj ? dObj : DataObjectRef(new DataObject())), 
                doBF(NULL), stored(false), createdFromNodeDescription(dObj ? true : false), 
                filterEventId(-1),
				matchThreshold(0),
				numberOfDataObjectsPerMatch(0)
{
	init_node(NULL);
}

Node::Node(const NodeType_t _type, const string& _name) : 
#ifdef DEBUG_LEAKS
                LeakMonitor(LEAK_TYPE_NODE),
#endif
                type(_type), num(totNum++), name(_name), nodeDescExch(false), 
                dObj(DataObjectRef(new DataObject())), doBF(NULL), stored(false), 
                createdFromNodeDescription(false), filterEventId(-1),
				matchThreshold(0),
				numberOfDataObjectsPerMatch(0)
{
	init_node(NULL);
}

Node::Node(const NodeType_t _type, const char *_id, const string& _name) : 
#ifdef DEBUG_LEAKS
                LeakMonitor(LEAK_TYPE_NODE),
#endif
                type(_type), num(totNum++), name(_name), nodeDescExch(false), 
                dObj(DataObjectRef(new DataObject())), doBF(NULL), 
                stored(false), createdFromNodeDescription(false),
                filterEventId(-1),
				matchThreshold(0),
				numberOfDataObjectsPerMatch(0)
{
	init_node(_id);
}

Node::Node(const char *_idStr, const NodeType_t _type, const string& _name) : 
#ifdef DEBUG_LEAKS
                LeakMonitor(LEAK_TYPE_NODE),
#endif
                type(_type), num(totNum++), name(_name), nodeDescExch(false), 
                dObj(DataObjectRef(new DataObject())), doBF(NULL), 
                stored(false), createdFromNodeDescription(false),
                filterEventId(-1),
				matchThreshold(0),
				numberOfDataObjectsPerMatch(0)
{
	char iD[NODE_ID_LEN];
	long i;
	
	for (i = 0; i < NODE_ID_LEN; i++) {
		iD[i] = 0;
		if(_idStr[i*2] <= '9')
			iD[i] += (_idStr[i*2] - '0') << 4;
		else if(_idStr[i*2] <= 'G')
			iD[i] += (_idStr[i*2] - 'A' + 10) << 4;
		else //if(_idStr[i*2] <= 'g')
			iD[i] += (_idStr[i*2] - 'a' + 10) << 4;
		if(_idStr[i*2 + 1] <= '9')
			iD[i] += (_idStr[i*2 + 1] - '0');
		else if(_idStr[i*2 + 1] <= 'G')
			iD[i] += (_idStr[i*2 + 1] - 'A' + 10);
		else //if(_idStr[i*2 + 1] <= 'g')
			iD[i] += (_idStr[i*2 + 1] - 'a' + 10);
	}
	
	init_node(iD);
}


Node::Node(const Node & n) :
#ifdef DEBUG_LEAKS
                LeakMonitor(LEAK_TYPE_NODE),
#endif
                type(n.type), num(totNum), name(n.name), 
                nodeDescExch(n.nodeDescExch), 
                dObj(NULL), interfaces(n.interfaces), 
                doBF(new Bloomfilter(*n.doBF)), 
                stored(n.stored), createdFromNodeDescription(n.createdFromNodeDescription),
                filterEventId(n.filterEventId),
				matchThreshold(n.matchThreshold),
				numberOfDataObjectsPerMatch(n.numberOfDataObjectsPerMatch)
{
	memcpy(id, n.id, NODE_ID_LEN);
	strncpy(idStr, n.idStr, MAX_NODE_ID_STR_LEN);

	if (n.dObj)
		dObj = DataObjectRef(n.dObj->copy());
}

Node::~Node()
{
    	while (eventInterests.size()) {
		Pair<long, long> p = *eventInterests.begin();

		if (EVENT_TYPE_PRIVATE(p.second)) 
			Event::unregisterType(p.second);

		eventInterests.erase(p.first);
	}
	if(doBF != NULL)
		delete doBF;
}

const NodeType_t Node::getType() const
{
	return type;
}

const char *Node::typeToStr(const NodeType_t type)
{
        return typestr[type];
}

const unsigned char *Node::getId() const
{
	return id;
}

void Node::setId(char *_id)
{
	memcpy(id, _id, NODE_ID_LEN);
	calcIdStr();
}

const char *Node::getIdStr() const
{
	return idStr;
}

string Node::getName() const
{
	return name;
}

void Node::setName(const string _name)
{
	name = _name;
}

Metadata *Node::toMetadata(bool withBloomfilter) const
{
	char *b64str = NULL;
	int b64len = 0;

        Metadata *nm = new XMLMetadata(NODE_METADATA);
        
        if (!nm)
                return NULL;
        
	b64len = base64_encode_alloc((const char *)getId(), NODE_ID_LEN, &b64str);

        nm->setParameter(NODE_METADATA_ID_PARAM, b64str);

	free(b64str);

        nm->setParameter(NODE_METADATA_NAME_PARAM, name.c_str());
		
        nm->setParameter(NODE_METADATA_THRESHOLD_PARAM, matchThreshold);

        nm->setParameter(NODE_METADATA_MAX_DATAOBJECTS_PARAM, numberOfDataObjectsPerMatch);

        for (InterfaceRefList::const_iterator it = interfaces.begin(); it != interfaces.end(); it++) {

                Metadata *im = nm->addMetadata(NODE_METADATA_INTERFACE); 
                
                if (!im)
                        return NULL;

                im->setParameter(NODE_METADATA_INTERFACE_TYPE_PARAM, (*it)->getTypeStr());

                b64len = base64_encode_alloc((const char *)(*it)->getRawIdentifier(), (*it)->getIdentifierLen(), &b64str);
                
                im->setParameter(NODE_METADATA_INTERFACE_IDENTIFIER_PARAM, b64str);
                
                free(b64str);

                const Addresses *adds = (*it)->getAddresses();

		for (Addresses::const_iterator jt = adds->begin(); jt != adds->end(); jt++) {
                        im->addMetadata(NODE_METADATA_INTERFACE_ADDRESS, (*jt)->getURI());
		}
        }

        if (withBloomfilter)
                nm->addMetadata(NODE_METADATA_BLOOMFILTER, doBF->toBase64());

        return nm;        
}

int Node::addAttribute(const Attribute & a)
{
	return dObj->addAttribute(a);
}

int Node::addAttribute(const string name, const string value, const unsigned long weight)
{
  	return dObj->addAttribute(name, value, weight);
}

int Node::removeAttribute(const Attribute & a)
{
	return dObj->removeAttribute(a);
}

int Node::removeAttribute(const string name, const string value)
{
	return (dObj->removeAttribute(name, value));
}

const Attribute *Node::getAttribute(const string name, const string value, const unsigned int n)
{
	return dObj->getAttribute(name, value, n);
}

const Attributes *Node::getAttributes()
{
	return dObj->getAttributes();
}

const unsigned char *Node::strIdToRaw(const char *strId)
{
	static unsigned char id[NODE_ID_LEN];
	int i = 0, j = 0;

	while (i < MAX_NODE_ID_STR_LEN && j < NODE_ID_LEN) {
		char hexByteStr[3] = { strId[i], strId[i + 1], '\0' };

		id[j++] = (unsigned char) strtol(hexByteStr, NULL, 16);
		i += 2;
	}
	return id;
}

#if defined(OS_MACOSX) && !defined(OS_MACOSX_IPHONE)
#include <CoreFoundation/CoreFoundation.h> 

#include <IOKit/IOKitLib.h> 
#include <IOKit/network/IOEthernetInterface.h> 
#include <IOKit/network/IONetworkInterface.h> 
#include <IOKit/network/IOEthernetController.h> 

static void getserial(char *serialNumber) 
{ 
        kern_return_t kernResult; 
        mach_port_t machPort; 

        kernResult = IOMasterPort( MACH_PORT_NULL, &machPort ); 

        serialNumber[0] = 0; 

        // if we got the master port 
        if ( kernResult == KERN_SUCCESS ) { 

                // create a dictionary matching IOPlatformExpertDevice 
                CFMutableDictionaryRef classesToMatch = IOServiceMatching("IOPlatformExpertDevice" ); 

                // if we are successful 
                if (classesToMatch) { 

                        // get the matching services iterator 
                        io_iterator_t iterator; 
                        kernResult = IOServiceGetMatchingServices( machPort, classesToMatch, &iterator ); 

                        // if we succeeded 
                        if ( (kernResult == KERN_SUCCESS) && iterator ) { 

                                io_object_t serviceObj; 
                                bool done = false; 
                                do { 
                                        // get the next item out of the dictionary 
                                        serviceObj = IOIteratorNext( iterator ); 

                                        // if it is not NULL 
                                        if (serviceObj) { 

                                                CFDataRef data = (CFDataRef) IORegistryEntryCreateCFProperty(serviceObj, CFSTR("serial-number" ), kCFAllocatorDefault, 0 ); 

                                                if (data != NULL) { 
                                                        CFIndex datalen = CFDataGetLength(data); 
                                                        const UInt8* rawdata = CFDataGetBytePtr(data); 
                                                        char dataBuffer[256]; 
                                                        memcpy(dataBuffer, rawdata, datalen); 
                                                        sprintf(serialNumber, "%s%s", dataBuffer+13, dataBuffer); 
                                                        CFRelease(data); 
                                                        done = true; 
                                                } 
                                        } 
                                } while (serviceObj && !done); 
                                IOObjectRelease(serviceObj); 
                        } 

                        IOObjectRelease(iterator); 
                } 
        } 
}
#endif

/**

   Calculate a unique node Id which will be consistent each time.
*/
void Node::calcId()
{
	HAGGLE_DBG("Updating Id for node type=%d\n", type);

#if defined(OS_WINDOWS_MOBILE)
#define APPLICATION_DATA "@^!Haggle!^@"

	HRESULT hr = NOERROR;
	DWORD idlen = sizeof(id);

	hr = GetDeviceUniqueID(reinterpret_cast<PBYTE>(APPLICATION_DATA), 
		strlen(APPLICATION_DATA), 
		GETDEVICEUNIQUEID_V1,
		(LPBYTE)id,
		&idlen);

#elif defined(OS_LINUX) || defined(OS_MACOSX_IPHONE) || defined(OS_WINDOWS_DESKTOP)
        InterfaceRefList iflist;
	SHA_CTX ctx;
	SHA1_Init(&ctx);

        if (getLocalInterfaceList(iflist, false) > 0) {
                for (InterfaceRefList::iterator it = iflist.begin(); it != iflist.end(); ++it) {
                        SHA1_Update(&ctx, (unsigned char *)(*it)->getRawIdentifier(), (*it)->getIdentifierLen());
                        //printf("Found local interface %s\n", (*it)->getIdentifierStr());
                }
        }

	SHA1_Final((unsigned char *)id, &ctx);

#elif defined(OS_MACOSX)
	SHA_CTX ctx;
	char serial[256];
	
	SHA1_Init(&ctx);
	getserial(serial);
	SHA1_Update(&ctx, (unsigned char *)serial, strlen(serial));
	SHA1_Final((unsigned char *)id, &ctx);
#else
#error "Unsupported OS!"
#endif

	calcIdStr();
}

void Node::calcIdStr()
{
	int len = 0;

	// Generate a readable string of the Id
	for (int i = 0; i < NODE_ID_LEN; i++) {
		len += sprintf(idStr + len, "%02x", id[i] & 0xff);
	}
}

bool Node::setFilterEvent(long feid) 
{ 
	if (filterEventId != -1)
		return false; 
	
	filterEventId = feid;

	return true;
}

bool Node::addEventInterest(long type)
{
	return eventInterests.insert(make_pair(type, '\0')).second;
}

void Node::removeEventInterest(long type)
{
	eventInterests.erase(type);

	if (EVENT_TYPE_PRIVATE(type)) 
		Event::unregisterType(type);
}

bool Node::hasEventInterest(long type)
{
	return eventInterests.find(type) != eventInterests.end();
}

#ifdef DEBUG
void Node::printInterfaces()
{
	int n = 0;
	
	for (InterfaceRefList::iterator it = interfaces.begin(); it != interfaces.end(); it++)  {
		const InterfaceRef iface = *it;
		iface.lock();
		const Addresses *addrs = iface->getAddresses();

		printf("%d : %s %s\n", n, iface->getIdentifierStr(), iface->isUp()? "up" : "down");
		
		for (Addresses::const_iterator itt = addrs->begin(); itt != addrs->end(); itt++) {
			const Address *addr = *itt;
			printf("\t%s\n", addr->getURI());
		}
		iface.unlock();
 	}
}
#endif

bool Node::addInterface(InterfaceRef inIfaceRef)
{
	for (InterfaceRefList::iterator it = interfaces.begin(); it != interfaces.end(); it++) {
		InterfaceRef ifaceRef = *it;
		if (inIfaceRef == ifaceRef) {
			// Node already has interface - ignore.
			return false;
		}
	}

	interfaces.add(inIfaceRef);
	setCreateTime();
	return true;
}

// Mark an interface as up
bool Node::setInterfaceUp(InterfaceRef inIfaceRef)
{
	for (InterfaceRefList::iterator it = interfaces.begin(); it != interfaces.end(); it++) {
		InterfaceRef ifaceRef = *it;
		if (inIfaceRef == ifaceRef && !ifaceRef->isUp()) {
			ifaceRef->up();
			setCreateTime();
			return true;
		}
	}
	return false;
}

// Mark an interface as down.
bool Node::setInterfaceDown(InterfaceRef inIfaceRef)
{
	for (InterfaceRefList::iterator it = interfaces.begin(); it != interfaces.end(); it++) {
		InterfaceRef ifaceRef = *it;
		if (inIfaceRef == ifaceRef && ifaceRef->isUp()) {
			ifaceRef->down();
			setCreateTime();
			return true;
		}
	}
	return false;
}

bool Node::removeInterface(InterfaceRef inIfaceRef)
{
	for (InterfaceRefList::iterator it = interfaces.begin(); it != interfaces.end(); it++) {
		InterfaceRef ifaceRef = *it;
		if (inIfaceRef == ifaceRef) {
			if (ifaceRef->isUp()) {
				ifaceRef->down();
			}

			interfaces.erase(it);
			setCreateTime();
			return true;
		}
	}
	return false;
}

const InterfaceRefList *Node::getInterfaces() const 
{
	return &interfaces;
} 

unsigned int Node::numActiveInterfaces()
{
        unsigned int num = 0;

        for (InterfaceRefList::const_iterator it = interfaces.begin(); it != interfaces.end(); it++) {
		InterfaceRef iface = *it;
		if (iface->isUp()) {
			num++;
		}
	}
	return num;
}

bool Node::hasInterface(const InterfaceRef inIfaceRef) 
{
	for (InterfaceRefList::iterator it = interfaces.begin(); it != interfaces.end(); it++) {
		InterfaceRef ifaceRef = *it;
		
		if (inIfaceRef == ifaceRef) {
			return true;
		}
	}
	return false;
}
bool Node::isAvailable() const
{
	for (InterfaceRefList::const_iterator it = interfaces.begin(); it != interfaces.end(); it++) {
		InterfaceRef iface = *it;
		if (iface->isUp()) {
			return true;
		}
	}
	return false;
}

bool Node::isNeighbor() const
{
	return ((type == NODE_TYPE_PEER) || (type == NODE_TYPE_UNDEF)) && isAvailable();
}

DataObjectRef Node::getDataObject(bool withBloomfilter) const
{
        // If the node was created from a node description (i.e., the
        // node description was received over the network), then
        // return it the node description as is.
        if (createdFromNodeDescription)
                return dObj;

        // Else generate a node description with our data object with
        // interests as a basis
        DataObjectRef ndObj = dObj->copy();
        
        // Add the node description attribute
        ndObj->addAttribute(NODE_DESC_ATTR, getIdStr());

        Metadata *m = ndObj->getMetadata();

        if (!m)
                return NULL;

        if (!m->addMetadata(toMetadata(withBloomfilter)))
                return NULL;
                    
	if (type == NODE_TYPE_THIS_NODE)
		ndObj->setIsThisNodeDescription(true);
	
        return ndObj;
}

Bloomfilter *Node::getBloomfilter(void)
{
	return doBF;
}

void Node::setCreateTime(Timeval t)
{
	if (dObj->isThisNodeDescription())
		dObj->setCreateTime(t);
}

Timeval Node::getCreateTime() const
{
	return dObj->getCreateTime();
}

bool operator==(const Node &n1, const Node &n2)
{
	if (n1.type == NODE_TYPE_UNDEF || n2.type == NODE_TYPE_UNDEF) {
		// Check if the interfaces overlap:
		
		for (InterfaceRefList::const_iterator it = n1.interfaces.begin(); it != n1.interfaces.end(); it++) {
			for (InterfaceRefList::const_iterator jt = n2.interfaces.begin(); jt != n2.interfaces.end(); jt++) {
				if (*it == *jt)
					return true;
			}
		}
		return false;
	} else {
		return (memcmp(n1.id, n2.id, NODE_ID_LEN) == 0);
	}
}

bool operator!=(const Node & n1, const Node & n2)
{
	return (!(n1 == n2));
}

// ostream & operator<<(std::ostream& os, const Node& n)
// {
// 	os << "Node with node id: " << n.getIdStr() << endl;
// 	return os;
// }