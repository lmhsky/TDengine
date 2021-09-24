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

#ifndef _TD_TSDB_H_
#define _TD_TSDB_H_

#include "os.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct STsdb STsdb;
typedef struct {
} STsdbCfg;

int    tsdbInit(int nthreads);
int    tsdbClear();
int    tsdbCreateRepo(int id);
int    tsdbDropRepo(int id);
STsdb *tsdbOpenRepo(STsdbCfg *pCfg);
int    tsdbCloseRepo(STsdb *pTsdb);
int    tsdbForceCloseRepo(STsdb *pTsdb);

#ifdef __cplusplus
}
#endif

#endif /*_TD_TSDB_H_*/