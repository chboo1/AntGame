#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <deque>
#include <iostream>
#include <cstdint>
#include <string>
#include <thread>
#include <chrono>
#include <stddef.h>
#include <climits>


#include "headers/sockets.hpp"
#include "headers/network.hpp"
#include "headers/map.hpp"


struct PosObject
{
    PyObject_HEAD
    DPos p;
};


static PyMemberDef Pos_members[] = {
    {"x", Py_T_DOUBLE, offsetof(PosObject, p.x), 0, "X position"},
    {"y", Py_T_DOUBLE, offsetof(PosObject, p.y), 0, "Y position"},
    {nullptr}
};


static PyTypeObject PosType = {
    .ob_base = PyVarObject_HEAD_INIT(nullptr, 0)
    .tp_name = "AntGame.Pos",
    .tp_basicsize = sizeof(PosObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    //TODO
    .tp_doc = PyDoc_STR("Placeholder doc string"),
    .tp_members = Pos_members,
    .tp_new = PyType_GenericNew,
};


struct AntGameClientObject
{
    PyObject_HEAD
    std::deque<ConnectionManager::RequestID> reqIDs;
    std::deque<ConnectionManager::RequestID> cmdIDs;
    std::deque<unsigned int> cmdpids;
    std::deque<std::uint64_t> cmdargs;
    std::string recvData = "";

    Map*map = nullptr;
    Connection*conn = nullptr;

    unsigned char selfNestID = 0xff;
    std::string name = "";
    std::string address = "127.0.0.1";
    int port = ANTNET_DEFAULT_PORT;

    PyObject*onStart;
    PyObject*onFrame;
    PyObject*onNewAnt;
    PyObject*onGrab;
    PyObject*onHome;
    PyObject*onAttacked;
    PyObject*onHit;
    PyObject*onDeath;
};





static void AntGameClient_dealloc(PyObject* op)
{
    AntGameClientObject*self = (AntGameClientObject*)op;
    if (self->map)
    {
        delete self->map;
    }
    if (self->conn)
    {
        self->conn->finish();
        delete self->conn;
    }
    Py_TYPE(self)->tp_free(self);
}


static PyObject* AntGameClient_new(PyTypeObject* type, PyObject* args, PyObject *kwds)
{
    AntGameClientObject* self;
    self = (AntGameClientObject*)type->tp_alloc(type, 0);
    if (self != nullptr)
    {
        self->map = nullptr;
        self->conn = nullptr;
        self->selfNestID = 0xff;
        self->name = "";
        self->address = "127.0.0.1";
        self->port = ANTNET_DEFAULT_PORT;
        self->reqIDs.clear();
        self->cmdIDs.clear();
        self->cmdpids.clear();
        self->cmdargs.clear();
        self->recvData = "";
        self->onStart = nullptr;
        self->onFrame = nullptr;
        self->onNewAnt = nullptr;
        self->onGrab = nullptr;
        self->onHome = nullptr;
        self->onAttacked = nullptr;
        self->onHit = nullptr;
        self->onDeath = nullptr;
    }
    return (PyObject*)self;
}


static PyObject* AntGameClient_getnests(PyObject* op, void *closure);
static PyObject* AntGameClient_getants(PyObject* op, void *closure);
static PyObject* AntGameClient_getmeid(PyObject* op, void* closure)
{
    AntGameClientObject*self = (AntGameClientObject*)op;
    if (self->map == nullptr || self->selfNestID == 0xff || self->conn == nullptr || !self->conn->connected())
    {
        std::cerr << self->map << ", " << (unsigned int)self->selfNestID << ", " << self->conn << ", " << (self->conn ? self->conn->connected() : false) << std::endl;
        PyErr_SetString(PyExc_RuntimeWarning, "Your client is not connected! Connect first to get meid.");
        return nullptr;
    }
    return PyLong_FromUInt32((std::uint32_t)self->selfNestID);
}
static PyObject* AntGameClient_getme(PyObject* op, void* closure);


static PyObject* AntGameClient_getaddr(PyObject* op, void* closure)
{
    AntGameClientObject*self = (AntGameClientObject*)op;
    return PyUnicode_FromString(self->address.c_str());
}


static int AntGameClient_setaddr(PyObject* op, PyObject* value, void* closure)
{
    AntGameClientObject*self = (AntGameClientObject*)op;
    if (!value)
    {
        self->address.clear();
        return 0;
    }
    if (!PyUnicode_Check(value))
    {
        PyErr_SetString(PyExc_TypeError, "Cannot assign the address to something other than a string (use an address like \"X.X.X.X\"");
        return -1;
    }
    if (!PyUnicode_IS_ASCII(value))
    {
        PyErr_SetString(PyExc_ValueError, "Cannot assign the address to something with special characters in it. (Should only be digits and '.')");
        return -1;
    }
    self->address = PyUnicode_AsUTF8AndSize(value, nullptr);
    return 0;
}


static PyObject* AntGameClient_getname(PyObject* op, void* closure)
{
    AntGameClientObject*self = (AntGameClientObject*)op;
    return PyUnicode_FromString(self->name.c_str());
}


static int AntGameClient_setname(PyObject* op, PyObject* value, void* closure)
{
    AntGameClientObject*self = (AntGameClientObject*)op;
    if (!value)
    {
        self->name.clear();
        return 0;
    }
    if (!PyUnicode_Check(value))
    {
        PyErr_SetString(PyExc_TypeError, "Cannot assign the player name to something other than a string (something \"Really cool bot!\"");
        return -1;
    }
    self->name = PyUnicode_AsUTF8AndSize(value, nullptr);
    return 0;
}


static PyObject* AntGameClient_getmapsize(PyObject* op, void* closure)
{
    AntGameClientObject*self = (AntGameClientObject*)op;
    PosObject*posobj = (PosObject*)PosType.tp_alloc(&PosType, 0);
    if (posobj)
    {
        if (!self->map)
        {
            posobj->p.x = 0;
            posobj->p.y = 0;
        }
        else
        {
            posobj->p = self->map->size;
        }
    }
    Py_INCREF(posobj);
    return (PyObject*)posobj;
}


static PyGetSetDef AntGameClient_getsetters[] = {
    {"nests", AntGameClient_getnests, nullptr, "map's nests", nullptr},
    {"ants", AntGameClient_getants, nullptr, "map's ants", nullptr},
    {"meID", AntGameClient_getmeid, nullptr, "this client's ID", nullptr},
    {"me", AntGameClient_getme, nullptr, "this client's nest", nullptr},
    {"addr", AntGameClient_getaddr, AntGameClient_setaddr, "server address", nullptr},
    {"name", AntGameClient_getname, AntGameClient_setname, "player name", nullptr},
    {"mapsize", AntGameClient_getmapsize, nullptr, "map size", nullptr},
    {nullptr}
};


static PyMemberDef AntGameClient_members[] = {
    {"port", Py_T_INT, offsetof(AntGameClientObject, port), 0, "remote port"},
    {nullptr}
};


static void AntGameClient_handleConnErrors(Connection* conn)
{
    switch (conn->errorState)
    {
        case Connection::OK:
            PyErr_SetString(PyExc_OSError, "Failed a network operation, but I'm being lied to so I don't know what the error is.");
            break;
        case Connection::EARLY:
            PyErr_SetString(PyExc_ConnectionError, "Failed a network operation because we were disconnected at a previous point.");
            break;
        case Connection::SYS:
            PyErr_SetString(PyExc_OSError, "Failed a network operation because of an internal system error.");
            break;
        case Connection::NET:
            PyErr_SetString(PyExc_TimeoutError, "Failed a network operation because the network is down. Make sure your internet is up.");
            break;
        case Connection::CLOSED:
            PyErr_SetString(PyExc_ConnectionError, "Failed a network operation because the server kicked us out.");
            break;
        case Connection::UNKNOWN:
        default:
            PyErr_SetString(PyExc_OSError, "Failed a network operation because of an unknown reason.");
            break;
    }
}


static std::string AntGameClient_readallWait(Connection* conn)
{
    std::string retval = "";
    char first = '\0';
    int val = 0;
    while (val < 1)
    {
        val = conn->receive(&first, 1);
        if (val < 0)
        {
            AntGameClient_handleConnErrors(conn);
            return "";
        }
    }
    retval.push_back(first);
    retval.append(conn->readall());
    if (conn->errorState != Connection::OK)
    {
        AntGameClient_handleConnErrors(conn);
        return "";
    }
    return retval;
}


static void AntGameClient_cleanup(AntGameClientObject* self)
{
    if (self->conn)
    {
        if (self->conn->connected())
        {
            self->conn->send("\0\0\0\x09\0\0\0\x01\x02", 9);
        }
        self->conn->finish();
        delete self->conn;
        self->conn = nullptr;
    }
    if (self->map)
    {
        self->map->cleanup();
        delete self->map;
        self->map = nullptr;
    }
    if (RoundSettings::instance)
    {
        delete RoundSettings::instance;
    }
    self->reqIDs.clear();
    self->cmdIDs.clear();
    self->cmdpids.clear();
    self->cmdargs.clear();
    self->recvData.clear();
    self->selfNestID = 0xff;
    self->name.clear();
    self->address = "127.0.0.1";
    self->port = ANTNET_DEFAULT_PORT;
}


bool AntGameClient_callGameCallback(PyObject* func)
{
    if (func == nullptr)
    {
        return true;
    }
    PyObject* res = PyObject_CallObject(func, nullptr);
    Py_XDECREF(res);
    return res != nullptr;
}


bool AntGameClient_callAntCallback(PyObject* func, PyObject*ant)
{
    if (func == nullptr)
    {
        return true;
    }
    PyObject* arglist = Py_BuildValue("(O)", ant);
    if (arglist == nullptr)
    {
        return false;
    }
    PyObject* result = PyObject_CallObject(func, arglist);
    Py_XDECREF(arglist);
    Py_XDECREF(result);
    return result != nullptr;
}


bool AntGameClient_running(AntGameClientObject* self)
{
    if (!self->conn->send("\0\0\0\x09\0\0\0\x01\x0a", 9))
    {
        AntGameClient_handleConnErrors(self->conn);
        AntGameClient_cleanup(self);
        return false;
    }
    self->reqIDs.push_back(ConnectionManager::RequestID::CHANGELOG);
    std::chrono::steady_clock::time_point lastServerResponse = std::chrono::steady_clock::now();
    return true;
}


static PyObject* AntGameClient_start(PyObject* op, PyObject* args, PyObject* keywds)
{
    AntGameClientObject*self = (AntGameClientObject*)op;
    int port = -1;
    const char* addr = nullptr;
    const char* name = nullptr;
    static char* kwlist[] = {(char*)"address", (char*)"port", (char*)"name", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|sis", kwlist, &addr, &port, &name))
    {
        return nullptr;
    }
    std::string addrStr = "";
    std::string nameStr = "";
    if (addr)
    {
        addrStr += addr;
    }
    if (name)
    {
        nameStr += name;
    }
    if (!addrStr.empty())
    {
        self->address = addrStr;
    }
    if (!nameStr.empty())
    {
        self->name = nameStr;
    }
    if (port >= 0)
    {
        self->port = port;
    }
    if (self->name.length() > 128)
    {
        self->name.erase(128, std::string::npos);
    }
    self->conn = new Connection;
    if (!self->conn)
    {
        PyErr_SetString(PyExc_MemoryError, "Failed to create the connection object because there is no available memory!");
        return nullptr;
    }
    if (!self->conn->connectTo(self->address, self->port))
    {
        switch (self->conn->errorState)
        {
            case Connection::OK:
                PyErr_SetString(PyExc_OSError, "Failed to connect to the server but I'm being lied to and told nothing is wrong. (Not your code's fault)");
                break;
            case Connection::SYS:
                PyErr_SetString(PyExc_OSError, "Failed to connect to the server because of a system level problem. (Not your code's fault)");
                break;
            case Connection::RETRY:
                PyErr_SetString(PyExc_OSError, "Failed to connect to the server because of a system level problem. Try again in a second and it might work. (Not your code's fault)");
                break;
            case Connection::ADDR:
                PyErr_SetString(PyExc_ConnectionError, "Failed to connect to the server because the address or port is invalid. This may be your fault; make sure the server is on and you have the right address.");
                break;
            case Connection::NET:
                PyErr_SetString(PyExc_TimeoutError, "Failed to connect to the server because the network is down. Make sure your internet is on and you have the right address. May or may not be your fault.");
                break;
            case Connection::UNKNOWN:
            default:
                PyErr_SetString(PyExc_OSError, "Failed to connect to the server because of an unknown error (not your fault).");
                break;
        }
        AntGameClient_cleanup(self);
        return nullptr;
    }
    if (!self->conn->send("\0\0\0\x09\0\0\0\x01\0", 9))
    {
        AntGameClient_handleConnErrors(self->conn);
        std::cerr << "Failed to ask the server to join to the server." << std::endl;
        AntGameClient_cleanup(self);
        return nullptr;
    }
    for (self->recvData = ""; self->recvData.length() < 10;)
    {
        std::string dat = AntGameClient_readallWait(self->conn);
        if (dat == "")
        {
            std::cerr << "Kicked out by server or ran into an error before reading any data." << std::endl;
            AntGameClient_cleanup(self);
            return nullptr;
        }
        self->recvData.append(dat);
    }
    std::string testerString;
    testerString.append("\0\0\0\x0a\0\0\0\x01\0", 9);
    if (self->recvData.compare(0, 9, testerString) != 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in opening response. Is this a modified server?");
        AntGameClient_cleanup(self);
        return nullptr;
    }
    self->recvData.erase(0,9);
    if (self->recvData[0] == '\x01')
    {
        PyErr_SetString(PyExc_RuntimeError, "Server responded but denied entry (is the game full?)");
        AntGameClient_cleanup(self);
        return nullptr;
    }
    else if (self->recvData[0] != '\0')
    {
        PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in opening response ID. Is this a modified server?");
        AntGameClient_cleanup(self);
        return nullptr;
    }
    self->recvData.erase(0,1);
    std::string messages = "";
    messages.append(ConnectionManager::makeAGNPuint(self->name.length() + 12));
    messages.append(ConnectionManager::makeAGNPuint(3));
    messages.push_back('\x05');
    messages.push_back('\x09');
    messages.push_back('\x03');
    messages.push_back((char)self->name.length());
    messages.append(self->name);
    if (!self->conn->send(messages.c_str(), messages.length()))
    {
        AntGameClient_handleConnErrors(self->conn);
        AntGameClient_cleanup(self);
        return nullptr;
    }
    messages.clear();
    bool gameStarted = false;
    bool gotNameOk = false;
    {
    unsigned int responsesLen = 0;
    unsigned int dataIndex = 0;
    unsigned int rc = 0;
    bool holdIt = false;
    while (!gameStarted && self->conn->connected() && self->conn->errorState == Connection::OK)
    {
        if (!holdIt)
        {
            std::string dat = AntGameClient_readallWait(self->conn);
            if (dat.empty())
            {
                std::cerr << "Kicked out by server or ran into an error while reading before game started." << std::endl;
                AntGameClient_cleanup(self);
                return nullptr;
            }
            self->recvData.append(dat);
        }
        else {holdIt = false;}
        if (responsesLen == 0 && self->recvData.length() >= 4)
        {
            responsesLen = ConnectionManager::getAGNPuint(self->recvData.substr(0, 4));
            dataIndex = 4;
            if (responsesLen < 8)
            {
                PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in request length. Is this a modified server?");
                AntGameClient_cleanup(self);
                return nullptr;
            }
        }
        if (rc == 0 && self->recvData.length() - dataIndex >= 4 && responsesLen != 0)
        {
            rc = ConnectionManager::getAGNPuint(self->recvData.substr(4, 4));
            dataIndex = 8;
        }
        if (self->recvData.length() >= responsesLen && responsesLen > 0)
        {
            while (rc > 0 && responsesLen - dataIndex >= 2)
            {
                rc--;
                ConnectionManager::RequestID reqResp = (ConnectionManager::RequestID)(unsigned char)self->recvData[dataIndex];
                ConnectionManager::ResponseID respType = (ConnectionManager::ResponseID)(unsigned char)self->recvData[dataIndex+1];
                dataIndex += 2;
                switch (reqResp)
                {
                    case ConnectionManager::RequestID::NONE:
                        switch (respType)
                        {
                            case ConnectionManager::ResponseID::PING:
                                if (!self->conn->send("\0\0\0\x09\0\0\0\x01\x01", 9))
                                {
                                    AntGameClient_handleConnErrors(self->conn);
                                    std::cerr << "Could not respond to a server ping request." << std::endl;
                                    AntGameClient_cleanup(self);
                                    return nullptr;
                                }
                                break;
                            case ConnectionManager::ResponseID::BYE:
                                self->conn->send("\0\0\0\x09\0\0\0\x01\x02", 9);
                                PyErr_SetString(PyExc_RuntimeError, "Kicked out by server.");
                                AntGameClient_cleanup(self);
                                return nullptr;
                            case ConnectionManager::ResponseID::START:
                                if (responsesLen - dataIndex < 1)
                                {
                                    PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in starting game request. Is this a modified server?");
                                    AntGameClient_cleanup(self);
                                    return nullptr;
                                }
                                gameStarted = true;
                                self->selfNestID = (unsigned char)self->recvData[dataIndex];
                                break;
                            default:
                                PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in unsolicited response ID. Is this a modified server?");
                                AntGameClient_cleanup(self);
                                return nullptr;
                            
                        }
                        break;
                    case ConnectionManager::RequestID::SETTINGS:{
                        if (respType == ConnectionManager::ResponseID::DENY)
                        {
                             if (!self->conn->send("\0\0\0\x09\0\0\0\x01\x05", 9))
                             {
                                AntGameClient_handleConnErrors(self->conn);
                                std::cerr << "Could not send a settings request." << std::endl;
                                AntGameClient_cleanup(self);
                                return nullptr;
                             }
                        }
                        else if (respType != ConnectionManager::ResponseID::OKDATA && respType != ConnectionManager::ResponseID::OK)
                        {
                            PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol for response ID of settings request. Is this a modified server?");
                            AntGameClient_cleanup(self);
                            return nullptr;
                        }
                        else if (RoundSettings::instance != nullptr)
                        {
                            PyErr_SetString(PyExc_RuntimeError, "Server responded to the same request twice.");
                            AntGameClient_cleanup(self);
                            return nullptr;
                        }
                        if (responsesLen - dataIndex < 4)
                        {
                            PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in response to settings request (not enough space for setting length). Is this a modified server?");
                            AntGameClient_cleanup(self);
                            return nullptr;
                        }
                        unsigned int settingsLength = ConnectionManager::getAGNPuint(self->recvData.substr(dataIndex, 4));
                        dataIndex += 4;
                        if (responsesLen - dataIndex < settingsLength)
                        {
                            PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in response to settings request (not enough space for settings). Is this a modified server?");
                            AntGameClient_cleanup(self);
                            return nullptr;
                        }
                        (new RoundSettings)->_loadConfig(self->recvData.substr(dataIndex, settingsLength));
                        dataIndex += settingsLength;
                        break;}
                    case ConnectionManager::RequestID::NAME:
                        if (respType == ConnectionManager::ResponseID::DENY)
                        {
                             std::string nameMessage = ConnectionManager::makeAGNPuint(10 + self->name.length());
                             nameMessage.append(ConnectionManager::makeAGNPuint(1));
                             nameMessage.push_back((char)ConnectionManager::RequestID::NAME);
                             nameMessage.push_back((char)self->name.length());
                             nameMessage.append(self->name);
                             if (!self->conn->send(nameMessage.c_str(), nameMessage.length()))
                             {
                                AntGameClient_handleConnErrors(self->conn);
                                std::cerr << "Could not send a settings request." << std::endl;
                                AntGameClient_cleanup(self);
                                return nullptr;
                             }
                        }
                        else if (respType != ConnectionManager::ResponseID::OK)
                        {
                            PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in response to name request response ID. Is this a modified server?");
                            AntGameClient_cleanup(self);
                            return nullptr;
                        }
                        gotNameOk = true;
                        break;
                    case ConnectionManager::RequestID::MAP:{
                        if (respType == ConnectionManager::ResponseID::DENY)
                        {
                             if (!self->conn->send("\0\0\0\x09\0\0\0\x01\x09", 9))
                             {
                                AntGameClient_handleConnErrors(self->conn);
                                std::cerr << "Could not send a map request." << std::endl;
                                AntGameClient_cleanup(self);
                                return nullptr;
                             }
                        }
                        else if (respType != ConnectionManager::ResponseID::OKDATA && respType != ConnectionManager::ResponseID::OK)
                        {
                            PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in response to map request (invalid req ID). Is this a modified server?");
                            AntGameClient_cleanup(self);
                            return nullptr;
                        }
                        else if (self->map != nullptr)
                        {
                            PyErr_SetString(PyExc_RuntimeError, "Server responded to the same request twice.");
                            AntGameClient_cleanup(self);
                            return nullptr;
                        }
                        if (responsesLen - dataIndex < 5)
                        {
                            PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in response to map request (not enough space for header). Is this a modified server?");
                            AntGameClient_cleanup(self);
                            return nullptr;
                        }
                        self->map = new Map;
                        self->map->size.x = ConnectionManager::getAGNPushort(self->recvData.substr(dataIndex, 2));
                        self->map->size.y = ConnectionManager::getAGNPushort(self->recvData.substr(dataIndex + 2, 2));
                        unsigned char nestc = (unsigned char)self->recvData[dataIndex+4];
                        self->map->nests.reserve(nestc);
                        dataIndex += 5;
                        unsigned int biggestPid = 0;
                        for (unsigned char i = 0; i < nestc; i++)
                        {
                            if (responsesLen - dataIndex < 13)
                            {
                                PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in resopnse to map request (not enough space for nest). Is this a modified server?");
                                AntGameClient_cleanup(self);
                                return nullptr;
                            }
                            if (self->recvData.compare(dataIndex, 8, "\xff\xff\xff\xff\xff\xff\xff\xff") * self->recvData.compare(dataIndex+8, 2, "\xff\xff") * self->recvData.compare(dataIndex+10, 2, "\xff\xff") == 0 || self->recvData[dataIndex+12] == '\xff')
                            {
                                self->map->nests.push_back(nullptr);
                                dataIndex += 13;
                            }
                            Nest*n = new Nest;
                            n->foodCount = ConnectionManager::getAGNPdoublestr(self->recvData.substr(dataIndex, 8));
                            n->p.x = ConnectionManager::getAGNPushort(self->recvData.substr(dataIndex + 8, 2));
                            n->p.y = ConnectionManager::getAGNPushort(self->recvData.substr(dataIndex + 10, 2));
                            unsigned char antc = (unsigned char)self->recvData[dataIndex+12];
                            n->ants.reserve(antc);
                            n->parent = self->map;
                            dataIndex += 13;
                            if (responsesLen - dataIndex < 29 * antc)
                            {
                                PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in response to map request (not enough space for ants). Is this a modified server?");
                                AntGameClient_cleanup(self);
                                return nullptr;
                            }
                            for (unsigned char j = 0; j < antc; j++)
                            {
                                Ant* a = new Ant;
                                a->p.x = ConnectionManager::getAGNPshortdouble(ConnectionManager::getAGNPuint(self->recvData.substr(dataIndex, 4)));
                                a->p.y = ConnectionManager::getAGNPshortdouble(ConnectionManager::getAGNPuint(self->recvData.substr(dataIndex+4, 4)));
                                a->type = self->recvData[dataIndex+8];
                                a->pid = ConnectionManager::getAGNPuint(self->recvData.substr(dataIndex+9, 4));
                                a->health = ConnectionManager::getAGNPdoublestr(self->recvData.substr(dataIndex+13, 8));
                                a->foodCarry = ConnectionManager::getAGNPdoublestr(self->recvData.substr(dataIndex+21, 8));
                                a->parent = n;
                                dataIndex += 29;
                                if (a->pid != 0xffffffff) {biggestPid = std::max(biggestPid, a->pid);}
                                n->ants.push_back(a);
                            }
                            self->map->nests.push_back(n);
                        }
                        self->map->antPermanents.resize(biggestPid + 1, nullptr);
                        for (Nest*n: self->map->nests)
                        {
                            if (n) {for (Ant*a : n->ants) { self->map->antPermanents[a->pid] = a; }}
                        }
                        if (responsesLen - dataIndex < (unsigned int)self->map->size.x * (unsigned int) self->map->size.y)
                        {
                            PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in response to map request (not enough space for map). Is this a modified server?");
                            AntGameClient_cleanup(self);
                            return nullptr;
                        }
                        self->map->map = new unsigned char[(unsigned int)self->map->size.x * (unsigned int)self->map->size.y];
                        for (unsigned int i = 0; i < (unsigned int)self->map->size.x * (unsigned int)self->map->size.y; i++)
                        {
                            self->map->map[i] = self->recvData[i+dataIndex];
                        }
                        dataIndex += (unsigned int)self->map->size.x * (unsigned int)self->map->size.y;
                        break;}
                    default:
                        PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol (wrong request ID echo). Is this a modified server?");
                        AntGameClient_cleanup(self);
                        return nullptr;
                }
            }
            if (rc == 0)
            {
                self->recvData.erase(0, responsesLen);
                responsesLen = 0;
                dataIndex = 0;
                holdIt = true;
            }
            else
            {
                PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol (not enough space for all responses). Is this a modified server?");
                AntGameClient_cleanup(self);
                return nullptr;
            }
        }
    }
    } // Scope end
    if (!self->map || !RoundSettings::instance || self->selfNestID == 0xff)
    {
        PyErr_SetString(PyExc_RuntimeError, "The server started the game before answering requests!");
        AntGameClient_cleanup(self);
        return nullptr;
    }
    if (AntGameClient_callGameCallback(self->onStart))
    {
        AntGameClient_cleanup(self);
        return nullptr;
    }
    
    if (!AntGameClient_running(self))
    {
        AntGameClient_cleanup(self);
        return nullptr;
    }

    AntGameClient_cleanup(self);
    Py_RETURN_NONE;
}


PyObject* AntGameClientObject::*callbackGetter(std::string str)
{
    for (int i = 0; i < str.length(); i++)
    {
        if ((int)str[i] > 64 && (int)str[i] < 91)
        {
            str[i] = (char)((int)str[i] + 32);
            if (i == 0 || str[i-1] != ' ')
            {
                str.insert(i, "_");
                i--;
            }
        }
        else if (str[i] == ' ' || str[i] == '-')
        {
            str[i] = '_';
        }
    }
    if (str == "game_start")
    {
        return &AntGameClientObject::onStart;
    }
    else if (str == "game_frame")
    {
        return &AntGameClientObject::onFrame;
    }
    else if (str == "ant_new")
    {
        return &AntGameClientObject::onNewAnt;
    }
    else if (str == "ant_grab")
    {
        return &AntGameClientObject::onGrab;
    }
    else if (str == "ant_home")
    {
        return &AntGameClientObject::onHome;
    }
    else if (str == "ant_hurt")
    {
        return &AntGameClientObject::onAttacked;
    }
    else if (str == "ant_hit")
    {
        return &AntGameClientObject::onHit;
    }
    else if (str == "ant_death")
    {
        return &AntGameClientObject::onDeath;
    }
    else
    {
        PyErr_SetString(PyExc_ValueError, "The callback type should be one of the defined types! Check the docs for all available types.");
        return nullptr;
    }
}


PyObject* AntGameClient_setCallback(PyObject*op, PyObject*args)
{
    AntGameClientObject* self = (AntGameClientObject*)op;
    PyObject* func;
    const char* callbackType = nullptr;
    if (PyArg_ParseTuple(args, "Os:setCallback", &func, &callbackType))
    {
        if (!PyCallable_Check(func))
        {
            PyErr_SetString(PyExc_TypeError, "Callback to set must be a function!");
            return nullptr;
        }
        std::string cback = callbackType;
        PyObject* AntGameClientObject::*memberPtr = callbackGetter(cback);
        if (!memberPtr)
        {
            return nullptr;
        }
        self->*memberPtr = func;
        Py_RETURN_NONE;
    }
    return nullptr;
}


static PyMethodDef AntGameClient_methods[] = {
    {"connect", (PyCFunction)(void(*)(void))AntGameClient_start, METH_VARARGS | METH_KEYWORDS, "Connect to a server and start running."},
    {"setCallback", AntGameClient_setCallback, METH_VARARGS, "Set a callback function."},
    {nullptr, nullptr, 0, nullptr}
};


static PyTypeObject AntGameClientType = {
    .ob_base = PyVarObject_HEAD_INIT(nullptr, 0)
    .tp_name = "AntGame.AntGameClient",
    .tp_basicsize = sizeof(AntGameClientObject),
    .tp_itemsize = 0,
    .tp_dealloc = AntGameClient_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    //TODO
    .tp_doc = PyDoc_STR("Placeholder doc string"),
    .tp_members = AntGameClient_members,
    .tp_methods = AntGameClient_methods,
    .tp_getset = AntGameClient_getsetters,
    .tp_new = AntGameClient_new,
};


struct NestObject
{
    PyObject_HEAD
    ;
    PyObject*root;
    unsigned char nestID;
};


static PyObject* Nest_getpos(PyObject* op, void* closure)
{
    NestObject*self = (NestObject*)op;
    PosObject*posobj = (PosObject*)PosType.tp_alloc(&PosType, 0);
    if (posobj)
    {
        if (!self->root || self->nestID == 0xff)
        {
            posobj->p.x = 0;
            posobj->p.y = 0;
        }
        else
        {
            AntGameClientObject*root = (AntGameClientObject*)self->root;
            if (!root->map || root->map->nests.size() <= self->nestID || !root->map->nests[self->nestID])
            {
                posobj->p.x = 0; posobj->p.y = 0;
            }
            else
            {
                posobj->p = root->map->nests[self->nestID]->p;
            }
        }
    }
    Py_INCREF(posobj);
    return (PyObject*)posobj;
}


static PyObject* Nest_getfood(PyObject* op, void* closure)
{
    NestObject*self = (NestObject*)op;
    if (!self->root || self->nestID == 0xff)
    {
        return PyFloat_FromDouble(0);
    }
    AntGameClientObject* root = (AntGameClientObject*)self->root;
    if (!root->map || root->map->nests.size() <= self->nestID || !root->map->nests[self->nestID])
    {
        return PyFloat_FromDouble(0);
    }
    return PyFloat_FromDouble(root->map->nests[self->nestID]->foodCount);
}


static PyObject* Nest_getants(PyObject* op, void *closure);


static PyGetSetDef Nest_getsetters[] = {
    {"ants", Nest_getants, nullptr, "nest's ants", nullptr},
    {"pos", Nest_getpos, nullptr, "position", nullptr},
    {"food", Nest_getfood, nullptr, "food amount", nullptr},
    {nullptr}
};


static PyTypeObject NestType = {
    .ob_base = PyVarObject_HEAD_INIT(nullptr, 0)
    .tp_name = "AntGame.Nest",
    .tp_basicsize = sizeof(NestObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    //TODO
    .tp_doc = PyDoc_STR("Placeholder doc string"),
    .tp_getset = Nest_getsetters,
    .tp_new = PyType_GenericNew,
};


struct AntObject
{
    PyObject_HEAD
    ;
    PyObject*root;
    unsigned int antID;
    Ant echo;
};


static PyObject* Ant_gettype(PyObject* op, void*closure)
{
    AntObject* self = (AntObject*)op;
    if (!self->root)
    {
        if (self->echo.type != 0xff)
        {
            return PyLong_FromUnsignedLong((unsigned long)self->echo.type);
        }
    }
    if (self->antID == 0xffffffff)
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "This ant is dead! Accessing its type may cause issues.", 2) < 0)
        {
            return nullptr;
        }
        return PyLong_FromLong(0);
    }
    AntGameClientObject* root = (AntGameClientObject*)self->root;
    if (!root->map || root->map->antPermanents.size() <= self->antID || !root->map->antPermanents[self->antID])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "This ant is dead! Accessing its type may cause issues.", 2) < 0)
        {
            return nullptr;
        }
        return PyLong_FromLong(0);
    }
    // TODO better
    return PyLong_FromUnsignedLong((unsigned long)root->map->antPermanents[self->antID]->type);
}


