// Copyright (C) 2013 PiB <pixelbound@gmail.com>
//  
// EQuilibre is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include <stdio.h>
#include <QTimer>
#include "EQuilibre/Network/Launcher.h"
#include "EQuilibre/Core/Log.h"
#include "EQuilibre/Core/Message.h"
#include "EQuilibre/Core/MessageStructs.h"

Launcher::Launcher(Log *log, QObject *parent) : QObject(parent)
{
    const int TICK_DURATION_MS = 10;
    m_sequence = 0;
    m_loginPort = 0;
    m_stage = eLaunchEnterLogin;
    m_destZoneID = 0;
    m_timeoutMs = 5000;
    m_log = log;
    m_clock = new Clock(TICK_DURATION_MS, log);
    m_loginTimer = m_clock->create(m_timeoutMs, "Login timeout");
    m_playTimer = m_clock->create(m_timeoutMs, "Play timeout");
    m_worldLoginTimer = m_clock->create(m_timeoutMs * 3, "World login timeout");
    m_worldEntryTimer = m_clock->create(m_timeoutMs, "World entry timeout");
    m_loginConn = new LoginClient(m_clock, log);
    m_worldConn = new WorldClient(m_clock, log);
    m_zoneConn = new ZoneClient(m_clock, log);
    
    connect(m_loginConn, SIGNAL(stateChanged(ConnectionState)),
            this, SLOT(handleLoginStateChange(ConnectionState)));
    connect(m_loginTimer, SIGNAL(elapsed()),
            this, SLOT(toLoginEntry()));
    connect(m_playTimer, SIGNAL(elapsed()),
            this, SLOT(toServerSelection()));
    connect(m_loginConn, SIGNAL(messageReceived(Message&)),
            this, SLOT(handleLoginMessage(Message&)));
    connect(m_worldConn, SIGNAL(stateChanged(ConnectionState)),
            this, SLOT(handleWorldStateChange(ConnectionState)));
    connect(m_worldConn, SIGNAL(messageReceived(Message&)),
            this, SLOT(handleWorldMessage(Message&)));
    connect(m_worldLoginTimer, SIGNAL(elapsed()),
            this, SLOT(toServerSelection()));
    connect(m_worldEntryTimer, SIGNAL(elapsed()),
            this, SLOT(toCharacterSelection()));
    connect(m_zoneConn, SIGNAL(stateChanged(ConnectionState)),
            this, SLOT(handleZoneStateChange(ConnectionState)));
    connect(m_zoneConn, SIGNAL(messageReceived(Message&)),
            this, SLOT(handleZoneMessage(Message&)));
}

Launcher::~Launcher()
{
    delete m_zoneConn;
    delete m_worldConn;
    delete m_loginConn;
    delete m_clock;
}

LaunchStage Launcher::stage() const
{
    return m_stage;
}

void Launcher::setStage(LaunchStage newStage)
{
    if(newStage != m_stage)
    {
        qDebug("Changing stage: %s -> %s", stageName(m_stage), stageName(newStage));
        m_stage = newStage;
        emit launchStageChanged();
    }
}

ServerList & Launcher::servers()
{
    return m_servers;
}

CharacterList & Launcher::characters()
{
    return m_characters;
}

QString Launcher::currentCharName() const
{
    return m_currentCharName;
}

Clock * Launcher::clock() const
{
    return m_clock;
}

Log * Launcher::log() const
{
    return m_log;   
}

void Launcher::setLog(Log *newLog)
{
    m_log = newLog;
}

QString Launcher::userName() const
{
    return m_user;
}

void Launcher::setUserName(QString newName)
{
    m_user = newName;
}

QString Launcher::password() const
{
    return m_password;
}

void Launcher::setPassword(QString newPassword)
{
    m_password = newPassword;
}

QHostAddress Launcher::loginHost() const
{
    return m_loginHost;
}

void Launcher::setLoginHost(QHostAddress newHost)
{
    m_loginHost = newHost;
}

bool Launcher::setLoginHost(QString newHost)
{
    QHostAddress hostAddr;
    if(!hostAddr.setAddress(newHost))
        return false;
    m_loginHost = hostAddr;
    return true;
}

int Launcher::loginPort() const
{
    return m_loginPort;
}

void Launcher::setLoginPort(int newPort)
{
    m_loginPort = newPort;
}

bool Launcher::isChangingZones() const
{
    return m_destZoneID != 0;
}

