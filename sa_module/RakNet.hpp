#pragma once

#include "BitStream.hpp"

using RakNetTime = unsigned int;
using RakNetTimeNS = long long;

enum PacketPriority {
  SYSTEM_PRIORITY,
  HIGH_PRIORITY,
  MEDIUM_PRIORITY,
  LOW_PRIORITY,
  NUMBER_OF_PRIORITIES
};

enum PacketReliability {
  UNRELIABLE = 6,
  UNRELIABLE_SEQUENCED,
  RELIABLE,
  RELIABLE_ORDERED,
  RELIABLE_SEQUENCED
};

using PlayerIndex = unsigned short;

#pragma pack(push, 1)
struct PlayerID {
  unsigned int binaryAddress;
  unsigned short port;

  PlayerID& operator =(const PlayerID& input) {
    binaryAddress = input.binaryAddress;
    port = input.port;
    return *this;
  }

  bool operator==(const PlayerID& right) const;
  bool operator!=(const PlayerID& right) const;
  bool operator >(const PlayerID& right) const;
  bool operator <(const PlayerID& right) const;
};

struct Packet {
  PlayerIndex playerIndex;
  PlayerID playerId;
  unsigned int length;
  unsigned int bitSize;
  unsigned char* data;
  bool deleteData;
};

struct RPCParameters {
  unsigned char* input;
  unsigned int numberOfBitsOfData;
  PlayerID sender;
  void* recipient;
  BitStream* replyToSender;
};

struct RakNetStatisticsStruct {
  unsigned messageSendBuffer[NUMBER_OF_PRIORITIES];
  unsigned messagesSent[NUMBER_OF_PRIORITIES];
  unsigned messageDataBitsSent[NUMBER_OF_PRIORITIES];
  unsigned messageTotalBitsSent[NUMBER_OF_PRIORITIES];
  unsigned packetsContainingOnlyAcknowlegements;
  unsigned acknowlegementsSent;
  unsigned acknowlegementsPending;
  unsigned acknowlegementBitsSent;
  unsigned packetsContainingOnlyAcknowlegementsAndResends;

  unsigned messageResends;
  unsigned messageDataBitsResent;
  unsigned messagesTotalBitsResent;
  unsigned messagesOnResendQueue;

  unsigned numberOfUnsplitMessages;
  unsigned numberOfSplitMessages;
  unsigned totalSplits;

  unsigned packetsSent;

  unsigned encryptionBitsSent;
  unsigned totalBitsSent;

  unsigned sequencedMessagesOutOfOrder;
  unsigned sequencedMessagesInOrder;

  unsigned orderedMessagesOutOfOrder;
  unsigned orderedMessagesInOrder;

  unsigned packetsReceived;
  unsigned packetsWithBadCRCReceived;
  unsigned bitsReceived;
  unsigned bitsWithBadCRCReceived;
  unsigned acknowlegementsReceived;
  unsigned duplicateAcknowlegementsReceived;
  unsigned messagesReceived;
  unsigned invalidMessagesReceived;
  unsigned duplicateMessagesReceived;
  unsigned messagesWaitingForReassembly;
  unsigned internalOutputQueueSize;
  double bitsPerSecond;
  RakNetTime connectionStartTime;

  char rakpad[104];
};

struct RakNetList {
  void* array;
  unsigned int size;
  unsigned int alloc_count;
};

struct RakNetQueue {
  void* array;
  unsigned int head;
  unsigned int tail;
  unsigned int alloc_count;
};

struct RakNetOrderedList {
  RakNetList ordered_list;
};

struct RakNetMemoryPool {
  int blocksout;
  RakNetList pool;
};


struct RakNetBPlusTree {
  RakNetMemoryPool page_pool;
  void* root;
  void* leftmost_left;
};

