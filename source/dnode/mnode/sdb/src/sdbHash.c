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

#define _DEFAULT_SOURCE
#include "sdbInt.h"
#include "tglobal.h"

static SHashObj *sdbGetHash(int32_t sdb) {
  if (sdb >= SDB_MAX || sdb <= SDB_START) {
    terrno = TSDB_CODE_SDB_INVALID_TABLE_TYPE;
    return NULL;
  }

  SHashObj *hash = tsSdb.hashObjs[sdb];
  if (hash == NULL) {
    terrno = TSDB_CODE_SDB_APP_ERROR;
    return NULL;
  }

  return hash;
}

static int32_t sdbGetkeySize(ESdbType sdb, void *pKey) {
  int32_t  keySize;
  EKeyType keyType = tsSdb.keyTypes[sdb];

  if (keyType == SDB_KEY_INT32) {
    keySize = sizeof(int32_t);
  } else if (keyType == SDB_KEY_BINARY) {
    keySize = strlen(pKey) + 1;
  } else {
    keySize = sizeof(int64_t);
  }

  return keySize;
}

static int32_t sdbInsertRow(SHashObj *hash, SSdbRaw *pRaw, SSdbRow *pRow, int32_t keySize) {
  SRWLatch *pLock = &tsSdb.locks[pRow->sdb];
  taosWLockLatch(pLock);

  SSdbRow *pDstRow = taosHashGet(hash, pRow->pObj, keySize);
  if (pDstRow != NULL) {
    terrno = TSDB_CODE_SDB_OBJ_ALREADY_THERE;
    taosWUnLockLatch(pLock);
    return -1;
  }

  pRow->refCount = 1;
  pRow->status = pRaw->status;

  if (taosHashPut(hash, pRow->pObj, keySize, &pRow, sizeof(void *)) != 0) {
    terrno = TSDB_CODE_OUT_OF_MEMORY;
    taosWUnLockLatch(pLock);
    return -1;
  }

  taosWUnLockLatch(pLock);

  SdbInsertFp insertFp = tsSdb.insertFps[pRow->sdb];
  if (insertFp != NULL) {
    if ((*insertFp)(pRow->pObj) != 0) {
      taosWLockLatch(pLock);
      taosHashRemove(hash, pRow->pObj, keySize);
      taosWUnLockLatch(pLock);
      return -1;
    }
  }

  return 0;
}

static int32_t sdbUpdateRow(SHashObj *hash, SSdbRaw *pRaw, SSdbRow *pRow, int32_t keySize) {
  SRWLatch *pLock = &tsSdb.locks[pRow->sdb];
  taosRLockLatch(pLock);

  SSdbRow *pDstRow = taosHashGet(hash, pRow->pObj, keySize);
  if (pDstRow == NULL) {
    terrno = TSDB_CODE_SDB_OBJ_NOT_THERE;
    taosRUnLockLatch(pLock);
    return -1;
  }

  pRow->status = pRaw->status;
  taosRUnLockLatch(pLock);

  SdbUpdateFp updateFp = tsSdb.updateFps[pRow->sdb];
  if (updateFp != NULL) {
    if ((*updateFp)(pRow->pObj, pDstRow->pObj) != 0) {
      return -1;
    }
  }

  return 0;
}

static int32_t sdbDeleteRow(SHashObj *hash, SSdbRaw *pRaw, SSdbRow *pRow, int32_t keySize) {
  SRWLatch *pLock = &tsSdb.locks[pRow->sdb];
  taosWLockLatch(pLock);

  SSdbRow *pDstRow = taosHashGet(hash, pRow->pObj, keySize);
  if (pDstRow == NULL) {
    terrno = TSDB_CODE_SDB_OBJ_NOT_THERE;
    taosWUnLockLatch(pLock);
    return -1;
  }

  pRow->status = pRaw->status;
  taosHashRemove(hash, pRow->pObj, keySize);
  taosWUnLockLatch(pLock);

  SdbDeleteFp deleteFp = tsSdb.deleteFps[pRow->sdb];
  if (deleteFp != NULL) {
    if ((*deleteFp)(pRow->pObj) != 0) {
      return -1;
    }
  }

  sdbRelease(pRow->pObj);
  return 0;
}