LaunchStage Launcher::interactiveStageFor(LaunchStage stage)
{
    switch(stage)
    {
    default:
    case eLaunchEnterLogin:
    case eLaunchSendingLogin:
        return eLaunchEnterLogin;
    case eLaunchSelectServer:
    case eLaunchRequestingPlay:
    case eLaunchEnteringWorld:
        return eLaunchSelectServer;
    case eLaunchEnteredWorld:
    case eLaunchSelectCharacter:
    case eLaunchEnteringZone:
        return eLaunchSelectCharacter;
    case eLaunchLoadingZone:
    case eLaunchEnteredZone:
    case eLaunchCamping:
    case eLaunchChangingZone:
    case eLaunchLeavingZone:
    case eLaunchLeftZone:
        return eLaunchEnteredZone;
    }
}

#define LAUNCH_STAGE(stage) case stage: return #stage;

const char * Launcher::stageName(LaunchStage stage)
{
    switch(stage)
    {
    default:
        return "eLaunchInvalid";
    LAUNCH_STAGE(eLaunchEnterLogin)
    LAUNCH_STAGE(eLaunchSendingLogin)
    LAUNCH_STAGE(eLaunchSelectServer)
    LAUNCH_STAGE(eLaunchRequestingPlay)
    LAUNCH_STAGE(eLaunchEnteringWorld)
    LAUNCH_STAGE(eLaunchEnteredWorld)
    LAUNCH_STAGE(eLaunchSelectCharacter)
    LAUNCH_STAGE(eLaunchEnteringZone)
    LAUNCH_STAGE(eLaunchLoadingZone)
    LAUNCH_STAGE(eLaunchEnteredZone)
    LAUNCH_STAGE(eLaunchCamping)
    LAUNCH_STAGE(eLaunchChangingZone)
    LAUNCH_STAGE(eLaunchLeavingZone)
    LAUNCH_STAGE(eLaunchLeftZone)
    }
}

#undef LAUNCH_STAGE

static bool canSendMessageAt(LaunchStage stage)
{
    switch(stage)
    {
    default:
        return false;
    case eLaunchLoadingZone:
    case eLaunchEnteredZone:
    case eLaunchCamping:
    case eLaunchLeavingZone:
    case eLaunchChangingZone:
        return true;
    }
}

NetworkResult Launcher::send(Message &msg)
{
    if(!canSendMessageAt(m_stage))
        return NetDisconnected;
    return m_zoneConn->send(msg);
}

NetworkResult Launcher::sendEmpty(MessageID id)
{
    if(!canSendMessageAt(m_stage))
        return NetDisconnected;
    return m_zoneConn->sendEmpty(id);
}

void Launcher::handleLoginMessage(Message &response)
{
    MessageID responseID = response.id();
    response.setHandled(true);
    if(responseID == AM_ChatMessage)
    {
        m_loginConn->login(m_user, m_password);
    }
    else if(responseID == AM_LoginAccepted)
    {
        // Waiting for a login response.
        const uint32_t invalidUserID = 0xffffffff;
        LoginAcceptedMessage *msg = response.as<LoginAcceptedMessage>();
        if(!msg || (msg->userID == invalidUserID) || (msg->status != 1))
        {
            if(m_log)
                m_log->writeLine("Failed to log in.");
            toLoginEntry();
        }
        else
        {
            if(m_log)
                m_log->writeLine("Successfully logged in.");
            m_sessionKey = msg->key;
            m_loginConn->listServers();
        }
    }
    else if(responseID == AM_ServerListResponse)
    {
        // Waiting for a server list response.
        ServerListResponseMessage *msg = response.as<ServerListResponseMessage>();
        if(msg)
        {
            m_servers.clear();
            foreach(ServerInfo server, msg->servers)
                m_servers.append(server);
            m_loginTimer->clear();
            setStage(eLaunchSelectServer);
            emit serverListUpdated();
        }
    }
    else if(responseID == AM_PlayEverquestResponse)
    {
        // Waiting for a play response.
        PlayEverquestResponseMessage *msg = response.as<PlayEverquestResponseMessage>();
        const ServerInfo *server = m_servers.currentServer();
        if(!msg || (msg->allowed != 1))
        {
            if(m_log)
                m_log->writeLine("Play permission denied for server '%s' (ID=%d), reason = %d.",
                                 server->name.toUtf8().constData(), msg->serverID, msg->status);
            toServerSelection();
        }
        else
        {
            if(m_log)
                m_log->writeLine("Play permission granted for server '%s' (ID=%d).",
                                  server->name.toUtf8().constData(), msg->serverID);
            m_playTimer->clear();
            setStage(eLaunchEnteringWorld);
            m_worldLoginTimer->start();
            m_loginConn->disconnect();
            // HACK: wait a bit to let the login server tell the world server
            // about our login.
            QTimer::singleShot(100, this, SLOT(connectToWorldServer()));
        }
    }
    else
    {
        response.setHandled(false);
    }
    emit messageReceived(response);
}