struct ReliabilityLevel {
  RakNetList orderingList;
  RakNetQueue output_queue;
  RakNetOrderedList acknowlegements;
  RakNetTimeNS nextAckTime;
  int splitMessageProgressInterval;
  RakNetTimeNS unreliableTimeout;
  RakNetBPlusTree resendList;
  RakNetQueue resendQueue;
  RakNetQueue sendPacketSet[4];
  RakNetOrderedList splitPacketChannelList;
  unsigned short message_number;
  RakNetTimeNS lastAckTime;
  BitStream updateBitStream;
  unsigned short waitingForOrderedPacketWriteIndex[32];
  unsigned short waitingForSequencedPacketWriteIndex[32];
  unsigned short waitingForOrderedPacketReadIndex[32];
  unsigned short waitingForSequencedPacketReadIndex[32];
  bool deadConnection;
  bool cheater;
  unsigned int ping;
  unsigned short splitPacketId;
  RakNetTime timeoutTime;
  unsigned int blockWindowIncreaseUntilTime;
  RakNetStatisticsStruct stats;
  RakNetQueue hasReceivedPacketQueue;
  unsigned short receivedPacketsBaseIndex;
  char pad0[0x293];
  bool key_set;
  char pad1[0x2DF - 0x293 - 1];
};

struct reliablity {
  RakNetTimeNS nextAckTime;
  int splitMessageProgressInterval;
  RakNetTimeNS unreliableTimeout;
  unsigned short message_number;
  RakNetTimeNS lastAckTime;
  bool deadConnection;
  bool cheater;
  unsigned int ping;
  unsigned short splitPacketId;
  RakNetTime timeoutTime;
  unsigned int blockWindowIncreaseUntilTime;
  unsigned short receivedPacketsBaseIndex;
};

struct RemoteSystemStruct {
  bool isActive;
  PlayerID playerId;
  PlayerID myExternalPlayerId;
  ReliabilityLevel reliabilityLayer;
  bool weInitiatedTheConnection;
  unsigned char pingAndClockDifferential[8][5];
  int pingAndClockDifferentialWriteIndex;
  unsigned __int16 lowestPing;
  unsigned int nextPingTime;
  unsigned int lastReliableSend;
  BitStream staticData;
  unsigned int connectionTime;
  unsigned char AESKey[16];
  bool setAESKey;
  void* rpcMap[256];

  enum ConnectMode {
    NO_ACTION = 0x0,
    DISCONNECT_ASAP = 0x1,
    DISCONNECT_ASAP_SILENTLY = 0x2,
    DISCONNECT_ON_NO_ACK = 0x3,
    REQUESTED_CONNECTION = 0x4,
    HANDLING_CONNECTION_REQUEST = 0x5,
    UNVERIFIED_SENDER = 0x6,
    SET_ENCRYPTION_ON_MULTIPLE_16_BYTE_PACKET = 0x7,
    CONNECTED = 0x8,
  } connectMode;

  unsigned char auth_str_id;
  unsigned char field17_0xc67;
  unsigned char field18_0xc68;
};

static_assert(sizeof(RemoteSystemStruct) == 0xC69);

struct RPCNode {
  bool unk;
  void (__cdecl *staticFunctionPointer)(RPCParameters*);
};

struct RakPeer {
  void* vtbl;
  volatile bool endThreads;
  volatile bool isMainLoopThreadActive;
  bool occasionalPing;
  unsigned short maximumNumberOfPeers;
  unsigned short maximumIncomingConnections;
  BitStream localStaticData, offlinePingResponse;
  PlayerID myPlayerId;
  char incomingPassword[256];
  unsigned char incomingPasswordLength;
  RemoteSystemStruct* remote_system_list;
  char pad[0x4C9];
  RPCNode* map[256];
};

struct NetworkID {
  bool peerToPeer;
  PlayerID playerId;
  unsigned short localSystemId;
};
#pragma pack(pop)

