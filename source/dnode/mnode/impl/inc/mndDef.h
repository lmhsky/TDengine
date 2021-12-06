/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _TD_MND_DEF_H_
#define _TD_MND_DEF_H_

#include "os.h"
#include "taosmsg.h"
#include "tlog.h"
#include "trpc.h"
#include "ttimer.h"
#include "thash.h"
#include "cJSON.h"
#include "mnode.h"
#include "sync.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int32_t mDebugFlag;

// mnode log function
#define mFatal(...) { if (mDebugFlag & DEBUG_FATAL) { taosPrintLog("MND FATAL ", 255, __VA_ARGS__); }}
#define mError(...) { if (mDebugFlag & DEBUG_ERROR) { taosPrintLog("MND ERROR ", 255, __VA_ARGS__); }}
#define mWarn(...)  { if (mDebugFlag & DEBUG_WARN)  { taosPrintLog("MND WARN ", 255, __VA_ARGS__); }}
#define mInfo(...)  { if (mDebugFlag & DEBUG_INFO)  { taosPrintLog("MND ", 255, __VA_ARGS__); }}
#define mDebug(...) { if (mDebugFlag & DEBUG_DEBUG) { taosPrintLog("MND ", mDebugFlag, __VA_ARGS__); }}
#define mTrace(...) { if (mDebugFlag & DEBUG_TRACE) { taosPrintLog("MND ", mDebugFlag, __VA_ARGS__); }}

typedef struct SClusterObj SClusterObj;
typedef struct SDnodeObj   SDnodeObj;
typedef struct SMnodeObj   SMnodeObj;
typedef struct SAcctObj    SAcctObj;
typedef struct SUserObj    SUserObj;
typedef struct SDbObj      SDbObj;
typedef struct SVgObj      SVgObj;
typedef struct SSTableObj  SSTableObj;
typedef struct SFuncObj    SFuncObj;
typedef struct SOperObj    SOperObj;

typedef enum {
  MND_AUTH_ACCT_START = 0,
  MND_AUTH_ACCT_USER,
  MND_AUTH_ACCT_DNODE,
  MND_AUTH_ACCT_MNODE,
  MND_AUTH_ACCT_DB,
  MND_AUTH_ACCT_TABLE,
  MND_AUTH_ACCT_MAX
} EAuthAcct;

typedef enum {
  MND_AUTH_OP_START = 0,
  MND_AUTH_OP_CREATE_USER,
  MND_AUTH_OP_ALTER_USER,
  MND_AUTH_OP_DROP_USER,
  MND_AUTH_MAX
} EAuthOp;

typedef enum {
  TRN_STAGE_PREPARE = 1,
  TRN_STAGE_EXECUTE = 2,
  TRN_STAGE_COMMIT = 3,
  TRN_STAGE_ROLLBACK = 4,
  TRN_STAGE_RETRY = 5
} ETrnStage;

typedef enum { TRN_POLICY_ROLLBACK = 1, TRN_POLICY_RETRY = 2 } ETrnPolicy;

typedef enum {
  DND_STATUS_OFFLINE = 0,
  DND_STATUS_READY = 1,
  DND_STATUS_CREATING = 2,
  DND_STATUS_DROPPING = 3
} EDndStatus;

typedef enum {
  DND_REASON_ONLINE = 0,
  DND_REASON_STATUS_MSG_TIMEOUT,
  DND_REASON_STATUS_NOT_RECEIVED,
  DND_REASON_VERSION_NOT_MATCH,
  DND_REASON_DNODE_ID_NOT_MATCH,
  DND_REASON_CLUSTER_ID_NOT_MATCH,
  DND_REASON_MN_EQUAL_VN_NOT_MATCH,
  DND_REASON_STATUS_INTERVAL_NOT_MATCH,
  DND_REASON_TIME_ZONE_NOT_MATCH,
  DND_REASON_LOCALE_NOT_MATCH,
  DND_REASON_CHARSET_NOT_MATCH,
  DND_REASON_OTHERS
} EDndReason;

typedef struct STrans {
  int32_t    id;
  ETrnStage  stage;
  ETrnPolicy policy;
  SMnode    *pMnode;
  void      *rpcHandle;
  SArray    *redoLogs;
  SArray    *undoLogs;
  SArray    *commitLogs;
  SArray    *redoActions;
  SArray    *undoActions;
} STrans;

typedef struct SClusterObj {
  int32_t id;
  char    uid[TSDB_CLUSTER_ID_LEN];
  int64_t createdTime;
  int64_t updateTime;
} SClusterObj;

typedef struct SDnodeObj {
  int32_t    id;
  int64_t    createdTime;
  int64_t    updateTime;
  int64_t    rebootTime;
  int32_t    accessTimes;
  int16_t    numOfMnodes;
  int16_t    numOfVnodes;
  int16_t    numOfQnodes;
  int16_t    numOfSupportMnodes;
  int16_t    numOfSupportVnodes;
  int16_t    numOfSupportQnodes;
  int16_t    numOfCores;
  EDndStatus status;
  EDndReason offlineReason;
  uint16_t   port;
  char       fqdn[TSDB_FQDN_LEN];
  char       ep[TSDB_EP_LEN];
} SDnodeObj;

typedef struct SMnodeObj {
  int32_t    id;
  int64_t    createdTime;
  int64_t    updateTime;
  ESyncState role;
  int32_t    roleTerm;
  int64_t    roleTime;
  SDnodeObj *pDnode;
} SMnodeObj;