static PyObject* Ant_gethealth(PyObject* op, void*closure)
{
    AntObject* self = (AntObject*)op;
    if (!self->root || self->antID == 0xffffffff)
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "This ant is dead! Accessing its health may cause issues.", 2) < 0)
        {
            return nullptr;
        }
        return PyFloat_FromDouble(0);
    }
    AntGameClientObject* root = (AntGameClientObject*)self->root;
    if (!root->map || root->map->antPermanents.size() <= self->antID || !root->map->antPermanents[self->antID])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "This ant is dead! Accessing its health may cause issues.", 2) < 0)
        {
            return nullptr;
        }
        return PyFloat_FromDouble(0);
    }
    // TODO better
    return PyFloat_FromDouble(root->map->antPermanents[self->antID]->health);
}


static PyObject* Ant_getfood(PyObject* op, void*closure)
{
    AntObject* self = (AntObject*)op;
    if (!self->root)
    {
        if (self->echo.type != 0xff)
        {
            return PyFloat_FromDouble(self->echo.foodCarry);
        }
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "This ant is dead! Accessing its food amount may cause issues.", 2) < 0)
        {
            return nullptr;
        }
        return PyFloat_FromDouble(0);
    }
    if (self->antID == 0xffffffff)
    {
        return PyFloat_FromDouble(0);
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "This ant is dead! Accessing its food amount may cause issues.", 2) < 0)
        {
            return nullptr;
        }
    }
    AntGameClientObject* root = (AntGameClientObject*)self->root;
    if (!root->map || root->map->antPermanents.size() <= self->antID || !root->map->antPermanents[self->antID])
    {
        return PyFloat_FromDouble(0);
    }
    // TODO better
    return PyFloat_FromDouble(root->map->antPermanents[self->antID]->foodCarry);
}