class RakClientInterface {
public:
  virtual ~RakClientInterface() {
  };
  virtual bool Connect(const char* host, unsigned short serverPort, unsigned short clientPort,
                       unsigned int depreciated, int threadSleepTimer);
  virtual void Disconnect(unsigned int blockDuration, unsigned char orderingChannel = 0);
  virtual void InitializeSecurity(const char* privKeyP, const char* privKeyQ);
  virtual void SetPassword(const char* _password);
  virtual bool HasPassword(void) const;
  virtual bool Send(const char* data, const int length, PacketPriority priority, PacketReliability reliability,
                    char orderingChannel);
  virtual bool Send(BitStream* bitStream, PacketPriority priority, PacketReliability reliability,
                    char orderingChannel);
  virtual Packet* Receive(void);
  virtual void DeallocatePacket(Packet* packet);
  virtual void PingServer(void);
  virtual void PingServer(const char* host, unsigned short serverPort, unsigned short clientPort,
                          bool onlyReplyOnAcceptingConnections);
  virtual int GetAveragePing(void);
  virtual int GetLastPing(void) const;
  virtual int GetLowestPing(void) const;
  virtual int GetPlayerPing(const PlayerID playerId);
  virtual void StartOccasionalPing(void);
  virtual void StopOccasionalPing(void);
  virtual bool IsConnected(void) const;
  virtual unsigned int GetSynchronizedRandomInteger(void) const;
  virtual bool GenerateCompressionLayer(unsigned int inputFrequencyTable[256], bool inputLayer);
  virtual bool DeleteCompressionLayer(bool inputLayer);
  virtual void RegisterAsRemoteProcedureCall(int* uniqueID, void (*functionPointer)(RPCParameters* rpcParms));
  virtual void RegisterClassMemberRPC(int* uniqueID, void* functionPointer);
  virtual void UnregisterAsRemoteProcedureCall(int* uniqueID);
  virtual bool RPC(int* uniqueID, const char* data, unsigned int bitLength, PacketPriority priority,
                   PacketReliability reliability, char orderingChannel, bool shiftTimestamp);
  virtual bool RPC(int* uniqueID, BitStream* bitStream, PacketPriority priority, PacketReliability reliability,
                   char orderingChannel, bool shiftTimestamp);
  virtual bool RPC_(int* uniqueID, BitStream* bitStream, PacketPriority priority, PacketReliability reliability,
                    char orderingChannel, bool shiftTimestamp, NetworkID networkID);
  virtual void SetTrackFrequencyTable(bool b);
  virtual bool GetSendFrequencyTable(unsigned int outputFrequencyTable[256]);
  virtual float GetCompressionRatio(void) const;
  virtual float GetDecompressionRatio(void) const;
  virtual void AttachPlugin(void* messageHandler);
  virtual void DetachPlugin(void* messageHandler);
  virtual BitStream* GetStaticServerData(void);
  virtual void SetStaticServerData(const char* data, const int length);
  virtual BitStream* GetStaticClientData(const PlayerID playerId);
  virtual void SetStaticClientData(const PlayerID playerId, const char* data, const int length);
  virtual void SendStaticClientDataToServer(void);
  virtual PlayerID GetServerID(void) const;
  virtual PlayerID GetPlayerID(void) const;
  virtual PlayerID GetInternalID(void) const;
  virtual const char* PlayerIDToDottedIP(const PlayerID playerId) const;
  virtual void PushBackPacket(Packet* packet, bool pushAtHead);
  virtual void SetRouterInterface(void* routerInterface);
  virtual void RemoveRouterInterface(void* routerInterface);
  virtual void SetTimeoutTime(RakNetTime timeMS);
  virtual bool SetMTUSize(int size);
  virtual int GetMTUSize(void) const;
  virtual void AllowConnectionResponseIPMigration(bool allow);
  virtual void AdvertiseSystem(const char* host, unsigned short remotePort, const char* data, int dataLength);
  virtual RakNetStatisticsStruct* const GetStatistics(void);
  virtual void ApplyNetworkSimulator(double maxSendBPS, unsigned short minExtraPing,
                                     unsigned short extraPingVariance);
  virtual bool IsNetworkSimulatorActive(void);
  virtual PlayerIndex GetPlayerIndex(void);
};