typedef struct {
  int32_t maxUsers;
  int32_t maxDbs;
  int32_t maxTimeSeries;
  int32_t maxStreams;
  int64_t maxStorage;   // In unit of GB
  int32_t accessState;  // Configured only by command
} SAcctCfg;

typedef struct {
  int32_t numOfUsers;
  int32_t numOfDbs;
  int32_t numOfTimeSeries;
  int32_t numOfStreams;
  int64_t totalStorage;  // Total storage wrtten from this account
  int64_t compStorage;   // Compressed storage on disk
} SAcctInfo;

typedef struct SAcctObj {
  char      acct[TSDB_USER_LEN];
  int64_t   createdTime;
  int64_t   updateTime;
  int32_t   acctId;
  int32_t   status;
  SAcctCfg  cfg;
  SAcctInfo info;
} SAcctObj;

typedef struct SUserObj {
  char      user[TSDB_USER_LEN];
  char      pass[TSDB_KEY_LEN];
  char      acct[TSDB_USER_LEN];
  int64_t   createdTime;
  int64_t   updateTime;
  int8_t    superAuth;
  int8_t    readAuth;
  int8_t    writeAuth;
  int32_t   acctId;
  SHashObj *prohibitDbHash;
} SUserObj;

typedef struct {
  int32_t cacheBlockSize;
  int32_t totalBlocks;
  int32_t maxTables;
  int32_t daysPerFile;
  int32_t daysToKeep0;
  int32_t daysToKeep1;
  int32_t daysToKeep2;
  int32_t minRowsPerFileBlock;
  int32_t maxRowsPerFileBlock;
  int32_t commitTime;
  int32_t fsyncPeriod;
  int8_t  precision;
  int8_t  compression;
  int8_t  walLevel;
  int8_t  replications;
  int8_t  quorum;
  int8_t  update;
  int8_t  cacheLastRow;
  int8_t  dbType;
  int16_t partitions;
} SDbCfg;

typedef struct SDbObj {
  char      name[TSDB_FULL_DB_NAME_LEN];
  char      acct[TSDB_USER_LEN];
  int64_t   createdTime;
  int64_t   updateTime;
  SDbCfg    cfg;
  int64_t   uid;
  int8_t    status;
  int32_t   numOfVgroups;
  int32_t   numOfTables;
  int32_t   numOfSuperTables;
  int32_t   vgListSize;
  int32_t   vgListIndex;
  SVgObj  **vgList;
  SAcctObj *pAcct;
} SDbObj;

typedef struct {
  int32_t    dnodeId;
  int8_t     role;
  SDnodeObj *pDnode;
} SVnodeGid;

typedef struct SVgObj {
  uint32_t  vgId;
  int32_t   numOfVnodes;
  int64_t   createdTime;
  int64_t   updateTime;
  int32_t   lbDnodeId;
  int32_t   lbTime;
  char      dbName[TSDB_FULL_DB_NAME_LEN];
  int8_t    inUse;
  int8_t    accessState;
  int8_t    status;
  SVnodeGid vnodeGid[TSDB_MAX_REPLICA];
  int32_t   vgCfgVersion;
  int8_t    compact;
  int32_t   numOfTables;
  int64_t   totalStorage;
  int64_t   compStorage;
  int64_t   pointsWritten;
  SDbObj   *pDb;
} SVgObj;

typedef struct SSTableObj {
  char     tableId[TSDB_TABLE_NAME_LEN];
  uint64_t uid;
  int64_t  createdTime;
  int64_t  updateTime;
  int32_t  numOfColumns;  // used by normal table
  int32_t  numOfTags;
  SSchema *schema;
} SSTableObj;

typedef struct SFuncObj {
  char    name[TSDB_FUNC_NAME_LEN];
  char    path[128];
  int32_t contLen;
  char    cont[TSDB_FUNC_CODE_LEN];
  int32_t funcType;
  int32_t bufSize;
  int64_t createdTime;
  uint8_t resType;
  int16_t resBytes;
  int64_t sig;
  int16_t type;
} SFuncObj;

typedef struct SShowObj SShowObj;
typedef struct SShowObj {
  int8_t     type;
  int8_t     maxReplica;
  int16_t    numOfColumns;
  int32_t    id;
  int32_t    rowSize;
  int32_t    numOfRows;
  int32_t    numOfReads;
  uint16_t   payloadLen;
  void      *pIter;
  void      *pVgIter;
  SMnode    *pMnode;
  SShowObj **ppShow;
  char       db[TSDB_FULL_DB_NAME_LEN];
  int16_t    offset[TSDB_MAX_COLUMNS];
  int32_t    bytes[TSDB_MAX_COLUMNS];
  char       payload[];
} SShowObj;

typedef struct SMnodeMsg {
  char    user[TSDB_USER_LEN];
  char    db[TSDB_FULL_DB_NAME_LEN];
  int32_t acctId;
  SMnode *pMnode;
  int16_t received;
  int16_t successed;
  int16_t expected;
  int16_t retry;
  int32_t code;
  int64_t createdTime;
  SRpcMsg rpcMsg;
  int32_t contLen;
  void   *pCont;
} SMnodeMsg;

#ifdef __cplusplus
}
#endif

#endif /*_TD_MND_DEF_H_*/