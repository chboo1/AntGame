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

#ifdef far // I have no fucking idea what moron defined a macro this simple, but I have to undef it on windows
#undef far
#endif


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


static PyObject* Pos_dist(PyObject* op, PyObject* args);


static PyMethodDef Pos_methods[] = {
    {"dist", Pos_dist, METH_VARARGS, "Get the distance to another pos."},
    {nullptr, nullptr, 0, nullptr}
};


static PyTypeObject PosType = {
    .ob_base = PyVarObject_HEAD_INIT(nullptr, 0)
    .tp_name = "AntGame.Pos",
    .tp_basicsize = sizeof(PosObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    //TODO
    .tp_doc = PyDoc_STR("Placeholder doc string"),
    .tp_methods = Pos_methods,
    .tp_members = Pos_members,
    .tp_new = PyType_GenericNew,
};


static PyObject* Pos_dist(PyObject* op, PyObject* args)
{
    PosObject* self = (PosObject*)op;
    PyObject* arg = nullptr;
    if (PyArg_ParseTuple(args, "O:take", &arg))
    {
        if (!arg)
        {
            PyErr_SetString(PyExc_TypeError,"The argument to the dist function must be a position!");
            return nullptr;
        }
        if (PyObject_TypeCheck(arg, &PosType) == 0)
        {
            PyErr_SetString(PyExc_TypeError,"The argument to the dist function must be a position!");
            return nullptr;
        }
        PosObject* posobj = (PosObject*)arg;
        double result = (posobj->p-self->p).magnitude();
        return PyFloat_FromDouble(result);
    }
    return nullptr;
}


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
    PyObject*onFull;
    PyObject*onDeliver;
    PyObject*onAttacked;
    PyObject*onHit;
    PyObject*onDeath;
    PyObject*onWait;

    std::deque<unsigned int> hitAnts;
    std::deque<unsigned int> newAnts;
    std::deque<unsigned int> hurtAnts;
    std::deque<Ant> deadAnts;
    std::deque<unsigned int> grabAnts;
    std::deque<unsigned int> deliverAnts;
    std::deque<unsigned int> fullAnts;

    std::chrono::steady_clock::time_point lastFrame;

    struct
    {
        unsigned int gap;
        std::uint64_t current;
        unsigned short count;
        std::deque<unsigned int> gaps;
        std::deque<std::uint64_t> currents;
        std::deque<unsigned short> counts;
    } nearestFoodData;
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


static Pos AntGameClient_findNearestFood(AntGameClientObject*, Pos, bool(*)(AntGameClientObject*,Pos));
static bool AntGameClient_isFood(AntGameClientObject*, Pos);
static bool AntGameClient_isUnclaimedFood(AntGameClientObject*, Pos);


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
        self->onDeliver = nullptr;
        self->onAttacked = nullptr;
        self->onHit = nullptr;
        self->onDeath = nullptr;
        self->onFull = nullptr;
        self->onWait = nullptr;

        self->hitAnts.clear();
        self->newAnts.clear();
        self->hurtAnts.clear();
        self->deadAnts.clear();
        self->grabAnts.clear();
        self->deliverAnts.clear();
        self->fullAnts.clear();

        self->nearestFoodData.gap = 5;
        self->nearestFoodData.current = 4;
        self->nearestFoodData.count = 2;
        self->nearestFoodData.gaps.clear();
        self->nearestFoodData.gaps.push_back(5);
        self->nearestFoodData.currents.clear();
        self->nearestFoodData.currents.push_back(5);
        self->nearestFoodData.counts.clear();
        self->nearestFoodData.counts.push_back(2);
    }
    return (PyObject*)self;
}


static PyObject* AntGameClient_getnests(PyObject* op, void *closure);
static PyObject* AntGameClient_getants(PyObject* op, void *closure);
static PyObject* AntGameClient_getenemies(PyObject* op, void *closure);
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
    Py_XINCREF(posobj);
    return (PyObject*)posobj;
}


static PyGetSetDef AntGameClient_getsetters[] = {
    {"nests", AntGameClient_getnests, nullptr, "map's nests", nullptr},
    {"ants", AntGameClient_getants, nullptr, "map's ants", nullptr},
    {"enemies", AntGameClient_getenemies, nullptr, "map's enemy ants", nullptr},
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


static bool AntGameClient_callGameCallback(PyObject* func)
{
    if (func == nullptr)
    {
        return true;
    }
    PyObject* res = PyObject_CallObject(func, nullptr);
    if (!res)
    {
        return false;
    }
    Py_XDECREF(res);
    return true;
}


static bool AntGameClient_callAntCallback(PyObject* func, PyObject*ant)

{
    if (func == nullptr)
    {
        return true;
    }
    PyObject* result = PyObject_CallOneArg(func, ant);
    if (!result)
    {
        return false;
    }
    Py_XDECREF(result);
    return true;
}


static bool AntGameClient_continueFollow(AntGameClientObject*, unsigned int, unsigned int);
static bool AntGameClient_continueFollowAttack(AntGameClientObject*, unsigned int, unsigned int);


static bool AntGameClient_callAntCallback(AntGameClientObject* self, PyObject*func, Ant* a);


static bool AntGameClient_frame(AntGameClientObject*self)
{
    for (Ant*a : self->map->nests[self->selfNestID]->ants)
    {
        if (!a->commands.empty())
        {
            unsigned int followPid = UINT_MAX;
            auto fcmdit = a->commands.end();
            bool followAttack = false;
            for (auto it = a->commands.begin(); it != a->commands.end();)
            {
                Ant::AntCommand acmd = *it;
                if (followPid == UINT_MAX)
                {
                    if (acmd.cmd == Ant::AntCommand::ID::FOLLOW)
                    {
                        if (acmd.arg == UINT_MAX || !self->map->antPermanents[acmd.arg])
                        {
                            it = a->commands.erase(it);
                            continue;
                        }
                        followPid = acmd.arg;
                        break;
                    }
                    else if (acmd.cmd == Ant::AntCommand::ID::CFOLLOW)
                    {
                        if (acmd.arg == UINT_MAX || !self->map->antPermanents[acmd.arg])
                        {
                            it = a->commands.erase(it);
                            continue;
                        }
                        followPid = acmd.arg;
                        followAttack = true;
                        fcmdit = it;
                    }
                }
                else if (fcmdit != a->commands.end())
                {
                    if (acmd.cmd == Ant::AntCommand::ID::AINTERACT && acmd.arg == followPid)
                    {
                        fcmdit = a->commands.end();
                        break;
                    }
                }
                it++;
            }
            if (fcmdit != a->commands.end())
            {
                a->commands.erase(fcmdit);
            }
            else if (followPid != UINT_MAX)
            {
                if (followAttack)
                {
                    if (!AntGameClient_continueFollowAttack(self, a->pid, followPid))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!AntGameClient_continueFollow(self, a->pid, followPid))
                    {
                        return false;
                    }
                }
            }
        }
    }
    auto now = std::chrono::steady_clock::now();
    if (!AntGameClient_callGameCallback(self->onFrame))
    {
        return false;
    }
    for (unsigned int pid : self->newAnts)
    {
        if (pid < self->map->antPermanents.size() && self->map->antPermanents[pid] && self->map->antPermanents[pid]->parent == self->map->nests[self->selfNestID])
        {
            if (!AntGameClient_callAntCallback(self, self->onNewAnt, self->map->antPermanents[pid]))
            {
                return false;
            }
        }
    }
    self->newAnts.clear();
    for (unsigned int pid : self->hurtAnts)
    {
        if (pid < self->map->antPermanents.size() && self->map->antPermanents[pid] && self->map->antPermanents[pid]->parent == self->map->nests[self->selfNestID])
        {
            if (!AntGameClient_callAntCallback(self, self->onAttacked, self->map->antPermanents[pid]))
            {
                return false;
            }
        }
    }
    self->hurtAnts.clear();
    for (unsigned int pid : self->grabAnts)
    {
        if (pid < self->map->antPermanents.size() && self->map->antPermanents[pid] && self->map->antPermanents[pid]->parent == self->map->nests[self->selfNestID])
        {
            if (!AntGameClient_callAntCallback(self, self->onGrab, self->map->antPermanents[pid]))
            {
                return false;
            }
        }
    }
    self->grabAnts.clear();
    for (unsigned int pid : self->deliverAnts)
    {
        if (pid < self->map->antPermanents.size() && self->map->antPermanents[pid] && self->map->antPermanents[pid]->parent == self->map->nests[self->selfNestID])
        {
            if (!AntGameClient_callAntCallback(self, self->onDeliver, self->map->antPermanents[pid]))
            {
                return false;
            }
        }
    }
    self->deliverAnts.clear();
    for (unsigned int pid : self->fullAnts)
    {
        if (pid < self->map->antPermanents.size() && self->map->antPermanents[pid] && self->map->antPermanents[pid]->parent == self->map->nests[self->selfNestID])
        {
            if (!AntGameClient_callAntCallback(self, self->onFull, self->map->antPermanents[pid]))
            {
                return false;
            }
        }
    }
    self->fullAnts.clear();
    for (Ant*a:self->map->nests[self->selfNestID]->ants)
    {
        if (a->commands.empty())
        {
            if (!AntGameClient_callAntCallback(self, self->onWait, a))
            {
                return false;
            }
        }
    }
    for (Ant a : self->deadAnts)
    {
        if (!AntGameClient_callAntCallback(self, self->onDeath, &a))
        {
            return false;
        }
    }
    self->deadAnts.clear();
    now = std::chrono::steady_clock::now();
    return true;
}