enum class RPCEnumeration {
  RPC_ServerJoin = 137,
  RPC_ServerQuit = 138,
  RPC_InitGame = 139,
  RPC_ClientJoin = 25,
  RPC_NPCJoin = 54,
  RPC_Death = 53,
  RPC_RequestClass = 128,
  RPC_RequestSpawn = 129,
  RPC_SetInteriorId = 118,
  RPC_Spawn = 52,
  RPC_Chat = 101,
  RPC_EnterVehicle = 26,
  RPC_ExitVehicle = 154,
  RPC_DamageVehicle = 106,
  RPC_MenuSelect = 132,
  RPC_MenuQuit = 140,
  RPC_ScmEvent = 96,
  RPC_AdminMapTeleport = 255,
  RPC_WorldPlayerAdd = 32,
  RPC_WorldPlayerDeath = 166,
  RPC_WorldPlayerRemove = 163,
  RPC_WorldVehicleAdd = 164,
  RPC_WorldVehicleRemove = 165,
  RPC_SetCheckpoint = 107,
  RPC_DisableCheckpoint = 37,
  RPC_SetRaceCheckpoint = 38,
  RPC_DisableRaceCheckpoint = 39,
  RPC_UpdateScoresPingsIPs = 155,
  RPC_SvrStats = 102,
  RPC_GameModeRestart = 40,
  RPC_ConnectionRejected = 130,
  RPC_ClientMessage = 93,
  RPC_WorldTime = 94,
  RPC_Pickup = 95,
  RPC_DestroyPickup = 63,
  RPC_DestroyWeaponPickup = 97,
  RPC_Weather = 152,
  RPC_SetTimeEx = 255,
  RPC_ToggleClock = 30,
  RPC_ServerCommand = 50,
  RPC_PickedUpPickup = 131,
  RPC_PickedUpWeapon = 255,
  RPC_VehicleDestroyed = 136,
  RPC_DialogResponse = 62,
  RPC_PlayAudioStream = 41,
  RPC_StopAudioStream = 42,
  RPC_ClickPlayer = 23,
  RPC_PlayerUpdate = 60,
  RPC_ClickTextDraw = 83,
  RPC_MapMarker = 119,
  RPC_PlayerGiveTakeDamage = 115,
  RPC_EnterEditObject = 27,
  RPC_EditObject = 117,

  RPC_ScrRemoveBuildingForPlayer = 43,
  RPC_ScrSetObjectMaterial = 84,
  RPC_ScrSetPlayerAttachedObject = 113,
  RPC_ScrSetPlayerSkillLevel = 34,
  RPC_ScrSetSpawnInfo = 68,
  RPC_ScrSetPlayerTeam = 69,
  RPC_ScrSetPlayerSkin = 153,
  RPC_ScrSetPlayerName = 11,
  RPC_ScrSetPlayerPos = 12,
  RPC_ScrSetPlayerPosFindZ = 13,
  RPC_ScrSetPlayerHealth = 14,
  RPC_ScrPutPlayerInVehicle = 70,
  RPC_ScrRemovePlayerFromVehicle = 71,
  RPC_ScrSetPlayerColor = 72,
  RPC_ScrDisplayGameText = 73,
  RPC_ScrSetInterior = 156,
  RPC_ScrSetCameraPos = 157,
  RPC_ScrSetCameraLookAt = 158,
  RPC_ScrSetVehiclePos = 159,
  RPC_ScrSetVehicleZAngle = 160,
  RPC_ScrVehicleParams = 161,
  RPC_ScrSetCameraBehindPlayer = 162,
  RPC_ScrTogglePlayerControllable = 15,
  RPC_ScrPlaySound = 16,
  RPC_ScrSetWorldBounds = 17,
  RPC_ScrHaveSomeMoney = 18,
  RPC_ScrSetPlayerFacingAngle = 19,
  RPC_ScrResetMoney = 20,
  RPC_ScrResetPlayerWeapons = 21,
  RPC_ScrGivePlayerWeapon = 22,
  RPC_ScrRespawnVehicle = 255,
  RPC_ScrLinkVehicle = 65,
  RPC_ScrSetPlayerArmour = 66,
  RPC_ScrDeathMessage = 55,
  RPC_ScrSetMapIcon = 56,
  RPC_ScrDisableMapIcon = 144,
  RPC_ScrSetWeaponAmmo = 145,
  RPC_ScrSetGravity = 146,
  RPC_ScrSetVehicleHealth = 147,
  RPC_ScrAttachTrailerToVehicle = 148,
  RPC_ScrDetachTrailerFromVehicle = 149,
  RPC_ScrCreateObject = 44,
  RPC_ScrSetObjectPos = 45,
  RPC_ScrSetObjectRotation = 46,
  RPC_ScrDestroyObject = 47,
  RPC_ScrCreateExplosion = 79,
  RPC_ScrShowNameTag = 80,
  RPC_ScrMoveObject = 99,
  RPC_ScrStopObject = 122,
  RPC_ScrNumberPlate = 123,
  RPC_ScrTogglePlayerSpectating = 124,
  RPC_ScrSetPlayerSpectating = 255,
  RPC_ScrPlayerSpectatePlayer = 126,
  RPC_ScrPlayerSpectateVehicle = 127,
  RPC_ScrRemoveComponent = 57,
  RPC_ScrForceSpawnSelection = 74,
  RPC_ScrAttachObjectToPlayer = 75,
  RPC_ScrInitMenu = 76,
  RPC_ScrShowMenu = 77,
  RPC_ScrHideMenu = 78,
  RPC_ScrSetPlayerWantedLevel = 133,
  RPC_ScrShowTextDraw = 134,
  RPC_ScrHideTextDraw = 135,
  RPC_ScrEditTextDraw = 105,
  RPC_ScrAddGangZone = 108,
  RPC_ScrRemoveGangZone = 120,
  RPC_ScrFlashGangZone = 121,
  RPC_ScrStopFlashGangZone = 85,
  RPC_ScrApplyAnimation = 86,
  RPC_ScrClearAnimations = 87,
  RPC_ScrSetSpecialAction = 88,
  RPC_ScrEnableStuntBonus = 104,
  RPC_ScrSetFightingStyle = 89,
  RPC_ScrSetPlayerVelocity = 90,
  RPC_ScrSetVehicleVelocity = 91,
  RPC_ScrToggleWidescreen = 255,
  RPC_ScrSetVehicleTireStatus = 255,
  RPC_ScrSetPlayerDrunkLevel = 35,
  RPC_ScrDialogBox = 61,
  RPC_ScrCreate3DTextLabel = 36,
};