void Launcher::handleWorldMessage(Message &response)
{
    MessageID responseID = response.id();
    response.setHandled(true);
    if(responseID == AM_EnterWorld)
    {
        // Waiting for logging to be complete.
        m_worldLoginTimer->clear();
        setStage(eLaunchEnteredWorld);
        m_worldConn->worldClientReady();
        bool zoning = isChangingZones() && m_characters.setCurrentName(m_currentCharName);
        if(zoning)
        {
            enterWorld();
        }
    }
    else if(responseID == AM_GuildsList)
    {
        qint64 elapsedMs = m_charSelTimer->elapsed();
        double elapsedS = (double)elapsedMs / 1000.0;
        if(m_log)
            m_log->writeLine("Guild list message received after %.2f s.", elapsedS);
    }
    else if(responseID == AM_SendCharInfo)
    {
        // Waiting for character selection.
        SendCharInfoMessage *msg = response.as<SendCharInfoMessage>();
        if(msg)
        {
            m_characters.clear();
            foreach(CharacterInfo charInfo, msg->characters)
                m_characters.append(charInfo);
            if(m_stage == eLaunchEnteredWorld)
            {
                setStage(eLaunchSelectCharacter);    
            }
            emit characterListUpdated();
        }
    }
    else if(responseID == AM_ZoneServerInfo)
    {
        // Waiting for zone server info.
        ZoneServerInfoMessage *msg = response.as<ZoneServerInfoMessage>();
        if(msg)
        {
            if(m_log)
                m_log->writeLine("Entered world, zone server: %s:%d.",
                                 msg->host.toLatin1().constData(), msg->port);
            QHostAddress hostAddr;
            if(hostAddr.setAddress(msg->host))
            {
                m_worldConn->sendEmpty(AM_WorldComplete);
                m_zoneConn->connect(hostAddr, msg->port);
            }
            else
            {
                if(m_log)
                    m_log->writeLine("Failed to parse zone server address.");
                toCharacterSelection();
            }
        }
    }
    else if(responseID == AM_ZoneUnavail)
    {
        // Waiting for zone server info.
        if(m_log)
            m_log->writeLine("The zone is unavailable.");
        setStage(eLaunchSelectCharacter);
    }
    else
    {
        response.setHandled(false);
    }
    emit messageReceived(response);
}

void Launcher::handleZoneMessage(Message &msg)
{
    MessageID id = msg.id();
    if((m_stage == eLaunchEnteringZone) && (id == AM_ZoneEntry))
    {
        m_worldEntryTimer->clear();
        setStage(eLaunchLoadingZone);
    }
    else if(id == AM_ZoneChange)
    {
        ZoneChangeMessage *body = msg.as<ZoneChangeMessage>();
        if(body->status < 0)
        {
            if(m_log)
                m_log->writeLine("Zone change denied, status = %d.", body->status);
            m_destZoneID = 0;
        }
        else if(m_stage != eLaunchChangingZone)
        {
            if(m_log)
                m_log->writeLine("Zone change to zoneID %d granted.", body->zoneID);
            m_destZoneID = body->zoneID;
            setStage(eLaunchChangingZone);
            m_zoneConn->sendEmpty(AM_SaveOnZoneReq);
            m_zoneConn->sendEmpty(AM_DeleteSpawn); // XXX Fill it with client ID.
        }
        msg.setHandled(true);
    }
    else if(id == AM_LogoutReply)
    {
        setStage(eLaunchLeftZone);
        msg.setHandled(true);
    }
    emit messageReceived(msg);
}

void Launcher::connectToLoginServer()
{
    m_loginConn->connect(m_loginHost, m_loginPort);
    setStage(eLaunchSendingLogin);
}

void Launcher::play()
{
    if(m_sessionKey.isEmpty())
    {
        if(m_log)
            m_log->writeLine("Must be logged in to play.");
        return;
    }
    const ServerInfo *server = m_servers.currentServer();
    if(!server)
    {
        if(m_log)
            m_log->writeLine("No server was selected.");
        return;
    }
    setStage(eLaunchRequestingPlay);
    m_playTimer->start();
    if(m_log)
        m_log->writeLine("Requesting play permission for world server '%s' (ID=%d).",
                         server->name.toUtf8().constData(), server->runtimeID);
    m_sequence++;
    m_loginConn->play(server->runtimeID, m_sequence);
}