static bool AntGameClient_running(AntGameClientObject* self)
{
    if (!self->conn->send("\0\0\0\x09\0\0\0\x01\x0a", 9))
    {
        AntGameClient_handleConnErrors(self->conn);
        AntGameClient_cleanup(self);
        return false;
    }
    self->reqIDs.push_back(ConnectionManager::RequestID::CHANGELOG);
    std::chrono::steady_clock::time_point lastServerResponse = std::chrono::steady_clock::now();
    bool dead = false;
    bool needData = self->recvData.length() < 8;
    unsigned int responsesLen = 0;
    unsigned int dataIndex = 0;
    unsigned int rc = 0;
    bool triggerFrame = false;
    self->lastFrame = std::chrono::steady_clock::now();
    while (!dead) // Aren't we all
    {
        if (!self->conn->connected())
        {
            std::cerr << "Kicked out by server for some reason." << std::endl;
            return false;
        }
        if (needData)
        {
            std::string dat = AntGameClient_readallWait(self->conn);
            if (dat.empty())
            {
                std::cerr << "Kicked out by server or ran into an error while reading." << std::endl;
                return false;
            }
            self->recvData.append(dat);
            needData = false;
        }
        if (responsesLen == 0)
        {
            if (self->recvData.length() >= 4)
            {
                responsesLen = ConnectionManager::getAGNPuint(self->recvData.substr(0, 4));
                dataIndex = 4;
                if (responsesLen < 8)
                {
                    PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in request length. Is this a modified server?");
                    return false;
                }
            }
            else
            {
                needData = true;
            }
        }
        if (rc == 0  && responsesLen != 0)
        {
            if (self->recvData.length() >= 8)
            {
                rc = ConnectionManager::getAGNPuint(self->recvData.substr(4, 4));
                dataIndex = 8;
            }
            else
            {
                needData = true;
            }
        }
        if (self->recvData.length() < responsesLen)
        {
            needData = true;
        }
        else
        {
            while (rc > 0 && responsesLen - dataIndex >= 2)
            {
                rc--;
                ConnectionManager::RequestID reqResp = (ConnectionManager::RequestID)(unsigned char)self->recvData[dataIndex];
                ConnectionManager::ResponseID respType = (ConnectionManager::ResponseID)(unsigned char)self->recvData[dataIndex+1];
                dataIndex += 2;
                bool corresponds = reqResp == ConnectionManager::RequestID::NONE;
                for (auto it = self->reqIDs.begin(); it != self->reqIDs.end() && !corresponds; it++)
                {
                    if (*it == reqResp)
                    {
                        self->reqIDs.erase(it);
                        corresponds = true;
                        break;
                    }
                }
                if (!corresponds)
                {
                    PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol (replying to nonexistent request). Is this a modified server?");
                    return false;
                }
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
                                    return false;
                                }
                                break;
                            case ConnectionManager::ResponseID::BYE:
                                self->conn->send("\0\0\0\x09\0\0\0\x01\x02", 0);
                                dead = true;
                                break;
                            case ConnectionManager::ResponseID::CMDFAIL:
                            case ConnectionManager::ResponseID::CMDSUCCESS:{
                                auto tempStart = std::chrono::high_resolution_clock::now();
                                if (responsesLen - dataIndex < 1)
                                {
                                    PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in CMDSUCCESS/FAIL (no space). Is this a modified server?");
                                    return false;
                                }
                                unsigned char serverCmdId = (unsigned char)self->recvData[dataIndex];
                                unsigned int antPid = UINT_MAX;
                                std::uint64_t serverCmdArg = 0;
                                dataIndex++;
                                switch (serverCmdId)
                                {
                                    case (char)ConnectionManager::RequestID::TINTERACT:
                                        if (responsesLen - dataIndex < 8)
                                        {
                                            PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in CMDSUCCESS/FAIL to tinter (no space). Is this a modified server?");
                                            return false;
                                        }
                                        antPid = ConnectionManager::getAGNPuint(self->recvData.substr(dataIndex, 4));
                                        serverCmdArg = ConnectionManager::getAGNPuint(self->recvData.substr(dataIndex+4, 4));
                                        dataIndex += 8;
                                        break;
                                    case (char)ConnectionManager::RequestID::AINTERACT:
                                        if (responsesLen - dataIndex < 8)
                                        {
                                            PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in CMDSUCCESS/FAIL to ainter (no space). Is this a modified server?");
                                            return false;
                                        }
                                        antPid = ConnectionManager::getAGNPuint(self->recvData.substr(dataIndex, 4));
                                        serverCmdArg = ConnectionManager::getAGNPuint(self->recvData.substr(dataIndex+4, 4));
                                        dataIndex += 8;
                                        break;
                                    case (char)ConnectionManager::RequestID::WALK:
                                        if (responsesLen - dataIndex < 12)
                                        {
                                            PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in CMDSUCCESS/FAIL to walk (no space). Is this a modified server?");
                                            return false;
                                        }
                                        antPid = ConnectionManager::getAGNPuint(self->recvData.substr(dataIndex, 4));
                                        serverCmdArg = ConnectionManager::getAGNPuint(self->recvData.substr(dataIndex+4, 4));
                                        serverCmdArg <<= 32;
                                        serverCmdArg += ConnectionManager::getAGNPuint(self->recvData.substr(dataIndex+8, 4));
                                        dataIndex += 12;
                                        break;
                                    case (char)ConnectionManager::RequestID::NEWANT:
                                        if (responsesLen - dataIndex < 1)
                                        {
                                            PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in CMDSUCCESS/FAIL to newant (no space). Is this a modified server?");
                                            return false;
                                        }
                                        serverCmdArg = (std::uint64_t)(unsigned char)self->recvData[dataIndex];
                                        dataIndex += 1;
                                        break;
                                    default:
                                        PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in CMDSUCCESS/FAIL (invalid ID). Is this a modified server?");
                                        return false;
                                }
                                bool valid = false;
                                if (antPid != UINT_MAX)
                                {
                                    if (self->map->antPermanents.size() > antPid && self->map->antPermanents[antPid])
                                    {
                                        for (auto it = self->map->antPermanents[antPid]->commands.begin();it!=self->map->antPermanents[antPid]->commands.end();)
                                        {
                                            if ((((*it).cmd == Ant::AntCommand::ID::MOVE && serverCmdId == (unsigned char)ConnectionManager::RequestID::WALK) || ((*it).cmd == Ant::AntCommand::ID::TINTERACT && serverCmdId == (unsigned char)ConnectionManager::RequestID::TINTERACT) || ((*it).cmd == Ant::AntCommand::ID::AINTERACT && serverCmdId == (unsigned char)ConnectionManager::RequestID::AINTERACT)) && serverCmdArg == (*it).arg)
                                            {
                                                it = self->map->antPermanents[antPid]->commands.erase(it);
                                            }
                                            else
                                            {
                                                it++;
                                            }
                                        }
                                    }
                                }
                                auto tempDatC = std::chrono::high_resolution_clock::now();
                                auto IDit = self->cmdIDs.begin();
                                auto pidit = self->cmdpids.begin();
                                auto argit = self->cmdargs.begin();
                                while (IDit != self->cmdIDs.end())
                                {
                                    if ((*IDit) == (ConnectionManager::RequestID)serverCmdId && (*pidit) == antPid && (*argit) == serverCmdArg)
                                    {
                                        IDit = self->cmdIDs.erase(IDit);
                                        pidit = self->cmdpids.erase(pidit);
                                        argit = self->cmdargs.erase(argit);
                                        valid = true;
                                        break;
                                    }
                                    IDit++;
                                    pidit++;
                                    argit++;
                                }
                                auto tempCheck = std::chrono::high_resolution_clock::now();
                                if (!valid)
                                {
                                    PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in CMDSUCCESS/FAIL (invalid previous req data). Is this a modified server?");
                                    return false;
                                }
                                if (serverCmdId == (char)ConnectionManager::RequestID::TINTERACT && respType == ConnectionManager::ResponseID::CMDSUCCESS)
                                {
                                    unsigned short x = (serverCmdArg>>16) & 0xffff;
                                    unsigned short y = serverCmdArg & 0xffff;
                                    if (x == self->map->nests[self->selfNestID]->p.x && y == self->map->nests[self->selfNestID]->p.y)
                                    {
                                        self->deliverAnts.push_back(antPid);
                                    }
                                }
                                if (serverCmdId == (char)ConnectionManager::RequestID::AINTERACT && respType == ConnectionManager::ResponseID::CMDSUCCESS)
                                {
                                    if (self->map->antPermanents.size() > serverCmdArg && (!self->map->antPermanents[serverCmdArg] || self->map->antPermanents[serverCmdArg]->parent != self->map->nests[self->selfNestID]))
                                    {
                                        self->hitAnts.push_back(antPid);
                                    }
                                }
                                auto tempEnd = std::chrono::high_resolution_clock::now();
                                break;}
                            default:
                                PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol (invalid unsolicited response). Is this a modified server?");
                                return false;
                        }
                        break;
                    case ConnectionManager::RequestID::CANCEL:
                    case ConnectionManager::RequestID::NEWANT:
                    case ConnectionManager::RequestID::WALK:
                    case ConnectionManager::RequestID::TINTERACT:
                    case ConnectionManager::RequestID::AINTERACT:
                        if (respType != ConnectionManager::ResponseID::OK && respType != ConnectionManager::ResponseID::DENY)
                        {
                            PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in response to a command (invalid resp ID). Is this a modified server?");
                            return false;
                        }
                        break;
                    case ConnectionManager::RequestID::CHANGELOG:{
                        if (!self->conn->send("\0\0\0\x09\0\0\0\x01\x0a", 9))
                        {
                            AntGameClient_handleConnErrors(self->conn);
                            std::cerr << "Could not send a changelog request." << std::endl;
                            return false;
                        }
                        self->reqIDs.push_back(ConnectionManager::RequestID::CHANGELOG);
                        if (respType == ConnectionManager::ResponseID::DENY) // why tho :(
                        {
                            break;
                        }
                        else if (respType != ConnectionManager::ResponseID::OKDATA && respType != ConnectionManager::ResponseID::OK)
                        {
                            PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in response to changelog request (invalid resp ID). Is this a modified server?");
                            return false;
                        }
                        unsigned char nid = 0;
                        std::vector<bool> antExists;
                        antExists.resize(self->map->antPermanents.size(), false);
                        Ant* a = new Ant;
                        for (Nest*n : self->map->nests)
                        {
                            if (responsesLen - dataIndex < 9)
                            {
                                PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in response to changelog request (no space for nest). Is this a modified server?");
                                return false;
                            }
                            if (n)
                            {
                                if (self->recvData.compare(dataIndex, 8, "\xff\xff\xff\xff\xff\xff\xff\xff") == 0 || self->recvData[dataIndex+8] == '\xff')
                                {
                                    n->salute();
                                    delete n;
                                    self->map->nests[nid] = nullptr;
                                    nid++;
                                    dataIndex += 9;
                                    continue;
                                }
                                n->ants.clear();
                                n->foodCount = ConnectionManager::getAGNPdoublestr(self->recvData.substr(dataIndex, 8));
                                unsigned char antc = (unsigned char)self->recvData[dataIndex+8];
                                if (responsesLen - dataIndex < antc*13)
                                {
                                    PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in response to changelog request (no space for ants). Is this a modified server?");
                                    return false;
                                }
                                n->ants.reserve(antc);
                                dataIndex += 9;
                                for (unsigned char i = 0; i < antc; i++)
                                {
                                    a->p.x = ConnectionManager::getAGNPshortdouble(ConnectionManager::getAGNPuint(self->recvData.substr(dataIndex, 4)));
                                    a->p.y = ConnectionManager::getAGNPshortdouble(ConnectionManager::getAGNPuint(self->recvData.substr(dataIndex+4, 4)));
                                    a->type = self->recvData[dataIndex+8];
                                    a->pid = ConnectionManager::getAGNPuint(self->recvData.substr(dataIndex+9, 4));
                                    dataIndex += 13;
                                    if (a->pid >= self->map->antPermanents.size())
                                    {
                                        self->map->antPermanents.resize(a->pid + 1, nullptr);
                                        antExists.resize(a->pid + 1, false);
                                        a->_init(n, a->p, a->type);
                                        self->map->antPermanents[a->pid] = a;
					antExists[a->pid] = true;
					n->ants.push_back(a);
                                        if (n == self->map->nests[self->selfNestID])
                                        {
                                            self->newAnts.push_back(a->pid);
                                        }
					a = new Ant;
                                    }
                                    else
                                    {
					unsigned int thisPid = a->pid;
					if (!self->map->antPermanents[a->pid])
                                        {
                                            a->_init(n, a->p, a->type);
                                            self->map->antPermanents[a->pid] = a;
                                            if (n == self->map->nests[self->selfNestID])
                                            {
                                                self->newAnts.push_back(a->pid);
                                            }
					    a = new Ant;
                                        }
                                        else
                                        {
                                            self->map->antPermanents[a->pid]->p = a->p;
                                            self->map->antPermanents[a->pid]->type = a->type;
                                        }
                                        antExists[thisPid] = true;
                                        n->ants.push_back(self->map->antPermanents[thisPid]);
                                    }
                                }
                            }
                            else
                            {
                                dataIndex += 9;
                            }
                            nid++;
                        }
                        delete a;
                        a = nullptr;
                        for (unsigned int i = 0; i < self->map->antPermanents.size(); i++)
                        {
                            if (!antExists[i])
                            {
                                if (self->map->antPermanents[i])
                                {
                                    if (self->map->antPermanents[i]->parent == self->map->nests[self->selfNestID])
                                    {
                                        self->deadAnts.push_back(*self->map->antPermanents[i]);
                                    }
                                    delete self->map->antPermanents[i];
                                    self->map->antPermanents[i] = nullptr;
                                }
                            }
                        }
                        if (responsesLen - dataIndex < 4)
                        {
                            PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in response to changelog request (no space for map event count). Is this a modified server?");
                            return false;
                        }
                        unsigned int mec = ConnectionManager::getAGNPuint(self->recvData.substr(dataIndex, 4));
                        dataIndex += 4;
                        if (mec > 858993459 || responsesLen - dataIndex < mec * 5)
                        {
                            PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in response to changelog request (no space for map events). Is this a modified server?");
                            return false;
                        }
                        for (unsigned int i = 0; i < mec; i++)
                        {
                            unsigned short x = ConnectionManager::getAGNPushort(self->recvData.substr(dataIndex, 2));
                            unsigned short y = ConnectionManager::getAGNPushort(self->recvData.substr(dataIndex+2, 2));
                            unsigned char tile = (unsigned char)self->recvData[dataIndex+4];
                            if (x >= self->map->size.x || y >= self->map->size.y)
                            {
                                PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in response to changelog request (map event oob). Is this a modified server?");
                                return false;
                            }
                            self->map->map[x+y*self->map->size.x] = tile;
                            dataIndex += 5;
                        }
                        if (responsesLen - dataIndex < 4)
                        {
                            PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in response to changelog request (no space for ant event count). Is this a modified server?");
                            return false;
                        }
                        unsigned int aec = ConnectionManager::getAGNPuint(self->recvData.substr(dataIndex, 4));
                        dataIndex += 4;
                        if (aec > 214748364 || responsesLen - dataIndex < aec * 20)
                        {
                            PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol in response to changelog request (no space for ant events). Is this a modified server?");
                            return false;
                        }
                        for (unsigned int i = 0; i < aec; i++)
                        {
			    unsigned int pid = ConnectionManager::getAGNPuint(self->recvData.substr(dataIndex, 4));
                            double health = ConnectionManager::getAGNPdoublestr(self->recvData.substr(dataIndex+4, 8));
                            double food = ConnectionManager::getAGNPdoublestr(self->recvData.substr(dataIndex+12, 8));
                            if (self->map->antPermanents.size() > pid && self->map->antPermanents[pid])
                            {

                                if (self->map->antPermanents[pid]->parent == self->map->nests[self->selfNestID])
                                {
                                    Ant* ant = self->map->antPermanents[pid];
                                    if (ant->foodCarry < food)
                                    {
                                        if (Ant::antTypes[ant->type].capacity * RoundSettings::instance->capacityMod - food < 0.001)
                                        {
                                            self->fullAnts.push_back(ant->pid);
                                        }
                                        self->grabAnts.push_back(ant->pid);
                                    }
                                    if (ant->health > health)
                                    {
                                        self->hurtAnts.push_back(ant->pid);
                                    }
                                }
                                self->map->antPermanents[pid]->foodCarry = food;
                                self->map->antPermanents[pid]->health = health;
                            }
                            dataIndex += 20;
                        }
                        triggerFrame = true;
                        break;}
                    default:
                        PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol (wrong request ID echo). Is this a modified server?");
                        return false;
                }
            }
            if (rc > 0)
            {
                PyErr_SetString(PyExc_RuntimeError, "Server responded but did not use the correct protocol (not enough space for all responses). Is this a modified server?");
                return false;
            }
            else
            {
                self->recvData.erase(0, responsesLen);
                responsesLen = 0;
                dataIndex = 0;
            }
            if (triggerFrame)
            {
                triggerFrame = false;
                auto now = std::chrono::steady_clock::now();
                if (!AntGameClient_frame(self))
                {
                    return false;
                }
                now = std::chrono::steady_clock::now();
                self->lastFrame = now;
            }
        }
    }
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
                             break;
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
                             break;
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
                            break;
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
    if (!AntGameClient_callGameCallback(self->onStart))
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


