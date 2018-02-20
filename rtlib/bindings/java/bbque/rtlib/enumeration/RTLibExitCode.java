package bbque.rtlib.enumeration;

public enum RTLibExitCode {
    /** Success (no errors) */
    RTLIB_OK,
    /** Unspecified (generic) error */
    RTLIB_ERROR,
    /** RTLib Version does not match that of the Barbeque RTRM */
    RTLIB_VERSION_MISMATCH,
    /** No new working mode error */
    RTLIB_NO_WORKING_MODE,
    /** Failed to setup the channel to connect the Barbeque RTRM */
    RTLIB_BBQUE_CHANNEL_SETUP_FAILED,
    /** Failed to release the channel to connect the Barbeque RTRM */
    RTLIB_BBQUE_CHANNEL_TEARDOWN_FAILED,
    /** Failed to write to the Barbeque RTRM communication channel */
    RTLIB_BBQUE_CHANNEL_WRITE_FAILED,
    /** Failed to read form the Barbeque RTRM communication channel */
    RTLIB_BBQUE_CHANNEL_READ_FAILED,
    /** Timeout on read form the Barbeque RTRM communication channel */
    RTLIB_BBQUE_CHANNEL_READ_TIMEOUT,
    /** The bbque and application RPC protocol versions does not match */
    RTLIB_BBQUE_CHANNEL_PROTOCOL_MISMATCH,
    /** The (expected) communicaiton channel is not available */
    RTLIB_BBQUE_CHANNEL_UNAVAILABLE,
    /** The (expected) response has gone in TIMEOUT */
    RTLIB_BBQUE_CHANNEL_TIMEOUT,
    /** The Barbeque RTRM is not available */
    RTLIB_BBQUE_UNREACHABLE,
    /** The Execution Context Duplicated */
    RTLIB_EXC_DUPLICATE,
    /** The Execution Context has not been registered */
    RTLIB_EXC_NOT_REGISTERED,
    /** The Execution Context Registration Failed */
    RTLIB_EXC_REGISTRATION_FAILED,
    /** The Execution Context Registration Failed */
    RTLIB_EXC_MISSING_RECIPE,
    /** The Execution Context Unregistration Failed */
    RTLIB_EXC_UNREGISTRATION_FAILED,
    /** The Execution Context has not been started yet */
    RTLIB_EXC_NOT_STARTED,
    /** The Execution Context Start Failed */
    RTLIB_EXC_ENABLE_FAILED,
    /** The Execution Context is not currently enabled */
    RTLIB_EXC_NOT_ENABLED,
    /** The Execution Context Stop Failed */
    RTLIB_EXC_DISABLE_FAILED,
    /** Failed to get a working mode */
    RTLIB_EXC_GWM_FAILED,
    /** Start running on the assigned AWM */
    RTLIB_EXC_GWM_START,
    /** Reconfiguration into a different AWM */
    RTLIB_EXC_GWM_RECONF,
    /** Migration and reconfiguration into a different AWM */
    RTLIB_EXC_GWM_MIGREC,
    /** Migration (still running on the same AWM) */
    RTLIB_EXC_GWM_MIGRATE,
    /** EXC suspended (resources not available) */
    RTLIB_EXC_GWM_BLOCKED
}