void Launcher::enterWorld()
{
    const CharacterInfo *character = m_characters.current();
    if(!character)
        return;
    m_currentCharName = character->name;
    m_destZoneID = 0;
    setStage(eLaunchEnteringZone);
    m_worldEntryTimer->start();
    m_worldConn->enterWorld(m_currentCharName, false, false);
}

void Launcher::changeZone(const ZonePoint &point)
{
    if((m_stage != eLaunchEnteredZone) || isChangingZones())
    {
        return;
    }
    qDebug("Requesting zone change (pointID = %d) to zoneID %d (%f %f %f)",
            point.pointID, point.destZoneID, point.destPos.x, point.destPos.y, point.destPos.z);
    m_destZoneID = point.destZoneID;
    
    // Send a zone change request to the server.
    ZoneChangeMessage msg;
    msg.charName = m_currentCharName;
    msg.pos = point.destPos;
    msg.zoneID = m_destZoneID;
    msg.instanceID = point.destInstanceID;
    msg.reason = 0;
    msg.status = 0;
    send(msg);
}

void Launcher::toLoginEntry()
{
    switch(m_stage)
    {
    case eLaunchEnterLogin:
        // Nothing to do.
        break;
    case eLaunchSendingLogin:
    case eLaunchSelectServer:
    case eLaunchRequestingPlay:
        m_loginConn->disconnect();
        setStage(eLaunchEnterLogin);
        break;
    case eLaunchEnteringWorld:
    case eLaunchEnteredWorld:
    case eLaunchSelectCharacter:
        m_worldConn->disconnect();
        setStage(eLaunchEnterLogin);
        break;
    case eLaunchEnteringZone:
    case eLaunchLoadingZone:
    case eLaunchChangingZone:
    case eLaunchLeftZone:
        m_zoneConn->disconnect();
        setStage(eLaunchEnterLogin);
        break;
    case eLaunchEnteredZone:
        setStage(eLaunchCamping);
        break;
    case eLaunchCamping:
    case eLaunchLeavingZone:
        // We need to wait for the eLaunchLeftZone state before disconnecting.
        // XXX Add a timer here to make sure we don't wait forever?
        break;
    }
}

void Launcher::toServerSelection()
{
    switch(m_stage)
    {
    case eLaunchEnterLogin:
        // No can do.
        break;
    case eLaunchSendingLogin:
        // Just wait a bit...
        break;
    case eLaunchSelectServer:
        // Nothing to do.
        break;
    case eLaunchRequestingPlay:
        setStage(eLaunchSelectServer);
        break;
    case eLaunchEnteringWorld:
    case eLaunchEnteredWorld:
    case eLaunchSelectCharacter:
        m_worldConn->disconnect();
        connectToLoginServer();
        break;
    case eLaunchEnteringZone:
    case eLaunchLoadingZone:
    case eLaunchChangingZone:
    case eLaunchLeftZone:
        m_zoneConn->disconnect();
        connectToLoginServer();
        break;
    case eLaunchEnteredZone:
        setStage(eLaunchCamping);
        break;
    case eLaunchCamping:
    case eLaunchLeavingZone:
        // We need to wait for the eLaunchLeftZone state before disconnecting.
        // XXX Add a timer here to make sure we don't wait forever?
        break;
    }
}

void Launcher::toCharacterSelection()
{
    switch(m_stage)
    {
    case eLaunchEnterLogin:
    case eLaunchSendingLogin:
    case eLaunchSelectServer:
        // No can do.
        break;
    case eLaunchRequestingPlay:
    case eLaunchEnteringWorld:
    case eLaunchEnteredWorld:
        // Just wait a bit...
        break;
    case eLaunchSelectCharacter:
        // Nothing to do.
        break;
    case eLaunchEnteringZone:
    case eLaunchLoadingZone:
    case eLaunchChangingZone:
    case eLaunchLeftZone:
        m_zoneConn->disconnect();
        connectToWorldServer();
        break;
    case eLaunchEnteredZone:
        setStage(eLaunchCamping);
        break;
    case eLaunchCamping:
    case eLaunchLeavingZone:
        // We need to wait for the eLaunchLeftZone state before disconnecting.
        // XXX Add a timer here to make sure we don't wait forever?
        break;
    }
}

