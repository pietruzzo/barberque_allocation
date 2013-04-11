/*
 * Copyright (C) 2012  Politecnico di Milano
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BBQUE_RPC_UNMANAGED_CLIENT_H_
#define BBQUE_RPC_UNMANAGED_CLIENT_H_

#include "bbque/rtlib.h"

namespace bbque { namespace rtlib {

/**
 * @brief Client side of the RPC Umnanaged channel
 *
 * An Unmanaged client is a BBQ client which is completely detached by the
 * BarbequeRTRM. This allows, for example, to run Bbque integrated
 * applications in case the BBQ daemon is not runinng. This is a convenient
 * execution mode especially useful to run testing and simulations.
 *
 * NOTE: Unmanaged clients always get assigned AWM 0.
 *
 */
class BbqueRPC_UNMANAGED_Client : public BbqueRPC {

public:

	BbqueRPC_UNMANAGED_Client() {};

	~BbqueRPC_UNMANAGED_Client() {};

protected:

	RTLIB_ExitCode_t _Init(const char *name) {
		// Remove compilation warning
		(void)name;
		return RTLIB_OK;
	}

	RTLIB_ExitCode_t _Register(pregExCtx_t prec) {
		// Remove compilation warning
		(void)prec;
		return RTLIB_OK;
	}

	RTLIB_ExitCode_t _Unregister(pregExCtx_t prec) {
		// Remove compilation warning
		(void)prec;
		return RTLIB_OK;
	}

	RTLIB_ExitCode_t _Enable(pregExCtx_t prec) {
		// Remove compilation warning
		(void)prec;
		return RTLIB_OK;
	}

	RTLIB_ExitCode_t _Disable(pregExCtx_t prec) {
		// Remove compilation warning
		(void)prec;
		return RTLIB_OK;
	}

	RTLIB_ExitCode_t _ScheduleRequest(pregExCtx_t prec) {
		// Remove compilation warning
		(void)prec;
		return RTLIB_OK;
	}

	RTLIB_ExitCode_t _Set(pregExCtx_t prec,
			RTLIB_Constraint* constraints, uint8_t count) {
		// Remove compilation warning
		(void)prec;
		(void)count;
		(void)constraints;
		return RTLIB_OK;
	}

	RTLIB_ExitCode_t _Clear(pregExCtx_t prec) {
		// Remove compilation warning
		(void)prec;
		return RTLIB_OK;
	}

	RTLIB_ExitCode_t _GGap(pregExCtx_t prec, uint8_t gap) {
		// Remove compilation warning
		(void)prec;
		(void)gap;
		return RTLIB_OK;
	}

	void _Exit() {}

/******************************************************************************
 * Synchronization Protocol Messages
 ******************************************************************************/

	RTLIB_ExitCode_t _SyncpPreChangeResp(
			rpc_msg_token_t token,
			pregExCtx_t prec,
			uint32_t syncLatency) {
		// Remove compilation warning
		(void)prec;
		(void)token;
		(void)syncLatency;
		return RTLIB_OK;
	}

	RTLIB_ExitCode_t _SyncpSyncChangeResp(
			rpc_msg_token_t token,
			pregExCtx_t prec,
			RTLIB_ExitCode_t sync) {
		// Remove compilation warning
		(void)prec;
		(void)token;
		(void)sync;
		return RTLIB_OK;
	}

	RTLIB_ExitCode_t _SyncpPostChangeResp(
			rpc_msg_token_t token,
			pregExCtx_t prec,
			RTLIB_ExitCode_t result) {
		// Remove compilation warning
		(void)prec;
		(void)token;
		(void)result;
		return RTLIB_OK;
	}

};

} // namespace rtlib

} // namespace bbque

#endif // BBQUE_RPC_UNMANAGED_CLIENT_H_