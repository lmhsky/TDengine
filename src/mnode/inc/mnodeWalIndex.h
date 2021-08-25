/*
 * Copyright (c) 2019 TAOS Data, Inc. <cli@taosdata.com>
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

#ifndef TDENGINE_MNODE_WAL_INDEX_H
#define TDENGINE_MNODE_WAL_INDEX_H

#pragma  pack(1)
typedef struct walIndex {
  int64_t offset;
  uint16_t size;
  uint8_t keyLen;
  char key[];
} walIndex;

struct walIndexFileInfo;

typedef struct walIndexItem {
  struct walIndexItem* prev;
  struct walIndexItem* next;

  struct walIndexFileInfo *pFileInfo;

  union {
    char    dbName[TSDB_ACCT_ID_LEN + TSDB_DB_NAME_LEN];  // SVgObj -> SDbObj
    char    tableId[TSDB_ACCT_ID_LEN + TSDB_DB_NAME_LEN]; // SSTableObj -> SDbObj
    uint64_t suid;                                        // SCTableObj -> SSTableObj
  } parentIndexKey;

  walIndex index;
} walIndexItem;

typedef int32_t FWalIndexReader(int64_t tfd, const char* name, int32_t tableId, uint64_t version, walIndex*);

int32_t sdbRestoreFromIndex(twalh, FWalIndexReader fpReader, FWalWrite writeFp);

void mnodeSdbBuildWalIndex(void*);

#endif // TDENGINE_MNODE_WAL_INDEX_H