static PyObject* AntGameClientObject::*callbackGetter(std::string str)
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
    else if (str == "ant_deliver")
    {
        return &AntGameClientObject::onDeliver;
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
    else if (str == "ant_full")
    {
        return &AntGameClientObject::onFull;
    }
    else if (str == "ant_wait")
    {
        return &AntGameClientObject::onWait;
    }
    else
    {
        PyErr_SetString(PyExc_ValueError, "The callback type should be one of the defined types! Check the docs for all available types.");
        return nullptr;
    }
}


static PyObject* AntGameClient_setCallback(PyObject*op, PyObject*args)
{
    AntGameClientObject* self = (AntGameClientObject*)op;
    PyObject* func;
    const char* callbackType = nullptr;
    if (PyArg_ParseTuple(args, "Os:setCallback", &func, &callbackType))
    {
        if (func == Py_None)
        {
            func = nullptr;
        }
        else if (!PyCallable_Check(func))
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


static PyObject* AntGameClient_newAnt(PyObject*op, PyObject*args);


static PyObject* AntGameClient_nearestFood(PyObject* op, PyObject* args)
{
    AntGameClientObject* self = (AntGameClientObject*)op;
    Pos p;
    if (!self->map)
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot get nearest food to nest if game is not started!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    else
    {
        p = self->map->nests[self->selfNestID]->p;
    }
    Pos t = AntGameClient_findNearestFood(self, p, AntGameClient_isFood);
    if (t.x == USHRT_MAX || t.y == USHRT_MAX)
    {
        Py_RETURN_NONE;
    }
    PosObject*posobj = (PosObject*)PosType.tp_alloc(&PosType, 0);
    if (posobj)
    {
        posobj->p = t;
    }
    Py_XINCREF(posobj);
    return (PyObject*)posobj;
}


static PyObject* AntGameClient_nearestUnclaimedFood(PyObject* op, PyObject* args)
{
    AntGameClientObject* self = (AntGameClientObject*)op;
    Pos p;
    if (!self->map)
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot get nearest food to nest if game is not started!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    else
    {
        p = self->map->nests[self->selfNestID]->p;
    }
    Pos t = AntGameClient_findNearestFood(self, p, AntGameClient_isUnclaimedFood);
    if (t.x == USHRT_MAX || t.y == USHRT_MAX)
    {
        Py_RETURN_NONE;
    }
    PosObject*posobj = (PosObject*)PosType.tp_alloc(&PosType, 0);
    if (posobj)
    {
        posobj->p = t;
    }
    Py_XINCREF(posobj);
    return (PyObject*)posobj;
}


static PyMethodDef AntGameClient_methods[] = {
    {"connect", (PyCFunction)(void(*)(void))AntGameClient_start, METH_VARARGS | METH_KEYWORDS, "Connect to a server and start running."},
    {"setCallback", AntGameClient_setCallback, METH_VARARGS, "Set a callback function."},
    {"newAnt", AntGameClient_newAnt, METH_VARARGS, "Create a new ant."},
    {"nearestFood", AntGameClient_nearestFood, METH_NOARGS, "Find closest food to nest."},
    {"nearestFreeFood", AntGameClient_nearestUnclaimedFood, METH_NOARGS, "Find closest food to nest that is not targeted by another ant."},
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
    .tp_methods = AntGameClient_methods,
    .tp_members = AntGameClient_members,
    .tp_getset = AntGameClient_getsetters,
    .tp_new = AntGameClient_new,
};


struct NestObject
{
    PyObject_HEAD
    ;
    AntGameClientObject*root;
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
    Py_XINCREF(posobj);
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


struct AntTypeObject
{
    PyObject_HEAD
    ;
    unsigned char type;
};


static PyObject* AntType_getmaxfood(PyObject*op, void*closure)
{
    AntTypeObject* self = (AntTypeObject*)op;
    if (!RoundSettings::instance)
    {
        PyErr_SetString(PyExc_RuntimeError, "Cannot get any attributes of an ant type before starting game, when the server has not told us the settings!");
        return nullptr;
    }
    return PyFloat_FromDouble(Ant::antTypes[self->type].capacity * RoundSettings::instance->capacityMod);
}


static PyObject* AntType_getmaxhealth(PyObject*op, void*closure)
{
    AntTypeObject* self = (AntTypeObject*)op;
    if (!RoundSettings::instance)
    {
        PyErr_SetString(PyExc_RuntimeError, "Cannot get any attributes of an ant type before starting game, when the server has not told us the settings!");
        return nullptr;
    }
    return PyFloat_FromDouble(Ant::antTypes[self->type].healthMod * RoundSettings::instance->antHealth);
}


static PyObject* AntType_getdamage(PyObject*op, void*closure)
{
    AntTypeObject* self = (AntTypeObject*)op;
    if (!RoundSettings::instance)
    {
        PyErr_SetString(PyExc_RuntimeError, "Cannot get any attributes of an ant type before starting game, when the server has not told us the settings!");
        return nullptr;
    }
    return PyFloat_FromDouble(Ant::antTypes[self->type].damageMod * RoundSettings::instance->attackDamage);
}


static PyObject* AntType_getcost(PyObject*op, void*closure)
{
    AntTypeObject* self = (AntTypeObject*)op;
    if (!RoundSettings::instance)
    {
        PyErr_SetString(PyExc_RuntimeError, "Cannot get any attributes of an ant type before starting game, when the server has not told us the settings!");
        return nullptr;
    }
    return PyFloat_FromDouble(Ant::antTypes[self->type].costMod * RoundSettings::instance->antCost);
}


static PyObject* AntType_getspeed(PyObject*op, void*closure)
{
    AntTypeObject* self = (AntTypeObject*)op;
    if (!RoundSettings::instance)
    {
        PyErr_SetString(PyExc_RuntimeError, "Cannot get any attributes of an ant type before starting game, when the server has not told us the settings!");
        return nullptr;
    }
    return PyFloat_FromDouble(Ant::antTypes[self->type].speedMod * RoundSettings::instance->movementSpeed);
}


static PyObject* AntType_getattackrange(PyObject*op, void*closure)
{
    AntTypeObject* self = (AntTypeObject*)op;
    if (!RoundSettings::instance)
    {
        PyErr_SetString(PyExc_RuntimeError, "Cannot get any attributes of an ant type before starting game, when the server has not told us the settings!");
        return nullptr;
    }
    return PyFloat_FromDouble(Ant::antTypes[self->type].rangeMod * RoundSettings::instance->attackRange);
}


static PyObject* AntType_getpickuprange(PyObject*op, void*closure)
{
    AntTypeObject* self = (AntTypeObject*)op;
    if (!RoundSettings::instance)
    {
        PyErr_SetString(PyExc_RuntimeError, "Cannot get any attributes of an ant type before starting game, when the server has not told us the settings!");
        return nullptr;
    }
    return PyFloat_FromDouble(Ant::antTypes[self->type].rangeMod * RoundSettings::instance->pickupRange);
}


static PyGetSetDef AntType_getsetters[] = {
    {"maxfood", AntType_getmaxfood, nullptr, "ant type's maximum food", nullptr},
    {"maxhealth", AntType_getmaxhealth, nullptr, "ant type's maximum health", nullptr},
    {"damage", AntType_getdamage, nullptr, "ant type's damage", nullptr},
    {"cost", AntType_getcost, nullptr, "ant type's cost", nullptr},
    {"speed", AntType_getspeed, nullptr, "ant type's speed", nullptr},
    {"attackrange", AntType_getattackrange, nullptr, "ant type's attack range", nullptr},
    {"pickuprange", AntType_getpickuprange, nullptr, "ant type's pickup range", nullptr},
    {nullptr}
};


static PyObject* AntType_new(PyTypeObject* type, PyObject* args, PyObject *kwds)
{
    AntTypeObject* self;
    self = (AntTypeObject*)type->tp_alloc(type, 0);
    if (!self)
    {
        return nullptr;
    }
    long val = 0xff;
    unsigned char t = 0;
    if (PyArg_ParseTuple(args, "|l", &val))
    {
        if (val >= 0 && val < 0xff)
        {
            t = val;
        }
        self->type = t;
        return (PyObject*)self;
    }
    return nullptr;
}


static PyObject* AntType_richcompare(PyObject* op, PyObject* other, int operation)
{
    AntTypeObject* self = (AntTypeObject*)op;
    unsigned char thisval = self->type;
    unsigned char otherval;
    if (PyObject_TypeCheck(other, Py_TYPE(op)))
    {
        AntTypeObject* otherO = (AntTypeObject*)other;
        otherval = otherO->type;
    }
    else if (PyLong_Check(other))
    {
        unsigned long otherv = PyLong_AsUnsignedLong(other);
        if (otherv >= 0xff)
        {
            otherval = 0xff;
        }
        else
        {
            otherval = otherv;
        }
    }
    else
    {
        if (operation == Py_EQ)
        {
            Py_RETURN_FALSE;
        }
        else if (operation == Py_NE)
        {
            Py_RETURN_TRUE;
        }
        Py_RETURN_NOTIMPLEMENTED;
    }
    if (operation == Py_EQ)
    {
        if (otherval == thisval)
        {
            Py_RETURN_TRUE;
        }
        Py_RETURN_FALSE;
    }
    else if (operation == Py_NE)
    {
        if (otherval == thisval)
        {
            Py_RETURN_FALSE;
        }
        Py_RETURN_TRUE;
    }
    Py_RETURN_NOTIMPLEMENTED;
}


static PyTypeObject AntTypeType = {
    .ob_base = PyVarObject_HEAD_INIT(nullptr, 0)
    .tp_name = "AntGame.AntType",
    .tp_basicsize = sizeof(AntTypeObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    // TODO
    .tp_doc = PyDoc_STR("Placerholder doc string"),
    .tp_richcompare = AntType_richcompare,
    .tp_getset = AntType_getsetters,
    .tp_new = AntType_new,
};


struct AntObject
{
    PyObject_HEAD
    ;
    AntGameClientObject*root;
    unsigned int antID;
    Ant echo;
};


static PyObject* Ant_gettype(PyObject* op, void*closure)
{
    AntObject* self = (AntObject*)op;
    AntTypeObject*typeobj = (AntTypeObject*)AntTypeType.tp_alloc(&AntTypeType, 0);
    if (!self->root || self->antID == 0xffffffff)
    {
        if (self->echo.type != 0xff)
        {
            typeobj->type = self->echo.type;
        }
    }
    else
    {
        AntGameClientObject* root = (AntGameClientObject*)self->root;
        if (!root->map || root->map->antPermanents.size() <= self->antID || !root->map->antPermanents[self->antID])
        {
            if (PyErr_WarnEx(PyExc_RuntimeWarning, "This ant is dead! Accessing its type may cause issues.", 2) < 0)
            {
                return nullptr;
            }
            typeobj->type = 0;
        }
        typeobj->type = root->map->antPermanents[self->antID]->type;
    }
    return (PyObject*)typeobj;
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
    if (!self->root || self->antID == 0xffffffff)
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
    Py_XINCREF(posobj);
    if (posobj)
    {
        if (!self->root || self->antID == 0xffffffff)
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
        else
        {
            AntGameClientObject*root = (AntGameClientObject*)self->root;
            if (!root->map || root->map->antPermanents.size() <= self->antID || !root->map->antPermanents[self->antID])
            {
                posobj->p.x = 0; posobj->p.y = 0;
                if (PyErr_WarnEx(PyExc_RuntimeWarning, "This ant is dead! Accessing its position may cause issues.", 2) < 0)
                {
                    Py_XDECREF(posobj);
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


static PyObject* Ant_getisFriend(PyObject* op, void*closure)
{
    AntObject*self = (AntObject*)op;
    if (!self->root || !self->root->map || self->antID == UINT_MAX)
    {
        if (self->echo.type != 0xff && self->echo.parent == self->root->map->nests[self->root->selfNestID])
        {
            Py_RETURN_TRUE;
        }
    }
    if (self->antID >= self->root->map->antPermanents.size() || !self->root->map->antPermanents[self->antID] || self->root->map->antPermanents[self->antID]->parent != self->root->map->nests[self->root->selfNestID])
    {
        Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}


static PyObject* Ant_getisEnemy(PyObject* op, void*closure)
{
    AntObject*self = (AntObject*)op;
    if (!self->root || !self->root->map || self->antID == UINT_MAX)
    {
        if (self->echo.type != 0xff && self->echo.parent != self->root->map->nests[self->root->selfNestID])
        {
            Py_RETURN_TRUE;
        }
    }
    if (self->antID >= self->root->map->antPermanents.size() || !self->root->map->antPermanents[self->antID] || self->root->map->antPermanents[self->antID]->parent == self->root->map->nests[self->root->selfNestID])
    {
        Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}


static PyObject* Ant_getindex(PyObject* op, void*closure)
{
    AntObject*self = (AntObject*)op;
    if (self)
    {
        return PyLong_FromUnsignedLong((unsigned long)self->antID);
    }
    return PyLong_FromUnsignedLong((unsigned long)UINT_MAX);
}


static PyObject* Ant_getisFull(PyObject* op, void*closure)
{
    AntObject*self = (AntObject*)op;
    if (!self->root || !self->root->map || self->antID == UINT_MAX)
    {
        if (self->echo.type != 0xff && Ant::antTypes[self->echo.type].capacity * RoundSettings::instance->capacityMod - self->echo.foodCarry < 0.001)
        {
            Py_RETURN_TRUE;
        }
        Py_RETURN_FALSE;
    }
    if (self->antID >= self->root->map->antPermanents.size() || !self->root->map->antPermanents[self->antID])
    {
        Py_RETURN_FALSE;
    }
    if (Ant::antTypes[self->root->map->antPermanents[self->antID]->type].capacity * RoundSettings::instance->capacityMod - self->root->map->antPermanents[self->antID]->foodCarry >= 0.001)
    {
        Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}


static PyObject* Ant_getparent(PyObject* op, void*closure)
{
    AntObject*self = (AntObject*)op;
    NestObject*nestobj = (NestObject*)NestType.tp_alloc(&NestType, 0);
    if (!nestobj)
    {
        return nullptr;
    }
    if (!self->root || !self->root->map)
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "This ant is bugged! Getting its parent is impossible. (Report this error!)", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    else if (self->antID == 0xffffffff)
    {
        nestobj->root = self->root;
        if (self->echo.type != 0xff)
        {
            nestobj->nestID = 0xff;
            for (unsigned char i = 0; i < self->root->map->nests.size(); i++)
            {
                if (self->root->map->nests[i] == self->echo.parent)
                {
                    nestobj->nestID = i;
                    break;
                }
            }
            if (nestobj->nestID != 0xff)
            {
                Py_INCREF(nestobj);
                return (PyObject*)nestobj;
            }
        }
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "This ant is bugged! Getting its parent is impossible. (Report this error!)", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    if (self->root->map->antPermanents.size() <= self->antID || !self->root->map->antPermanents[self->antID])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot get the parent of an ant that died!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    for (unsigned char i = 0; i < self->root->map->nests.size(); i++)
    {
        if (self->root->map->nests[i] == self->root->map->antPermanents[self->antID]->parent)
        {
            nestobj->nestID = i;
            break;
        }
    }
    if (nestobj->nestID != 0xff)
    {
        Py_INCREF(nestobj);
        return (PyObject*)nestobj;
    }
    if (PyErr_WarnEx(PyExc_RuntimeWarning, "This ant is bugged! Getting its parent is impossible. (Report this error!)", 2) < 0)
    {
        return nullptr;
    }
    Py_RETURN_NONE;
}


static PyObject* Ant_gettarget(PyObject* op, void*closure);


static PyObject* Ant_getisAlive(PyObject*op, void*closure)
{
    AntObject*self = (AntObject*)op;
    if (!self->root || !self->root->map)
    {
        Py_RETURN_FALSE;
    }
    if (self->antID == UINT_MAX)
    {
        if (self->echo.type == 0xff)
        {
            Py_RETURN_FALSE;
        }
        else
        {
            Py_RETURN_TRUE;
        }
    }
    if (self->root->map->antPermanents.size() <= self->antID || !self->root->map->antPermanents[self->antID])
    {
        Py_RETURN_FALSE;
    }
    else
    {
        Py_RETURN_TRUE;
    }
}


static PyObject* Ant_getisSafe(PyObject*op, void*closure)
{
    AntObject*self = (AntObject*)op;
    if (!self->root || !self->root->map)
    {
        Py_RETURN_FALSE;
    }
    if (self->antID == UINT_MAX)
    {
        if (self->echo.type == 0xff)
        {
            Py_RETURN_FALSE;
        }
        else
        {
            if (self->root->map->antPermanents.size() <= self->echo.pid || !self->root->map->antPermanents[self->echo.pid])
            {
                Py_RETURN_FALSE;
            }
            else
            {
                Py_RETURN_TRUE;
            }
        }
    }
    if (self->root->map->antPermanents.size() <= self->antID || !self->root->map->antPermanents[self->antID])
    {
        Py_RETURN_FALSE;
    }
    else
    {
        Py_RETURN_TRUE;
    }
}


static PyObject* Ant_getisFrozen(PyObject*op, void*closure)
{
    AntObject*self = (AntObject*)op;
    if (!self->root || !self->root->map)
    {
        Py_RETURN_FALSE;
    }
    if (self->antID == UINT_MAX)
    {
        if (self->echo.type == 0xff)
        {
            Py_RETURN_FALSE;
        }
        else
        {
            Py_RETURN_TRUE;
        }
    }
    Py_RETURN_FALSE;
}


static PyGetSetDef Ant_getsetters[] = {
    {"food", Ant_getfood, nullptr, "ant's food", nullptr},
    {"health", Ant_gethealth, nullptr, "ant's health", nullptr},
    {"type", Ant_gettype, nullptr, "ant's type", nullptr},
    {"pos", Ant_getpos, nullptr, "ant's position", nullptr},
    {"isFriend", Ant_getisFriend, nullptr, "whether ant is a friend", nullptr},
    {"isEnemy", Ant_getisEnemy, nullptr, "whether ant is an enemy", nullptr},
    {"isFull", Ant_getisFull, nullptr, "whether ant has max food", nullptr},
    {"index", Ant_getindex, nullptr, "ant's index in ants list", nullptr},
    {"target", Ant_gettarget, nullptr, "ant's target", nullptr},
    {"parent", Ant_getparent, nullptr, "ant's parent nest", nullptr},
    {"isAlive", Ant_getisAlive, nullptr, "whether ant is alive", nullptr},
    {"isSafe", Ant_getisSafe, nullptr, "whether ant is safe to unfreeze or access", nullptr},
    {"isFrozen", Ant_getisFrozen, nullptr, "whether ant is frozen", nullptr},
    {nullptr}
};


static bool Ant_edible(AntObject* self, PosObject* target, bool far)
{
    if (!self || !target || !self->root || !self->root->map || self->antID >= self->root->map->antPermanents.size() || !self->root->map->antPermanents[self->antID] || self->root->map->antPermanents[self->antID]->parent != self->root->map->nests[self->root->selfNestID])
    {
        return false;
    }
    Pos t = target->p;
    Ant*a = self->root->map->antPermanents[self->antID];
    if (((DPos)t-a->p).magnitude() > RoundSettings::instance->pickupRange * Ant::antTypes[a->type].rangeMod && !far)
    {
        return false;
    }
    if (self->root->map->tileEdible(t))
    {
        if (Ant::antTypes[a->type].capacity * RoundSettings::instance->capacityMod < a->foodCarry + RoundSettings::instance->foodYield)
        {
            return false;
        }
    }
    else if ((*self->root->map)[t] != Map::Tile::NEST || self->root->map->nests[self->root->selfNestID]->p == t || Ant::antTypes[a->type].capacity * RoundSettings::instance->capacityMod < a->foodCarry + RoundSettings::instance->foodTheftYield)
    {
        return false;
    }
    return true;
}


static bool Ant_attackable(AntObject*self, AntObject* target, bool far)
{
    if (!self || !target || !self->root || self->root != target->root || !self->root->map || self->antID >= self->root->map->antPermanents.size() || !self->root->map->antPermanents[self->antID] || self->root->map->antPermanents[self->antID]->parent != self->root->map->nests[self->root->selfNestID] || self->root->map->antPermanents.size() <= target->antID || !self->root->map->antPermanents[target->antID] || self->root->map->antPermanents[target->antID]->parent == self->root->map->nests[self->root->selfNestID])
    {
        return false;
    }
    Ant*a = self->root->map->antPermanents[self->antID];
    Ant*t = self->root->map->antPermanents[target->antID];
    if ((a->p-t->p).magnitude() > RoundSettings::instance->attackRange * Ant::antTypes[a->type].rangeMod && !far)
    {
        return false;
    }
    return true;
}


static bool Ant_sendGoto(AntObject*self, DPos t) // TODO this will be the pathing alg
{
    Ant* a = self->root->map->antPermanents[self->antID];
    if (!a->commands.empty())
    {
        std::string msg;
        msg.append("\0\0\0\x1a\0\0\0\x02\x0c", 9);
        msg.append(ConnectionManager::makeAGNPuint(self->antID));
        msg.push_back('\x04');
        msg.append(ConnectionManager::makeAGNPuint(self->antID));
        msg.append(ConnectionManager::makeAGNPuint(ConnectionManager::makeAGNPshortdouble(t.x)));
        msg.append(ConnectionManager::makeAGNPuint(ConnectionManager::makeAGNPshortdouble(t.y)));
        if (!self->root->conn->send(msg.c_str(), msg.length()))
        {
            AntGameClient_handleConnErrors(self->root->conn);
            return false;
        }
        self->root->reqIDs.push_back(ConnectionManager::RequestID::CANCEL);
        self->root->reqIDs.push_back(ConnectionManager::RequestID::WALK);
        self->root->cmdIDs.push_back(ConnectionManager::RequestID::WALK);
        self->root->cmdpids.push_back(self->antID);
        self->root->cmdargs.push_back(((std::uint64_t)ConnectionManager::makeAGNPshortdouble(t.x)<<32)+(std::uint64_t)ConnectionManager::makeAGNPshortdouble(t.y));
        a->commands.clear();
        Ant::AntCommand acmd;
        acmd.cmd = Ant::AntCommand::ID::MOVE;
        acmd.state = Ant::AntCommand::State::ONGOING;
        acmd.arg = ((std::uint64_t)ConnectionManager::makeAGNPshortdouble(t.x)<<32)+(std::uint64_t)ConnectionManager::makeAGNPshortdouble(t.y);
        a->commands.push_back(acmd);
    }
    else
    {
        std::string msg;
        msg.append("\0\0\0\x15\0\0\0\x01\x04", 9);
        msg.append(ConnectionManager::makeAGNPuint(self->antID));
        msg.append(ConnectionManager::makeAGNPuint(ConnectionManager::makeAGNPshortdouble(t.x)));
        msg.append(ConnectionManager::makeAGNPuint(ConnectionManager::makeAGNPshortdouble(t.y)));
        if (!self->root->conn->send(msg.c_str(), msg.length()))
        {
            AntGameClient_handleConnErrors(self->root->conn);
            return false;
        }
        self->root->reqIDs.push_back(ConnectionManager::RequestID::WALK);
        self->root->cmdIDs.push_back(ConnectionManager::RequestID::WALK);
        self->root->cmdpids.push_back(self->antID);
        self->root->cmdargs.push_back(((std::uint64_t)ConnectionManager::makeAGNPshortdouble(t.x)<<32)+(std::uint64_t)ConnectionManager::makeAGNPshortdouble(t.y));
        Ant::AntCommand acmd;
        acmd.cmd = Ant::AntCommand::ID::MOVE;
        acmd.state = Ant::AntCommand::State::ONGOING;
        acmd.arg = ((std::uint64_t)ConnectionManager::makeAGNPshortdouble(t.x)<<32)+(std::uint64_t)ConnectionManager::makeAGNPshortdouble(t.y);
        a->commands.push_back(acmd);
    }
    return true;
}


static bool Ant_sendTake(AntObject*self, Pos t)
{
    Ant* a = self->root->map->antPermanents[self->antID];
    std::string msg;
    msg.append("\0\0\0\x11\0\0\0\x01\x06", 9);
    msg.append(ConnectionManager::makeAGNPuint(self->antID));
    msg.append(ConnectionManager::makeAGNPushort(t.x));
    msg.append(ConnectionManager::makeAGNPushort(t.y));
    if (!self->root->conn->send(msg.c_str(), msg.length()))
    {
        AntGameClient_handleConnErrors(self->root->conn);
        return false;
    }
    self->root->reqIDs.push_back(ConnectionManager::RequestID::TINTERACT);
    self->root->cmdIDs.push_back(ConnectionManager::RequestID::TINTERACT);
    self->root->cmdpids.push_back(self->antID);
    self->root->cmdargs.push_back(((unsigned int)t.x<<16)+(unsigned int)t.y);
    Ant::AntCommand acmd;
    acmd.cmd = Ant::AntCommand::ID::TINTERACT;
    acmd.state = Ant::AntCommand::State::ONGOING;
    acmd.arg = ((unsigned int)t.x<<16)+(unsigned int)t.y;
    a->commands.push_back(acmd);
    return true;
}


static bool Ant_sendAttack(AntObject*self, AntObject*target)
{
    Ant* a = self->root->map->antPermanents[self->antID];
    std::string msg;
    msg.append("\0\0\0\x11\0\0\0\x01\x07", 9);
    msg.append(ConnectionManager::makeAGNPuint(self->antID));
    msg.append(ConnectionManager::makeAGNPuint(target->antID));
    if (!self->root->conn->send(msg.c_str(), msg.length()))
    {
        AntGameClient_handleConnErrors(self->root->conn);
        return false;
    }
    self->root->reqIDs.push_back(ConnectionManager::RequestID::AINTERACT);
    self->root->cmdIDs.push_back(ConnectionManager::RequestID::AINTERACT);
    self->root->cmdpids.push_back(self->antID);
    self->root->cmdargs.push_back(target->antID);
    Ant::AntCommand acmd;
    acmd.cmd = Ant::AntCommand::ID::AINTERACT;
    acmd.state = Ant::AntCommand::State::ONGOING;
    acmd.arg = target->antID;
    a->commands.push_back(acmd);
    return true;
}


static bool Ant_sendFollow(AntObject*self, DPos t, AntObject*target) // TODO this will be the pathing alg
{
    Ant* a = self->root->map->antPermanents[self->antID];
    if (!a->commands.empty())
    {
        std::string msg;
        msg.append("\0\0\0\x1a\0\0\0\x02\x0c", 9);
        msg.append(ConnectionManager::makeAGNPuint(self->antID));
        msg.push_back('\x04');
        msg.append(ConnectionManager::makeAGNPuint(self->antID));
        msg.append(ConnectionManager::makeAGNPuint(ConnectionManager::makeAGNPshortdouble(t.x)));
        msg.append(ConnectionManager::makeAGNPuint(ConnectionManager::makeAGNPshortdouble(t.y)));
        if (!self->root->conn->send(msg.c_str(), msg.length()))
        {
            AntGameClient_handleConnErrors(self->root->conn);
            return false;
        }
        self->root->reqIDs.push_back(ConnectionManager::RequestID::CANCEL);
        self->root->reqIDs.push_back(ConnectionManager::RequestID::WALK);
        self->root->cmdIDs.push_back(ConnectionManager::RequestID::WALK);
        self->root->cmdpids.push_back(self->antID);
        self->root->cmdargs.push_back(((std::uint64_t)ConnectionManager::makeAGNPshortdouble(t.x)<<32)+(std::uint64_t)ConnectionManager::makeAGNPshortdouble(t.y));
        a->commands.clear();
        Ant::AntCommand acmd;
        acmd.cmd = Ant::AntCommand::ID::FOLLOW;
        acmd.state = Ant::AntCommand::State::ONGOING;
        acmd.arg = target->antID;
        a->commands.push_back(acmd);
    }
    else
    {
        std::string msg;
        msg.append("\0\0\0\x15\0\0\0\x01\x04", 9);
        msg.append(ConnectionManager::makeAGNPuint(self->antID));
        msg.append(ConnectionManager::makeAGNPuint(ConnectionManager::makeAGNPshortdouble(t.x)));
        msg.append(ConnectionManager::makeAGNPuint(ConnectionManager::makeAGNPshortdouble(t.y)));
        if (!self->root->conn->send(msg.c_str(), msg.length()))
        {
            AntGameClient_handleConnErrors(self->root->conn);
            return false;
        }
        self->root->reqIDs.push_back(ConnectionManager::RequestID::WALK);
        self->root->cmdIDs.push_back(ConnectionManager::RequestID::WALK);
        self->root->cmdpids.push_back(self->antID);
        self->root->cmdargs.push_back(((std::uint64_t)ConnectionManager::makeAGNPshortdouble(t.x)<<32)+(std::uint64_t)ConnectionManager::makeAGNPshortdouble(t.y));
        Ant::AntCommand acmd;
        acmd.cmd = Ant::AntCommand::ID::FOLLOW;
        acmd.state = Ant::AntCommand::State::ONGOING;
        acmd.arg = target->antID;
        a->commands.push_back(acmd);
    }
    return true;
}


static bool AntGameClient_continueFollow(AntGameClientObject* agc, unsigned int self, unsigned int other)
{
    Ant* a = agc->map->antPermanents[self];
    Ant* t = agc->map->antPermanents[other];
    std::string msg;
    msg.append("\0\0\0\x1a\0\0\0\x02\x0c", 9);
    msg.append(ConnectionManager::makeAGNPuint(self));
    msg.push_back('\x04');
    msg.append(ConnectionManager::makeAGNPuint(self));
    msg.append(ConnectionManager::makeAGNPuint(ConnectionManager::makeAGNPshortdouble(t->p.x)));
    msg.append(ConnectionManager::makeAGNPuint(ConnectionManager::makeAGNPshortdouble(t->p.y)));
    if (!agc->conn->send(msg.c_str(), msg.length()))
    {
        AntGameClient_handleConnErrors(agc->conn);
        return false;
    }
    agc->reqIDs.push_back(ConnectionManager::RequestID::CANCEL);
    agc->reqIDs.push_back(ConnectionManager::RequestID::WALK);
    agc->cmdIDs.push_back(ConnectionManager::RequestID::WALK);
    agc->cmdpids.push_back(self);
    agc->cmdargs.push_back(((std::uint64_t)ConnectionManager::makeAGNPshortdouble(t->p.x)<<32)+(std::uint64_t)ConnectionManager::makeAGNPshortdouble(t->p.y));
    a->commands.clear();
    Ant::AntCommand acmd;
    acmd.cmd = Ant::AntCommand::ID::FOLLOW;
    acmd.state = Ant::AntCommand::State::ONGOING;
    acmd.arg = other;
    a->commands.push_back(acmd);
    return true;
}


static bool AntGameClient_continueFollowAttack(AntGameClientObject* agc, unsigned int self, unsigned int other)
{
    Ant* a = agc->map->antPermanents[self];
    Ant* t = agc->map->antPermanents[other];
    std::string msg;
    msg.append("\0\0\0\x23\0\0\0\x03\x0c", 9);
    msg.append(ConnectionManager::makeAGNPuint(self));
    msg.push_back('\x04');
    msg.append(ConnectionManager::makeAGNPuint(self));
    msg.append(ConnectionManager::makeAGNPuint(ConnectionManager::makeAGNPshortdouble(t->p.x)));
    msg.append(ConnectionManager::makeAGNPuint(ConnectionManager::makeAGNPshortdouble(t->p.y)));
    msg.push_back('\x07');
    msg.append(ConnectionManager::makeAGNPuint(self));
    msg.append(ConnectionManager::makeAGNPuint(other));
    if (!agc->conn->send(msg.c_str(), msg.length()))
    {
        AntGameClient_handleConnErrors(agc->conn);
        return false;
    }
    agc->reqIDs.push_back(ConnectionManager::RequestID::CANCEL);
    agc->reqIDs.push_back(ConnectionManager::RequestID::WALK);
    agc->cmdIDs.push_back(ConnectionManager::RequestID::WALK);
    agc->cmdpids.push_back(self);
    agc->cmdargs.push_back(((std::uint64_t)ConnectionManager::makeAGNPshortdouble(t->p.x)<<32)+(std::uint64_t)ConnectionManager::makeAGNPshortdouble(t->p.y));
    a->commands.clear();
    Ant::AntCommand acmd;
    acmd.cmd = Ant::AntCommand::ID::CFOLLOW;
    acmd.state = Ant::AntCommand::State::ONGOING;
    acmd.arg = other;
    a->commands.push_back(acmd);

    agc->reqIDs.push_back(ConnectionManager::RequestID::AINTERACT);
    agc->cmdIDs.push_back(ConnectionManager::RequestID::AINTERACT);
    agc->cmdpids.push_back(self);
    agc->cmdargs.push_back(other);
    acmd.cmd = Ant::AntCommand::ID::AINTERACT;
    acmd.state = Ant::AntCommand::State::ONGOING;
    acmd.arg = other;
    a->commands.push_back(acmd);
    return true;
}


static bool Ant_sendFollowAttack(AntObject*self, DPos t, AntObject*target) // TODO this will be the pathing alg
{
    Ant* a = self->root->map->antPermanents[self->antID];
    if (!a->commands.empty())
    {
        std::string msg;
        msg.append("\0\0\0\x1a\0\0\0\x02\x0c", 9);
        msg.append(ConnectionManager::makeAGNPuint(self->antID));
        msg.push_back('\x04');
        msg.append(ConnectionManager::makeAGNPuint(self->antID));
        msg.append(ConnectionManager::makeAGNPuint(ConnectionManager::makeAGNPshortdouble(t.x)));
        msg.append(ConnectionManager::makeAGNPuint(ConnectionManager::makeAGNPshortdouble(t.y)));
        if (!self->root->conn->send(msg.c_str(), msg.length()))
        {
            AntGameClient_handleConnErrors(self->root->conn);
            return false;
        }
        self->root->reqIDs.push_back(ConnectionManager::RequestID::CANCEL);
        self->root->reqIDs.push_back(ConnectionManager::RequestID::WALK);
        self->root->cmdIDs.push_back(ConnectionManager::RequestID::WALK);
        self->root->cmdpids.push_back(self->antID);
        self->root->cmdargs.push_back(((std::uint64_t)ConnectionManager::makeAGNPshortdouble(t.x)<<32)+(std::uint64_t)ConnectionManager::makeAGNPshortdouble(t.y));
        a->commands.clear();
        Ant::AntCommand acmd;
        acmd.cmd = Ant::AntCommand::ID::CFOLLOW;
        acmd.state = Ant::AntCommand::State::ONGOING;
        acmd.arg = target->antID;
        a->commands.push_back(acmd);
    }
    else
    {
        std::string msg;
        msg.append("\0\0\0\x15\0\0\0\x01\x04", 9);
        msg.append(ConnectionManager::makeAGNPuint(self->antID));
        msg.append(ConnectionManager::makeAGNPuint(ConnectionManager::makeAGNPshortdouble(t.x)));
        msg.append(ConnectionManager::makeAGNPuint(ConnectionManager::makeAGNPshortdouble(t.y)));
        if (!self->root->conn->send(msg.c_str(), msg.length()))
        {
            AntGameClient_handleConnErrors(self->root->conn);
            return false;
        }
        self->root->reqIDs.push_back(ConnectionManager::RequestID::WALK);
        self->root->cmdIDs.push_back(ConnectionManager::RequestID::WALK);
        self->root->cmdpids.push_back(self->antID);
        self->root->cmdargs.push_back(((std::uint64_t)ConnectionManager::makeAGNPshortdouble(t.x)<<32)+(std::uint64_t)ConnectionManager::makeAGNPshortdouble(t.y));
        Ant::AntCommand acmd;
        acmd.cmd = Ant::AntCommand::ID::CFOLLOW;
        acmd.state = Ant::AntCommand::State::ONGOING;
        acmd.arg = target->antID;
        a->commands.push_back(acmd);
    }
    if (!Ant_sendAttack(self, target))
    {
        return false;
    }
    return true;
}


static PyObject* AntGameClient_newAnt(PyObject*op, PyObject*args)
{
    AntGameClientObject* self = (AntGameClientObject*)op;
    if (!self->map)
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot make a new ant when the game has not started!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    if (self->map->nests[self->selfNestID]->ants.size() >= 255)
    {
        Py_RETURN_NONE;
    }
    PyObject* arg = nullptr;
    if (PyArg_ParseTuple(args, "O:newAnt", &arg))
    {
        if (!arg)
        {
            PyErr_SetString(PyExc_TypeError, "newAnt function needs an argument");
            return nullptr;
        }
        if (PyObject_TypeCheck(arg, &AntTypeType))
        {
            AntTypeObject* ato = (AntTypeObject*)arg;
            std::string msg;
            msg.append("\0\0\0\x0a\0\0\0\x01\x08", 9);
            msg.push_back((char)ato->type);
            if (!self->conn->send(msg.c_str(), msg.length()))
            {
                AntGameClient_handleConnErrors(self->conn);
                return nullptr;
            }
            self->reqIDs.push_back(ConnectionManager::RequestID::NEWANT);
            self->cmdIDs.push_back(ConnectionManager::RequestID::NEWANT);
            self->cmdpids.push_back(UINT_MAX);
            self->cmdargs.push_back(ato->type);
        }
        else if (PyLong_Check(arg))
        {
            unsigned long val = PyLong_AsUnsignedLong(arg);
            if (PyErr_Occurred())
            {
                PyErr_Clear();
                PyErr_SetString(PyExc_ValueError, "newAnt function's argument must be a valid ant type!");
                return nullptr;
            }
            if (val >= Ant::antTypec)
            {
                PyErr_SetString(PyExc_ValueError, "newAnt function's argument must be a valid ant type!");
                return nullptr;
            }
            std::string msg;
            msg.append("\0\0\0\x0a\0\0\0\x01\x08", 9);
            msg.push_back((char)(unsigned char)val);
            if (!self->conn->send(msg.c_str(), msg.length()))
            {
                AntGameClient_handleConnErrors(self->conn);
                return nullptr;
            }
            self->reqIDs.push_back(ConnectionManager::RequestID::NEWANT);
            self->cmdIDs.push_back(ConnectionManager::RequestID::NEWANT);
            self->cmdpids.push_back(UINT_MAX);
            self->cmdargs.push_back((char)(unsigned char)val);
        }
        else
        {
            PyErr_SetString(PyExc_TypeError, "newAnt function's argument must be a valid ant type!");
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    return nullptr;
}


static PyObject* Ant_move(PyObject* op, PyObject* args)
{
    AntObject* self = (AntObject*)op;
    if (!self->root || self->antID >= self->root->map->antPermanents.size() || !self->root->map->antPermanents[self->antID])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot move a dead ant!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    else if (self->root->map->antPermanents[self->antID]->parent != self->root->map->nests[self->root->selfNestID])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot move an ant that isn't ours!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    PyObject* arg = nullptr;
    if (PyArg_ParseTuple(args, "O:move", &arg))
    {
        if (!arg)
        {
            PyErr_SetString(PyExc_TypeError,"The argument to the move function must be a position!");
            return nullptr;
        }
        if (arg == Py_None)
        {
            Py_RETURN_NONE;
        }
        if (PyObject_TypeCheck(arg, &PosType) == 0)
        {
            PyErr_SetString(PyExc_TypeError,"The argument to the move function must be a position!");
            return nullptr;
        }
        PosObject* posobj = (PosObject*)arg;
        DPos p = posobj->p;
        if (p.x > self->root->map->size.x || p.y > self->root->map->size.y)
        {
            if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot move an ant to a spot outside the map!", 2) < 0)
            {
                return nullptr;
            }
            Py_RETURN_NONE;
        }
        if (!Ant_sendGoto(self, p))
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    return nullptr;
}


static PyObject* Ant_take(PyObject* op, PyObject* args)
{
    AntObject* self = (AntObject*)op;
    if (!self->root || self->antID >= self->root->map->antPermanents.size() || !self->root->map->antPermanents[self->antID])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot take with a dead ant!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    else if (self->root->map->antPermanents[self->antID]->parent != self->root->map->nests[self->root->selfNestID])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot take with an ant that isn't ours!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    PyObject* arg = nullptr;
    if (PyArg_ParseTuple(args, "O:take", &arg))
    {
        if (!arg)
        {
            PyErr_SetString(PyExc_TypeError,"The argument to the take function must be a position!");
            return nullptr;
        }
        if (arg == Py_None)
        {
            Py_RETURN_NONE;
        }
        if (PyObject_TypeCheck(arg, &PosType) == 0)
        {
            PyErr_SetString(PyExc_TypeError,"The argument to the take function must be a position!");
            return nullptr;
        }
        PosObject* posobj = (PosObject*)arg;
        DPos p = posobj->p;
        if (p.x > self->root->map->size.x || p.y > self->root->map->size.y)
        {
            if(PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot take food on a spot outside the map!", 2) < 0)
            {
                return nullptr;
            }
            Py_RETURN_NONE;
        }
        Pos t = p;
        if (!Ant_edible(self, posobj, false))
        {
            Py_RETURN_NONE; // Silently exiting is better than a warning because food getting nabbed by another ant is not uncommon
        }
        if (!Ant_sendTake(self, t))
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    return nullptr;
}


static PyObject* Ant_attack(PyObject* op, PyObject* args);


static PyObject* Ant_goTake(PyObject* op, PyObject* args)
{
    AntObject* self = (AntObject*)op;
    if (!self->root || self->antID >= self->root->map->antPermanents.size() || !self->root->map->antPermanents[self->antID])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot move a dead ant!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    else if (self->root->map->antPermanents[self->antID]->parent != self->root->map->nests[self->root->selfNestID])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot move an ant that isn't ours!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    PyObject* arg = nullptr;
    if (PyArg_ParseTuple(args, "O:move", &arg))
    {
        if (!arg)
        {
            PyErr_SetString(PyExc_TypeError,"The argument to the goTake function must be a position!");
            return nullptr;
        }
        if (arg == Py_None)
        {
            Py_RETURN_NONE;
        }
        if (PyObject_TypeCheck(arg, &PosType) == 0)
        {
            PyErr_SetString(PyExc_TypeError,"The argument to the goTake function must be a position!");
            return nullptr;
        }
        PosObject* posobj = (PosObject*)arg;
        DPos p = posobj->p;
        if (p.x >= self->root->map->size.x || p.y >= self->root->map->size.y)
        {
            if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot move an ant to a spot outside the map!", 2) < 0)
            {
                return nullptr;
            }
            Py_RETURN_NONE;
        }
        if (!Ant_edible(self, posobj, true))
        {
            Py_RETURN_NONE;
        }
        Pos t = posobj->p;
        bool found = false;
        if (!Ant_sendGoto(self, p))
        {
            return nullptr;
        }
        if (!Ant_sendTake(self, t))
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    return nullptr;
}


static PyObject* Ant_deliver(PyObject* op, PyObject* args)
{
    AntObject* self = (AntObject*)op;
    if (!self->root || self->antID >= self->root->map->antPermanents.size() || !self->root->map->antPermanents[self->antID])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot move a dead ant!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    else if (self->root->map->antPermanents[self->antID]->parent != self->root->map->nests[self->root->selfNestID])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot move an ant that isn't ours!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    Pos t = self->root->map->nests[self->root->selfNestID]->p;
    if (!Ant_sendTake(self, t))
    {
        return nullptr;
    }
    Py_RETURN_NONE;
}


static PyObject* Ant_goDeliver(PyObject* op, PyObject* args)
{
    AntObject* self = (AntObject*)op;
    if (!self->root || self->antID >= self->root->map->antPermanents.size() || !self->root->map->antPermanents[self->antID])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot move a dead ant!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    else if (self->root->map->antPermanents[self->antID]->parent != self->root->map->nests[self->root->selfNestID])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot move an ant that isn't ours!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    Pos t = self->root->map->nests[self->root->selfNestID]->p;
    DPos p = t;
    if (!Ant_sendGoto(self, p))
    {
        return nullptr;
    }
    if (!Ant_sendTake(self, t))
    {
        return nullptr;
    }
    Py_RETURN_NONE;
}


static PyObject* Ant_stop(PyObject* op, PyObject* args)
{
    AntObject* self = (AntObject*)op;
    if (!self->root || self->antID >= self->root->map->antPermanents.size() || !self->root->map->antPermanents[self->antID])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot stop a dead ant!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    else if (self->root->map->antPermanents[self->antID]->parent != self->root->map->nests[self->root->selfNestID])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot stop an ant that isn't ours!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    std::string msg;
    msg.append("\0\0\0\x0d\0\0\0\x01\x0c", 9);
    msg.append(ConnectionManager::makeAGNPuint(self->antID));
    if (!self->root->conn->send(msg.c_str(), msg.length()))
    {
        AntGameClient_handleConnErrors(self->root->conn);
        return nullptr;
    }
    self->root->reqIDs.push_back(ConnectionManager::RequestID::CANCEL);
    self->root->map->antPermanents[self->antID]->commands.clear();
    Py_RETURN_NONE;
}



static PyObject* Ant_goAttack(PyObject* op, PyObject* args);


static PyObject* Ant_follow(PyObject* op, PyObject* args);
static PyObject* Ant_followAttack(PyObject* op, PyObject* args);


static PyObject* Ant_nearestFood(PyObject* op, PyObject* args)
{
    AntObject* self = (AntObject*)op;
    Pos p;
    if (!self->root || !self->root->map || self->antID == UINT_MAX || self->antID >= self->root->map->antPermanents.size() || !self->root->map->antPermanents[self->antID])
    {
        if (self->echo.type != 0xff && self->echo.p.x != USHRT_MAX && self->echo.p.y != USHRT_MAX)
        {
            p = self->echo.p;
        }
        else
        {
            if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot get nearest food to dead ant!", 2) < 0)
            {
                return nullptr;
            }
            Py_RETURN_NONE;
        }
    }
    else
    {
        p = self->root->map->antPermanents[self->antID]->p;
    }
    Pos t = AntGameClient_findNearestFood(self->root, p, AntGameClient_isFood);
    if (t.x == USHRT_MAX || t.y == USHRT_MAX)
    {
        Py_RETURN_NONE;
    }
    PosObject*posobj = (PosObject*)PosType.tp_alloc(&PosType, 0);
    if (posobj)
    {
        posobj->p = t;
    }
    Py_XINCREF(posobj);
    return (PyObject*)posobj;
}


static PyObject* Ant_nearestFreeFood(PyObject* op, PyObject* args)
{
    AntObject* self = (AntObject*)op;
    Pos p;
    if (!self->root || !self->root->map || self->antID == UINT_MAX || self->antID >= self->root->map->antPermanents.size() || !self->root->map->antPermanents[self->antID])
    {
        if (self->echo.type != 0xff && self->echo.p.x != USHRT_MAX && self->echo.p.y != USHRT_MAX)
        {
            p = self->echo.p;
        }
        else
        {
            if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot get nearest food to dead ant!", 2) < 0)
            {
                return nullptr;
            }
            Py_RETURN_NONE;
        }
    }
    else
    {
        p = self->root->map->antPermanents[self->antID]->p;
    }
    Pos t = AntGameClient_findNearestFood(self->root, p, AntGameClient_isUnclaimedFood);
    if (t.x == USHRT_MAX || t.y == USHRT_MAX)
    {
        Py_RETURN_NONE;
    }
    PosObject*posobj = (PosObject*)PosType.tp_alloc(&PosType, 0);
    if (posobj)
    {
        posobj->p = t;
    }
    Py_XINCREF(posobj);
    return (PyObject*)posobj;
}


static PyObject* Ant_freeze(PyObject* op, PyObject* args)
{
    AntObject* self = (AntObject*)op;
    if (!self->root || !self->root->map || self->antID == UINT_MAX || self->root->map->antPermanents.size() <= self->antID || !self->root->map->antPermanents[self->antID])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Tried to freeze ant that is either dead or already frozen! The freeze function will do nothing. This may cause issues.", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    self->echo = *self->root->map->antPermanents[self->antID];
    self->antID = UINT_MAX;
    Py_RETURN_NONE;
}


static PyObject* Ant_unfreeze(PyObject* op, PyObject* args)
{
    AntObject* self = (AntObject*)op;
    if (!self->root || !self->root->map)
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Tried to unfreeze bugged ant!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    if (self->antID != UINT_MAX)
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Tried to unfreeze an already unfrozen ant! The unfreeze function will do nothing.", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    if (self->echo.type == 0xff)
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Tried to unfreeze a dead ant! The unfreeze function will do nothing. This may cause issues", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    if (self->echo.pid >= self->root->map->antPermanents.size() || !self->root->map->antPermanents[self->echo.pid])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Tried to unfreeze a dead ant! The unfreeze function will do nothing. This may cause issues", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    self->antID = self->echo.pid;
    self->echo.type = 0xff;
    Py_RETURN_NONE;
}


static PyObject* Ant_nearestAnt(PyObject* op, PyObject* args);
static PyObject* Ant_nearestFriend(PyObject* op, PyObject* args);
static PyObject* Ant_nearestEnemy(PyObject* op, PyObject* args);


static PyMethodDef Ant_methods[] = {
    {"move", Ant_move, METH_VARARGS, "Move an ant."},
    {"take", Ant_take, METH_VARARGS, "Take food."},
    {"goTake", Ant_goTake, METH_VARARGS, "Move an ant to take food."},
    {"attack", Ant_attack, METH_VARARGS, "Attack an enemy."},
    {"goAttack", Ant_goAttack, METH_VARARGS, "Move an ant to attack an enemy."},
    {"follow", Ant_follow, METH_VARARGS, "Follow an ant."},
    {"followAttack", Ant_followAttack, METH_VARARGS, "Follow an ant and attack it."},
    {"deliver", Ant_deliver, METH_NOARGS, "Deliver all food."},
    {"goDeliver", Ant_goDeliver, METH_NOARGS, "Walk to deliver all food."},
    {"nearestFood", Ant_nearestFood, METH_NOARGS, "Find the position of the closest food."},
    {"nearestFreeFood", Ant_nearestFreeFood, METH_NOARGS, "Find the position of the closest food no friend is targeting."},
    {"nearestEnemy", Ant_nearestEnemy, METH_NOARGS, "Find the nearest enemy."},
    {"nearestFriend", Ant_nearestFriend, METH_NOARGS, "Find the nearest friend."},
    {"nearestAnt", Ant_nearestAnt, METH_NOARGS, "Find the nearest ant."},
    {"stop", Ant_stop, METH_NOARGS, "Stop all commands."},
    {"freeze", Ant_freeze, METH_NOARGS, "Take a frozen snapshot of this ant."},
    {"unfreeze", Ant_unfreeze, METH_NOARGS, "Try to recover an ant from a snapshot."},
    {nullptr, nullptr, 0, nullptr}
};


static PyObject* Ant_richcompare(PyObject* op, PyObject* other, int operation)
{
    AntObject* self = (AntObject*)op;
    unsigned int thisval = self->antID;
    unsigned int otherval;
    if (PyObject_TypeCheck(other, Py_TYPE(op)))
    {
        AntObject* otherO = (AntObject*)other;
        otherval = otherO->antID;
    }
    else
    {
        if (operation == Py_EQ)
        {
            Py_RETURN_FALSE;
        }
        else if (operation == Py_NE)
        {
            Py_RETURN_TRUE;
        }
    }
    if (operation == Py_EQ)
    {
        if (otherval == thisval)
        {
            Py_RETURN_TRUE;
        }
        Py_RETURN_FALSE;
    }
    else if (operation == Py_NE)
    {
        if (otherval == thisval)
        {
            Py_RETURN_FALSE;
        }
        Py_RETURN_TRUE;
    }
    Py_RETURN_NOTIMPLEMENTED;
}


static PyTypeObject AntType = {
    .ob_base = PyVarObject_HEAD_INIT(nullptr, 0)
    .tp_name = "AntGame.Ant",
    .tp_basicsize = sizeof(AntObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    //TODO
    .tp_doc = PyDoc_STR("Placeholder doc string"),
    .tp_richcompare = Ant_richcompare,
    .tp_methods = Ant_methods,
    .tp_getset = Ant_getsetters,
    .tp_new = PyType_GenericNew,
};


static PyObject* Ant_gettarget(PyObject* op, void*closure)
{
    AntObject*self = (AntObject*)op;
    if (!self->root || !self->root->map || self->antID == UINT_MAX || self->root->map->antPermanents.size() <= self->antID || !self->root->map->antPermanents[self->antID])
    {
        Py_RETURN_NONE;
    }
    unsigned int other = UINT_MAX;
    for (Ant::AntCommand acmd : self->root->map->antPermanents[self->antID]->commands)
    {
        if (acmd.cmd == Ant::AntCommand::ID::FOLLOW || acmd.cmd == Ant::AntCommand::ID::CFOLLOW || acmd.cmd == Ant::AntCommand::ID::AINTERACT)
        {
            other = acmd.arg;
            break;
        }
    }
    if (other == UINT_MAX || other >= self->root->map->antPermanents.size() || !self->root->map->antPermanents[other])
    {
        Py_RETURN_NONE;
    }
    AntObject* item = (AntObject*)AntType.tp_alloc(&AntType, 0);
    if (!item)
    {
        return nullptr;
    }
    item->root = self->root;
    item->antID = other;
    item->echo.type = 0xff;
    Py_INCREF(item);
    return (PyObject*)item;
}


static PyObject* Ant_attack(PyObject* op, PyObject* args)
{
    AntObject* self = (AntObject*)op;
    if (!self->root || self->antID >= self->root->map->antPermanents.size() || !self->root->map->antPermanents[self->antID])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot attack with a dead ant!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    else if (self->root->map->antPermanents[self->antID]->parent != self->root->map->nests[self->root->selfNestID])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot attack with an ant that isn't ours!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    PyObject* arg = nullptr;
    if (PyArg_ParseTuple(args, "O:take", &arg))
    {
        if (!arg)
        {
            PyErr_SetString(PyExc_TypeError,"The argument to the attack function must be an ant!");
            return nullptr;
        }
        if (arg == Py_None)
        {
            Py_RETURN_NONE;
        }
        if (PyObject_TypeCheck(arg, &AntType) == 0)
        {
            PyErr_SetString(PyExc_TypeError,"The argument to the attack function must be an ant!");
            return nullptr;
        }
        AntObject* target = (AntObject*)arg;
        if (!Ant_attackable(self, target, false))
        {
            Py_RETURN_NONE; // Silently exiting is better than a warning because ants moving out of range is not uncommon
        }
        if (!Ant_sendAttack(self, target))
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    return nullptr;
}


static PyObject* Ant_goAttack(PyObject* op, PyObject* args)
{
    AntObject* self = (AntObject*)op;
    if (!self->root || self->antID >= self->root->map->antPermanents.size() || !self->root->map->antPermanents[self->antID])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot attack with a dead ant!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    else if (self->root->map->antPermanents[self->antID]->parent != self->root->map->nests[self->root->selfNestID])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot take with an ant that isn't ours!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    PyObject* arg = nullptr;
    if (PyArg_ParseTuple(args, "O:take", &arg))
    {
        if (!arg)
        {
            PyErr_SetString(PyExc_TypeError,"The argument to the goAttack function must be an ant!");
            return nullptr;
        }
        if (arg == Py_None)
        {
            Py_RETURN_NONE;
        }
        if (PyObject_TypeCheck(arg, &AntType) == 0)
        {
            PyErr_SetString(PyExc_TypeError,"The argument to the goAttack function must be an ant!");
            return nullptr;
        }
        AntObject* target = (AntObject*)arg;
        if (!Ant_attackable(self, target, true))
        {
            Py_RETURN_NONE; // Silently exiting is better than a warning because ants moving out of range is not uncommon
        }
        DPos p = self->root->map->antPermanents[target->antID]->p;
        if (!Ant_sendGoto(self, p))
        {
            return nullptr;
        }
        if (!Ant_sendAttack(self, target))
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    return nullptr;
}


static PyObject* Ant_follow(PyObject* op, PyObject* args)
{
    AntObject* self = (AntObject*)op;
    if (!self->root || self->antID >= self->root->map->antPermanents.size() || !self->root->map->antPermanents[self->antID])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot move a dead ant!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    else if (self->root->map->antPermanents[self->antID]->parent != self->root->map->nests[self->root->selfNestID])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot move an ant that isn't ours!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    PyObject* arg = nullptr;
    if (PyArg_ParseTuple(args, "O:move", &arg))
    {
        if (!arg)
        {
            PyErr_SetString(PyExc_TypeError,"The argument to the follow function must be an ant!");
            return nullptr;
        }
        if (arg == Py_None)
        {
            Py_RETURN_NONE;
        }
        if (PyObject_TypeCheck(arg, &AntType) == 0)
        {
            PyErr_SetString(PyExc_TypeError,"The argument to the follow function must be an ant!");
            return nullptr;
        }
        AntObject* target = (AntObject*)arg;
        if (!Ant_attackable(self, target, true))
        {
            Py_RETURN_NONE; // Silently exiting is better than a warning because ants moving out of range is not uncommon
        }
        DPos p = self->root->map->antPermanents[target->antID]->p;
        if (!Ant_sendFollow(self, p, target))
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    return nullptr;
}


static PyObject* Ant_followAttack(PyObject* op, PyObject* args)
{
    AntObject* self = (AntObject*)op;
    if (!self->root || self->antID >= self->root->map->antPermanents.size() || !self->root->map->antPermanents[self->antID])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot move a dead ant!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    else if (self->root->map->antPermanents[self->antID]->parent != self->root->map->nests[self->root->selfNestID])
    {
        if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot move an ant that isn't ours!", 2) < 0)
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    PyObject* arg = nullptr;
    if (PyArg_ParseTuple(args, "O:move", &arg))
    {
        if (!arg)
        {
            PyErr_SetString(PyExc_TypeError,"The argument to the follow function must be an ant!");
            return nullptr;
        }
        if (arg == Py_None)
        {
            Py_RETURN_NONE;
        }
        if (PyObject_TypeCheck(arg, &AntType) == 0)
        {
            PyErr_SetString(PyExc_TypeError,"The argument to the follow function must be an ant!");
            return nullptr;
        }
        AntObject* target = (AntObject*)arg;
        if (!Ant_attackable(self, target, true))
        {
            Py_RETURN_NONE; // Silently exiting is better than a warning because ants moving out of range is not uncommon
        }
        DPos p = self->root->map->antPermanents[target->antID]->p;
        if (!Ant_sendFollowAttack(self, p, target))
        {
            return nullptr;
        }
        Py_RETURN_NONE;
    }
    return nullptr;
}


static PyObject* Ant_nearestEnemy(PyObject* op, PyObject* args)
{
    AntObject* self = (AntObject*)op;
    DPos p;
    if (!self->root || !self->root->map || self->antID == UINT_MAX || self->antID >= self->root->map->antPermanents.size() || !self->root->map->antPermanents[self->antID])
    {
        if (self->echo.type != 0xff && self->echo.p.x != USHRT_MAX && self->echo.p.y != USHRT_MAX)
        {
            p = self->echo.p;
        }
        else
        {
            if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot get nearest food to dead ant!", 2) < 0)
            {
                return nullptr;
            }
            Py_RETURN_NONE;
        }
    }
    else
    {
        p = self->root->map->antPermanents[self->antID]->p;
    }
    Ant*a = nullptr;
    double bestDist = -1;
    for (Ant*i : self->root->map->antPermanents)
    {
        if (!i)
        {
            continue;
        }
        if (i->pid != self->antID && i->parent != self->root->map->nests[self->root->selfNestID] && (a == nullptr || (p.x-i->p.x)*(p.x-i->p.x)+(p.y-i->p.y)*(p.y-i->p.y) < bestDist))
        {
            a = i;
            bestDist = (p.x-i->p.x)*(p.x-i->p.x)+(p.y-i->p.y)*(p.y-i->p.y);
        }
    }
    if (!a)
    {
        Py_RETURN_NONE;
    }
    AntObject*antobj = (AntObject*)AntType.tp_alloc(&AntType, 0);
    if (antobj)
    {
        antobj->echo.type = 0xff;
        antobj->antID = a->pid;
        antobj->root = self->root;
    }
    Py_XINCREF(antobj);
    return (PyObject*)antobj;
}


static PyObject* Ant_nearestFriend(PyObject* op, PyObject* args)
{
    AntObject* self = (AntObject*)op;
    DPos p;
    if (!self->root || !self->root->map || self->antID == UINT_MAX || self->antID >= self->root->map->antPermanents.size() || !self->root->map->antPermanents[self->antID])
    {
        if (self->echo.type != 0xff && self->echo.p.x != USHRT_MAX && self->echo.p.y != USHRT_MAX)
        {
            p = self->echo.p;
        }
        else
        {
            if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot get nearest food to dead ant!", 2) < 0)
            {
                return nullptr;
            }
            Py_RETURN_NONE;
        }
    }
    else
    {
        p = self->root->map->antPermanents[self->antID]->p;
    }
    Ant*a = nullptr;
    double bestDist = -1;
    for (Ant*i : self->root->map->antPermanents)
    {
        if (!i)
        {
            continue;
        }
        if (i->pid != self->antID && i->parent == self->root->map->nests[self->root->selfNestID] && (a == nullptr || (p.x-i->p.x)*(p.x-i->p.x)+(p.y-i->p.y)*(p.y-i->p.y) < bestDist))
        {
            a = i;
            bestDist = (p.x-i->p.x)*(p.x-i->p.x)+(p.y-i->p.y)*(p.y-i->p.y);
        }
    }
    if (!a)
    {
        Py_RETURN_NONE;
    }
    AntObject*antobj = (AntObject*)AntType.tp_alloc(&AntType, 0);
    if (antobj)
    {
        antobj->echo.type = 0xff;
        antobj->antID = a->pid;
        antobj->root = self->root;
    }
    Py_XINCREF(antobj);
    return (PyObject*)antobj;
}


static PyObject* Ant_nearestAnt(PyObject* op, PyObject* args)
{
    AntObject* self = (AntObject*)op;
    DPos p;
    if (!self->root || !self->root->map || self->antID == UINT_MAX || self->antID >= self->root->map->antPermanents.size() || !self->root->map->antPermanents[self->antID])
    {
        if (self->echo.type != 0xff && self->echo.p.x != USHRT_MAX && self->echo.p.y != USHRT_MAX)
        {
            p = self->echo.p;
        }
        else
        {
            if (PyErr_WarnEx(PyExc_RuntimeWarning, "Cannot get nearest food to dead ant!", 2) < 0)
            {
                return nullptr;
            }
            Py_RETURN_NONE;
        }
    }
    else
    {
        p = self->root->map->antPermanents[self->antID]->p;
    }
    Ant*a = nullptr;
    double bestDist = -1;
    for (Ant*i : self->root->map->antPermanents)
    {
        if (!i)
        {
            continue;
        }
        if (i->pid != self->antID && (a == nullptr || (p.x-i->p.x)*(p.x-i->p.x)+(p.y-i->p.y)*(p.y-i->p.y) < bestDist))
        {
            a = i;
            bestDist = (p.x-i->p.x)*(p.x-i->p.x)+(p.y-i->p.y)*(p.y-i->p.y);
        }
    }
    if (!a)
    {
        Py_RETURN_NONE;
    }
    AntObject*antobj = (AntObject*)AntType.tp_alloc(&AntType, 0);
    if (antobj)
    {
        antobj->echo.type = 0xff;
        antobj->antID = a->pid;
        antobj->root = self->root;
    }
    Py_XINCREF(antobj);
    return (PyObject*)antobj;
}


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
        item->root = self;
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
        item->root = self;
        item->antID = i;
        item->echo.type = 0xff;
        Py_INCREF(item);
        PyList_SetItem(ret, j, (PyObject*)item);
        j++;
    }
    return ret;
}


static PyObject* AntGameClient_getenemies(PyObject* op, void *closure)
{
    AntGameClientObject*self = (AntGameClientObject*)op;
    if (self->map == nullptr || self->map->antPermanents.empty() || self->selfNestID == 0xff)
    {
        return PyList_New(0);
    }
    unsigned int amount = 0;
    for (Ant*a : self->map->antPermanents)
    {
        if (a && a->parent != self->map->nests[self->selfNestID])
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
        if (self->map->nests[self->selfNestID] == self->map->antPermanents[i]->parent)
        {
            continue;
        }
        AntObject* item = (AntObject*)AntType.tp_alloc(&AntType, 0);
        if (!item)
        {
            Py_DECREF(ret);
            return nullptr;
        }
        item->root = self;
        item->antID = i;
        item->echo.type = 0xff;
        Py_INCREF(item);
        PyList_SetItem(ret, j, (PyObject*)item);
        j++;
    }
    return ret;
}


static bool AntGameClient_callAntCallback(AntGameClientObject* self, PyObject*func, Ant* a)
{
    if (!a)
    {
        PyErr_SetString(PyExc_RuntimeError, "A null ant was passed to an ant callback. This is not your fault. Report the error!");
        return false;
    }
    AntObject* item = (AntObject*)AntType.tp_alloc(&AntType, 0);
    if (!item)
    {
        return false;
    }
    if (self && self->map->antPermanents.size() > a->pid && self->map->antPermanents[a->pid])
    {
        item->root = self;
        item->antID = a->pid;
        item->echo.type = 0xff;
    }
    else
    {
        item->root = self;
        item->antID = 0xffffffff;
        item->echo = *a;
    }
    return AntGameClient_callAntCallback(func, (PyObject*)item);
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
    nobj->root = self;
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
    if (PyType_Ready(&AntTypeType) < 0)
    {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "AntType", (PyObject*)&AntTypeType) < 0)
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



// CODE YOU COULD EDIT BELOW HERE


static bool AntGameClient_isFood(AntGameClientObject* self, Pos t)
{
    if (!self->map)
    {
        return false;
    }
    return self->map->tileEdible(t);
}


static bool AntGameClient_isUnclaimedFood(AntGameClientObject* self, Pos t)
{
    if (!self->map)
    {
        return false;
    }
    if (!self->map->tileEdible(t))
    {
        return false;
    }
    for (Ant*a : self->map->nests[self->selfNestID]->ants)
    {
        for (Ant::AntCommand acmd : a->commands)
        {
            if (acmd.cmd == Ant::AntCommand::ID::TINTERACT && acmd.arg == ((unsigned int)t.x<<16)+(unsigned int)t.y)
            {
                return false;
            }
        }
    }
    return true;
}


static Pos AntGameClient_findNearestFood(AntGameClientObject* self, Pos center, bool (*checker)(AntGameClientObject*, Pos))
{
    Pos check = center;
    if (checker(self, check))
    {
        return check;
    }
    check.x += 1;
    if (checker(self, check))
    {
        return check;
    }
    check.y -= 1;
    if (checker(self, check))
    {
        return check;
    }
    check.y += 2;
    if (checker(self, check))
    {
        return check;
    }
    check.x -= 1;
    if (checker(self, check))
    {
        return check;
    }
    check.y -= 2;
    if (checker(self, check))
    {
        return check;
    }
    check.x -= 1;
    if (checker(self, check))
    {
        return check;
    }
    check.y += 1;
    if (checker(self, check))
    {
        return check;
    }
    check.y += 1;
    if (checker(self, check))
    {
        return check;
    }

    unsigned int gap = 5;
    std::uint64_t current = 4;
    unsigned short count = 2;
    std::deque<unsigned int> gaps;
    std::deque<std::uint64_t> currents;
    std::deque<unsigned short> counts;
    gaps.push_back(5);
    currents.push_back(5);
    counts.push_back(2);
    if (center == self->map->nests[self->selfNestID]->p)
    {
        gap = self->nearestFoodData.gap;
        current = self->nearestFoodData.current;
        count = self->nearestFoodData.count;
        currents = self->nearestFoodData.currents;
        counts = self->nearestFoodData.counts;
        gaps = self->nearestFoodData.gaps;
    }
    while (count < center.x+1 || count < self->map->size.x-center.x || count < center.y+1 || count < self->map->size.y-center.y)
    {
        auto bestCurrentsIt = currents.end();
        auto bestCountsIt = counts.end();
        auto bestGapsIt = gaps.end();
        unsigned short bestIndex = -1;
        std::uint64_t best = -1;
        {
        auto currentsIt = currents.begin();
        auto countsIt = counts.begin();
        auto gapsIt = gaps.begin();
        unsigned short index = 1;
        for (;currentsIt != currents.end();currentsIt++)
        {
            if (*currentsIt < best && *countsIt <= index)
            {
                best = *currentsIt;
                bestCurrentsIt = currentsIt;
                bestCountsIt = countsIt;
                bestGapsIt = gapsIt;
                bestIndex = index;
            }
            index++;
            countsIt++;
            gapsIt++;
        }
        if (bestIndex == USHRT_MAX || current < best)
        {
            check.x = center.x + count;
            check.y = center.y;
            gaps.push_back(3);
            currents.push_back(current+1);
            counts.push_back(1);
            current += gap;
            gap += 2;
            if (self->map->size.x - center.x > count && checker(self, check))
            {
                if (center == self->map->nests[self->selfNestID]->p)
                {
                    self->nearestFoodData.gap = gap;
                    self->nearestFoodData.current = current;
                    self->nearestFoodData.count = count+1;
                    self->nearestFoodData.currents = currents;
                    self->nearestFoodData.counts = counts;
                    self->nearestFoodData.gaps = gaps;
                }
                return check;
            }
            check.x = center.x - count;
            if (center.x >= count && checker(self, check))
            {
                if (center == self->map->nests[self->selfNestID]->p)
                {
                    self->nearestFoodData.gap = gap;
                    self->nearestFoodData.current = current;
                    self->nearestFoodData.count = count+1;
                    self->nearestFoodData.currents = currents;
                    self->nearestFoodData.counts = counts;
                    self->nearestFoodData.gaps = gaps;
                }
                return check;
            }
            check.x = center.x;
            check.y = center.y - count;
            if (center.y >= count && checker(self, check))
            {
                if (center == self->map->nests[self->selfNestID]->p)
                {
                    self->nearestFoodData.gap = gap;
                    self->nearestFoodData.current = current;
                    self->nearestFoodData.count = count+1;
                    self->nearestFoodData.currents = currents;
                    self->nearestFoodData.counts = counts;
                    self->nearestFoodData.gaps = gaps;
                }
                return check;
            }
            check.y = center.y + count;
            if (self->map->size.y - center.y > count && checker(self, check))
            {
                if (center == self->map->nests[self->selfNestID]->p)
                {
                    self->nearestFoodData.gap = gap;
                    self->nearestFoodData.current = current;
                    self->nearestFoodData.count = count+1;
                    self->nearestFoodData.currents = currents;
                    self->nearestFoodData.counts = counts;
                    self->nearestFoodData.gaps = gaps;
                }
                return check;
            }
            count++;
        }
        else
        {
            check.x = bestIndex;
            check.y = *bestCountsIt;
            *bestCurrentsIt += *bestGapsIt;
            *bestGapsIt += 2;
            if (self->map->size.x - center.x > check.x && self->map->size.y - center.y > check.y && checker(self, center+check))
            {
                if (center == self->map->nests[self->selfNestID]->p)
                {
                    (*bestCountsIt)++;
                    self->nearestFoodData.gap = gap;
                    self->nearestFoodData.current = current;
                    self->nearestFoodData.count = count;
                    self->nearestFoodData.currents = currents;
                    self->nearestFoodData.counts = counts;
                    self->nearestFoodData.gaps = gaps;
                }
                return center+check;
            }
            if (center.x >= check.x && center.y >= check.y && checker(self, center-check))
            {
                if (center == self->map->nests[self->selfNestID]->p)
                {
                    (*bestCountsIt)++;
                    self->nearestFoodData.gap = gap;
                    self->nearestFoodData.current = current;
                    self->nearestFoodData.count = count;
                    self->nearestFoodData.currents = currents;
                    self->nearestFoodData.counts = counts;
                    self->nearestFoodData.gaps = gaps;
                }
                return center-check;
            }
            if (self->map->size.x - center.x > check.x && center.y >= check.y && checker(self, {(unsigned short)(center.x+check.x), (unsigned short)(center.y-check.y)}))
            {
                if (center == self->map->nests[self->selfNestID]->p)
                {
                    (*bestCountsIt)++;
                    self->nearestFoodData.gap = gap;
                    self->nearestFoodData.current = current;
                    self->nearestFoodData.count = count;
                    self->nearestFoodData.currents = currents;
                    self->nearestFoodData.counts = counts;
                    self->nearestFoodData.gaps = gaps;
                }
                return {(unsigned short)(center.x+check.x), (unsigned short)(center.y-check.y)};
            }
            if (center.x >= check.x && self->map->size.y - center.y > check.y && checker(self, {(unsigned short)(center.x-check.x), (unsigned short)(center.y+check.y)}))
            {
                if (center == self->map->nests[self->selfNestID]->p)
                {
                    (*bestCountsIt)++;
                    self->nearestFoodData.gap = gap;
                    self->nearestFoodData.current = current;
                    self->nearestFoodData.count = count;
                    self->nearestFoodData.currents = currents;
                    self->nearestFoodData.counts = counts;
                    self->nearestFoodData.gaps = gaps;
                }
                return {(unsigned short)(center.x-check.x), (unsigned short)(center.y+check.y)};
            }
            check.y = bestIndex;
            check.x = *bestCountsIt;
            if (self->map->size.x - center.x > check.x && self->map->size.y - center.y > check.y && checker(self, center+check))
            {
                if (center == self->map->nests[self->selfNestID]->p)
                {
                    (*bestCountsIt)++;
                    self->nearestFoodData.gap = gap;
                    self->nearestFoodData.current = current;
                    self->nearestFoodData.count = count;
                    self->nearestFoodData.currents = currents;
                    self->nearestFoodData.counts = counts;
                    self->nearestFoodData.gaps = gaps;
                }
                return center+check;
            }
            if (center.x >= check.x && center.y >= check.y && checker(self, center-check))
            {
                if (center == self->map->nests[self->selfNestID]->p)
                {
                    (*bestCountsIt)++;
                    self->nearestFoodData.gap = gap;
                    self->nearestFoodData.current = current;
                    self->nearestFoodData.count = count;
                    self->nearestFoodData.currents = currents;
                    self->nearestFoodData.counts = counts;
                    self->nearestFoodData.gaps = gaps;
                }
                return center-check;
            }
            if (self->map->size.x - center.x > check.x && center.y >= check.y && checker(self, {(unsigned short)(center.x+check.x), (unsigned short)(center.y-check.y)}))
            {
                if (center == self->map->nests[self->selfNestID]->p)
                {
                    (*bestCountsIt)++;
                    self->nearestFoodData.gap = gap;
                    self->nearestFoodData.current = current;
                    self->nearestFoodData.count = count;
                    self->nearestFoodData.currents = currents;
                    self->nearestFoodData.counts = counts;
                    self->nearestFoodData.gaps = gaps;
                }
                return {(unsigned short)(center.x+check.x), (unsigned short)(center.y-check.y)};
            }
            if (center.x >= check.x && self->map->size.y - center.y > check.y && checker(self, {(unsigned short)(center.x-check.x), (unsigned short)(center.y+check.y)}))
            {
                if (center == self->map->nests[self->selfNestID]->p)
                {
                    (*bestCountsIt)++;
                    self->nearestFoodData.gap = gap;
                    self->nearestFoodData.current = current;
                    self->nearestFoodData.count = count;
                    self->nearestFoodData.currents = currents;
                    self->nearestFoodData.counts = counts;
                    self->nearestFoodData.gaps = gaps;
                }
                return {(unsigned short)(center.x-check.x), (unsigned short)(center.y+check.y)};
            }
            (*bestCountsIt)++;
        }
        }
    }
    check.x = -1;
    check.y = -1;
    if (center == self->map->nests[self->selfNestID]->p)
    {
        self->nearestFoodData.gap = gap;
        self->nearestFoodData.current = current;
        self->nearestFoodData.count = count;
        self->nearestFoodData.currents = currents;
        self->nearestFoodData.counts = counts;
        self->nearestFoodData.gaps = gaps;
    }
    return check;
}