enum PacketEnumeration : unsigned char {
  ID_INTERNAL_PING = 6,
  ID_PING,
  ID_PING_OPEN_CONNECTIONS,
  ID_CONNECTED_PONG,
  ID_REQUEST_STATIC_DATA,
  ID_CONNECTION_REQUEST,
  ID_AUTH_KEY,
  ID_BROADCAST_PINGS = 14,
  ID_SECURED_CONNECTION_RESPONSE,
  ID_SECURED_CONNECTION_CONFIRMATION,
  ID_RPC_MAPPING,
  ID_SET_RANDOM_NUMBER_SEED = 19,
  ID_RPC,
  ID_RPC_REPLY,
  ID_DETECT_LOST_CONNECTIONS = 23,
  ID_OPEN_CONNECTION_REQUEST,
  ID_OPEN_CONNECTION_REPLY,
  ID_OPEN_CONNECTION_COOKIE,
  ID_RSA_PUBLIC_KEY_MISMATCH = 28,
  ID_CONNECTION_ATTEMPT_FAILED,
  ID_NEW_INCOMING_CONNECTION = 30,
  ID_NO_FREE_INCOMING_CONNECTIONS = 31,
  ID_DISCONNECTION_NOTIFICATION,
  ID_CONNECTION_LOST,
  ID_CONNECTION_REQUEST_ACCEPTED,
  ID_CONNECTION_BANNED = 36,
  ID_INVALID_PASSWORD,
  ID_MODIFIED_PACKET,
  ID_PONG,
  ID_TIMESTAMP,
  ID_RECEIVED_STATIC_DATA,
  ID_REMOTE_DISCONNECTION_NOTIFICATION,
  ID_REMOTE_CONNECTION_LOST,
  ID_REMOTE_NEW_INCOMING_CONNECTION,
  ID_REMOTE_EXISTING_CONNECTION,
  ID_REMOTE_STATIC_DATA,
  ID_ADVERTISE_SYSTEM = 55,
  ID_PLAYER_SYNC = 207,
  ID_MARKERS_SYNC = 208,
  ID_UNOCCUPIED_SYNC = 209,
  ID_TRAILER_SYNC = 210,
  ID_PASSENGER_SYNC = 211,
  ID_SPECTATOR_SYNC = 212,
  ID_AIM_SYNC = 203,
  ID_VEHICLE_SYNC = 200,
  ID_RCON_COMMAND = 201,
  ID_RCON_RESPONCE = 202,
  ID_WEAPONS_UPDATE = 204,
  ID_STATS_UPDATE = 205,
  ID_BULLET_SYNC = 206,
};