static PyObject* Ant_getpos(PyObject* op, void* closure)
{
    AntObject*self = (AntObject*)op;
    PosObject*posobj = (PosObject*)PosType.tp_alloc(&PosType, 0);
    Py_INCREF(posobj);
    if (posobj)
    {
        if (!self->root)
        {
            if (self->echo.type != 0xff)
            {
                posobj->p = self->echo.p;
            }
            else
            {
                posobj->p.x = 0;
                posobj->p.y = 0;
                if (PyErr_WarnEx(PyExc_RuntimeWarning, "This ant is dead! Accessing its position may cause issues.", 2) < 0)
                {
                    Py_DECREF(posobj);
                    return nullptr;
                }
            }
        }
        if (self->antID == 0xffffffff)
        {
            posobj->p.x = 0;
            posobj->p.y = 0;
            if (PyErr_WarnEx(PyExc_RuntimeWarning, "This ant is dead! Accessing its position may cause issues.", 2) < 0)
            {
                Py_DECREF(posobj);
                return nullptr;
            }
        }
        else
        {
            AntGameClientObject*root = (AntGameClientObject*)self->root;
            if (!root->map || root->map->antPermanents.size() <= self->antID || !root->map->antPermanents[self->antID])
            {
                posobj->p.x = 0; posobj->p.y = 0;
                if (PyErr_WarnEx(PyExc_RuntimeWarning, "This ant is dead! Accessing its position may cause issues.", 2) < 0)
                {
                    Py_DECREF(posobj);
                    return nullptr;
                }
            }
            else
            {
                posobj->p = root->map->antPermanents[self->antID]->p;
            }
        }
    }
    return (PyObject*)posobj;
}