int32_t sdbWrite(SSdbRaw *pRaw) {
  SHashObj *hash = sdbGetHash(pRaw->sdb);
  if (hash == NULL) return -1;

  SdbDecodeFp decodeFp = tsSdb.decodeFps[pRaw->sdb];
  SSdbRow    *pRow = (*decodeFp)(pRaw);
  if (pRow == NULL) {
    terrno = TSDB_CODE_SDB_INVALID_DATA_CONTENT;
    return -1;
  }

  pRow->sdb = pRaw->sdb;

  int32_t keySize = sdbGetkeySize(pRow->sdb, pRow->pObj);
  int32_t code = -1;

  switch (pRaw->status) {
    case SDB_STATUS_CREATING:
      code = sdbInsertRow(hash, pRaw, pRow, keySize);
      break;
    case SDB_STATUS_READY:
    case SDB_STATUS_DROPPING:
      code = sdbUpdateRow(hash, pRaw, pRow, keySize);
      break;
    case SDB_STATUS_DROPPED:
      code = sdbDeleteRow(hash, pRaw, pRow, keySize);
      break;
    default:
      terrno = TSDB_CODE_SDB_INVALID_ACTION_TYPE;
      break;
  }

  if (code != 0) {
    sdbFreeRow(pRow);
  }

  return 0;
}

void *sdbAcquire(ESdbType sdb, void *pKey) {
  SHashObj *hash = sdbGetHash(sdb);
  if (hash == NULL) return NULL;

  void   *pRet = NULL;
  int32_t keySize = sdbGetkeySize(sdb, pKey);

  SRWLatch *pLock = &tsSdb.locks[sdb];
  taosRLockLatch(pLock);

  SSdbRow **ppRow = taosHashGet(hash, pKey, keySize);
  if (ppRow == NULL || *ppRow) {
    terrno = TSDB_CODE_SDB_OBJ_NOT_THERE;
    taosRUnLockLatch(pLock);
    return NULL;
  }

  SSdbRow *pRow = *ppRow;
  switch (pRow->status) {
    case SDB_STATUS_READY:
      atomic_add_fetch_32(&pRow->refCount, 1);
      pRet = pRow->pObj;
      break;
    case SDB_STATUS_CREATING:
      terrno = TSDB_CODE_SDB_OBJ_CREATING;
      break;
    case SDB_STATUS_DROPPING:
      terrno = TSDB_CODE_SDB_OBJ_DROPPING;
      break;
    default:
      terrno = TSDB_CODE_SDB_APP_ERROR;
      break;
  }

  taosRUnLockLatch(pLock);
  return pRet;
}

void sdbRelease(void *pObj) {
  if (pObj == NULL) return;

  SSdbRow *pRow = (SSdbRow *)((char *)pObj - sizeof(SSdbRow));
  if (pRow->sdb >= SDB_MAX || pRow->sdb <= SDB_START) return;

  SRWLatch *pLock = &tsSdb.locks[pRow->sdb];
  taosRLockLatch(pLock);

  int32_t ref = atomic_sub_fetch_32(&pRow->refCount, 1);
  if (ref <= 0 && pRow->status == SDB_STATUS_DROPPED) {
    sdbFreeRow(pRow);
  }

  taosRUnLockLatch(pLock);
}

void *sdbFetchRow(ESdbType sdb, void *pIter) {
  SHashObj *hash = sdbGetHash(sdb);
  if (hash == NULL) {
    return NULL;
  }

  SRWLatch *pLock = &tsSdb.locks[sdb];
  taosRLockLatch(pLock);
  void *pRet = taosHashIterate(hash, pIter);
  taosRUnLockLatch(pLock);

  return pRet;
}

void sdbCancelFetch(ESdbType sdb, void *pIter) {
  SHashObj *hash = sdbGetHash(sdb);
  if (hash == NULL) {
    return;
  }

  SRWLatch *pLock = &tsSdb.locks[sdb];
  taosRLockLatch(pLock);
  taosHashCancelIterate(hash, pIter);
  taosRUnLockLatch(pLock);
}

int32_t sdbGetSize(ESdbType sdb) {
  SHashObj *hash = sdbGetHash(sdb);
  if (hash == NULL) {
    return 0;
  }

  SRWLatch *pLock = &tsSdb.locks[sdb];
  taosRLockLatch(pLock);
  int32_t size = taosHashGetSize(hash);
  taosRUnLockLatch(pLock);

  return size;
}