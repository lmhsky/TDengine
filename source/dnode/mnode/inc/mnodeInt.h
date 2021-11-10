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

#ifndef _TD_MNODE_INT_H_
#define _TD_MNODE_INT_H_

#include "mnodeDef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { MN_STATUS_UNINIT = 0, MN_STATUS_INIT = 1, MN_STATUS_READY = 2, MN_STATUS_CLOSING = 3 } EMnStatus;

tmr_h     mnodeGetTimer();
int32_t   mnodeGetDnodeId();
int64_t   mnodeGetClusterId();
EMnStatus mnodeGetStatus();

void mnodeSendMsgToDnode(struct SEpSet *epSet, struct SRpcMsg *rpcMsg);
void mnodeSendMsgToMnode(struct SRpcMsg *rpcMsg);
void mnodeSendRedirectMsg(struct SRpcMsg *rpcMsg, bool forShell);

#ifdef __cplusplus
}
#endif

#endif /*_TD_MNODE_INT_H_*/