static PyGetSetDef Ant_getsetters[] = {
    {"food", Ant_getfood, nullptr, "ant's food", nullptr},
    {"health", Ant_gethealth, nullptr, "ant's health", nullptr},
    {"type", Ant_gettype, nullptr, "ant's type", nullptr},
    {"pos", Ant_getpos, nullptr, "ant's position", nullptr},
    {nullptr}
};


static PyTypeObject AntType = {
    .ob_base = PyVarObject_HEAD_INIT(nullptr, 0)
    .tp_name = "AntGame.Ant",
    .tp_basicsize = sizeof(AntObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    //TODO
    .tp_doc = PyDoc_STR("Placeholder doc string"),
    .tp_getset = Ant_getsetters,
    .tp_new = PyType_GenericNew,
};


static PyObject* AntGameClient_getnests(PyObject* op, void *closure)
{
    AntGameClientObject*self = (AntGameClientObject*)op;
    if (self->map == nullptr || self->map->nests.empty())
    {
        return PyList_New(0);
    }
    unsigned char nestc = 0;
    for (Nest* n : self->map->nests)
    {if(n){nestc++;}}
    PyObject* ret = PyList_New(nestc);
    if (!ret)
    {
        PyErr_SetString(PyExc_MemoryError, "Failed to get memory for the list of nests!");
        return nullptr;
    }
    for (int i = 0; i < self->map->nests.size() && i < 256; i++)
    {
        if (!self->map->nests[i]) {continue;}
        NestObject* item = (NestObject*)NestType.tp_alloc(&NestType, 0);
        if (!item)
        {
            Py_DECREF(ret);
            PyErr_SetString(PyExc_MemoryError, "Failed to get memory for a nest in the list of nests!");
            return nullptr;
        }
        item->root = op;
        item->nestID = i;
        Py_INCREF(item);
        PyList_SetItem(ret, i, (PyObject*)item);
    }
    return ret;
}


static PyObject* AntGameClient_getants(PyObject* op, void *closure)
{
    AntGameClientObject*self = (AntGameClientObject*)op;
    if (self->map == nullptr || self->map->antPermanents.empty())
    {
        return PyList_New(0);
    }
    unsigned int amount = 0;
    for (Ant*a : self->map->antPermanents)
    {
        if (a)
        {
            amount++;
        }
    }
    PyObject* ret = PyList_New(amount);
    if (!ret)
    {
        return nullptr;
    }
    unsigned int j = 0;
    for (unsigned int i = 0; i < self->map->antPermanents.size(); i++)
    {
        if (!self->map->antPermanents[i])
        {
            continue;
        }
        AntObject* item = (AntObject*)AntType.tp_alloc(&AntType, 0);
        if (!item)
        {
            Py_DECREF(ret);
            return nullptr;
        }
        item->root = op;
        item->antID = i;
        item->echo.type = 0xff;
        Py_INCREF(item);
        PyList_SetItem(ret, j, (PyObject*)item);
        j++;
    }
    return ret;
}


static PyObject* AntGameClient_getme(PyObject* op, void* closure)
{
    AntGameClientObject*self = (AntGameClientObject*)op;
    if (self->map == nullptr || self->selfNestID == 0xff || self->conn == nullptr || !self->conn->connected())
    {
        PyErr_SetString(PyExc_RuntimeWarning, "Your client is not connected! Connect first to get me.");
        return nullptr;
    }
    NestObject*nobj = (NestObject*)NestType.tp_alloc(&NestType, 0);
    nobj->root = op;
    nobj->nestID = self->selfNestID;
    Py_INCREF((PyObject*)nobj);
    return (PyObject*)nobj;
}


static int AntGame_module_exec(PyObject *m)
{
    if (PyType_Ready(&AntGameClientType) < 0)
    {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "AntGameClient", (PyObject*)&AntGameClientType) < 0)
    {
        return -1;
    }
    if (PyType_Ready(&NestType) < 0)
    {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "Nest", (PyObject*)&NestType) < 0)
    {
        return -1;
    }
    if (PyType_Ready(&AntType) < 0)
    {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "Ant", (PyObject*)&AntType) < 0)
    {
        return -1;
    }
    if (PyType_Ready(&PosType) < 0)
    {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "Pos", (PyObject*)&PosType) < 0)
    {
        return -1;
    }
    return 0;
}