void Launcher::handleLoginStateChange(ConnectionState newState)
{
    if(newState == eConnConnected)
    {
        if(m_log)
            m_log->writeLine("Connected to the login server.");
        m_loginConn->sessionReady();
        m_loginTimer->start();
    }
    else if(newState == eConnDisconnected)
    {
        if(m_log)
            m_log->writeLine("Disconnected from the login server.");
        if(stage() < eLaunchEnteringWorld)
            toLoginEntry();
    }
}

void Launcher::connectToWorldServer()
{
    const ServerInfo *server = m_servers.currentServer();
    QHostAddress hostAddress(server->host);
    setStage(eLaunchEnteringWorld);
    m_worldConn->connect(hostAddress, 9000);
}

void Launcher::handleWorldStateChange(ConnectionState newState)
{
    if(newState == eConnConnected)
    {
        bool zoning = isChangingZones();
        if(m_log)
            m_log->writeLine("Connected to the world server.");
        m_worldConn->login(m_sequence, m_sessionKey, zoning);
        m_charSelTimer->start();
    }
    else if(newState == eConnDisconnected)
    {
        if(m_log)
            m_log->writeLine("Disconnected from the world server.");
        if(stage() < eLaunchEnteringZone)
            toServerSelection();
    }
}

void Launcher::handleZoneStateChange(ConnectionState newState)
{
    if(newState == eConnConnected)
    {
        const CharacterInfo *character = m_characters.current();
        if(m_log)
            m_log->writeLine("Connected to the zone server.");
        m_zoneConn->enterZone(character->name);
    }
    else if(newState == eConnDisconnected)
    {
        if(m_log)
            m_log->writeLine("Disconnected from the zone server.");
        toCharacterSelection();
    }
}

////////////////////////////////////////////////////////////////////////////////

LoginClient::LoginClient(Clock *clock, Log *log, QObject *parent)
    : Connection(clock, log, parent)
{
}

bool LoginClient::sessionReady()
{
    SessionReadyMessage request;
    request.unknownA = 2;
    request.unknownB = 0;
    request.unknownC = 8;
    request.unknownD = 0;
    return (send(request) == NetSuccess);
}

bool LoginClient::login(QString user, QString password)
{
    LoginMessage request;
    request.unknownA = 3;
    request.unknownB = 2;
    request.unknownC = 0;
    request.user = user;
    request.password = password;
    return (send(request) == NetSuccess);
}

bool LoginClient::listServers()
{
    ServerListRequestMessage request;
    request.unknownA = 4;
    request.unknownB = 0;
    request.unknownC = 0;
    return (send(request) == NetSuccess);
}

bool LoginClient::play(uint32_t serverID, uint32_t sequence)
{
    PlayEverquestRequestMessage request;
    request.sequence = sequence;
    request.unknownA = 0;
    request.unknownB = 0;
    request.serverID = serverID;
    return (send(request) == NetSuccess);
}

////////////////////////////////////////////////////////////////////////////////

WorldClient::WorldClient(Clock *clock, Log *log, QObject *parent)
    : Connection(clock, log, parent)
{
}

bool WorldClient::login(uint32_t sequence, QString sessionKey, bool zoning)
{
    SendLoginInfoMessage request;
    request.sequence = sequence;
    request.sessionKey = sessionKey;
    request.zoning = zoning;
    return (send(request) == NetSuccess);
}

bool WorldClient::enterWorld(QString charName, bool tutorial, bool goHome)
{
    EnterWorldMessage request;
    request.charName = charName;
    request.tutorial = tutorial;
    request.goHome = goHome;
    return (send(request) == NetSuccess);
}

bool WorldClient::worldClientReady()
{
    // XXX Obviously we should wait for an answer after sending each message.
    // The emulator sends all of the responses at the same time so it's
    // difficult to know the ordering of requests and responses here.
    MessageID messages[] = {AM_ApproveWorld,
                            AM_World_Client_CRC1,
                            AM_World_Client_CRC2,
                            AM_WorldClientReady,
                            AM_Invalid};
    bool success = true;
    int i = 0;
    while(success && messages[i])
    {
        success = (sendEmpty(messages[i]) == NetSuccess);
        i++;
    }
    return success;
}

////////////////////////////////////////////////////////////////////////////////

ZoneClient::ZoneClient(Clock *clock, Log *log, QObject *parent)
    : Connection(clock, log, parent)
{
}

bool ZoneClient::enterZone(QString charName)
{
    ZoneEntryMessage request;
    request.unknownA = 0;
    request.charName = charName;
    return (send(request) == NetSuccess);
}