static PyObject* Nest_getants(PyObject* op, void *closure)
{
    NestObject*self = (NestObject*)op;
    if (!self->root || self->nestID == 0xff)
    {
        return PyList_New(0);
    }
    AntGameClientObject* root = (AntGameClientObject*)self->root;
    if (!root->map || root->map->nests.size() <= self->nestID || !root->map->nests[self->nestID])
    {
        return PyList_New(0);
    }
    Nest*n = root->map->nests[self->nestID];
    PyObject* ret = PyList_New(n->ants.size());
    if (!ret)
    {
        PyErr_SetString(PyExc_MemoryError, "Failed to get memory for the list of nests!");
        return nullptr;
    }
    for (int i = 0; i < n->ants.size() && i < 256; i++)
    {
        AntObject* item = (AntObject*)AntType.tp_alloc(&AntType, 0);
        if (!item)
        {
            Py_DECREF(ret);
            PyErr_SetString(PyExc_MemoryError, "Failed to get memory for an ant in the list of ants!");
            return nullptr;
        }
        item->root = self->root;
        item->antID = n->ants[i]->pid;
        item->echo.type = 0xff;
        Py_INCREF(item);
        PyList_SetItem(ret, i, (PyObject*)item);
    }
    return ret;
}


static PyMethodDef AntGame_methods[] = {
    {nullptr, nullptr, 0, nullptr}
};


static PyModuleDef_Slot AntGame_module_slots[] = {
    {Py_mod_exec, (void*)AntGame_module_exec},
    {0, nullptr}
};


static struct PyModuleDef AntGame_module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "AntGame",
    .m_size = 0,
    .m_methods = AntGame_methods,
    .m_slots = AntGame_module_slots
};


PyMODINIT_FUNC PyInit_AntGame(void)
{
    return PyModuleDef_Init(&AntGame_module);
}
