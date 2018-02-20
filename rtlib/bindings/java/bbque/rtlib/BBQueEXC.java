package bbque.rtlib;

import bbque.rtlib.enumeration.RTLibExitCode;
import bbque.rtlib.enumeration.RTLibResourceType;
import bbque.rtlib.exception.*;

public abstract class BBQueEXC {

    private static final int ERROR_LIB_INIT = -1;
    private static final int ERROR_EXECUTION_CONTEXT_REGISTRATION = -2;

    private final long mNativePointer;
    private boolean mDisposed;

    public BBQueEXC(String name, String recipe) throws RTLibException {
        mNativePointer = initNative(name, recipe);
        mDisposed = false;
        if (mNativePointer <= 0) {
            if (mNativePointer == ERROR_LIB_INIT) {
                throw new RTLibInitException("RTLib initialization failed");
            } else if (mNativePointer == ERROR_EXECUTION_CONTEXT_REGISTRATION) {
                throw new RTLibRegistrationException("RTLib execution context registration failed");
            } else {
                throw new RTLibException(String.format("Unknown error, RTLib returned pointer: %d", mNativePointer));
            }
        }
    }

    public synchronized void dispose() {
        if (!mDisposed) {
            disposeNative(mNativePointer);
            mDisposed = true;
        } else {
            throw new IllegalStateException("Executor already disposed");
        }
    }

    private native long initNative(String name, String recipe);

    private native void disposeNative(long nativePointer);

    /***********************************************************************************************************
     ********************************** AEM EXECUTION CONTEXT MANAGEMENT ***************************************
     ***********************************************************************************************************/

    public native boolean isRegistered();

    public native void start() throws RTLibException;

    public native void waitCompletion() throws RTLibException;

    public native void terminate() throws RTLibException;

    public native void enable() throws RTLibException;

    public native void disable() throws RTLibException;

    /***********************************************************************************************************
     ************************************ AEM CONSTRAINTS MANAGEMENT *******************************************
     ***********************************************************************************************************/

    public native void setAWMConstraints(RTLibConstraint[] constraints) throws RTLibException;

    public native void clearAWMConstraints() throws RTLibException;

    public native void setGoalGap(int goalGapPercent) throws RTLibException;

    /***********************************************************************************************************
     ******************************************* AEM UTILITIES *************************************************
     ***********************************************************************************************************/

    public native String getUniqueID_String();

    /*TODO: is getUniqueId() necessary? */

    public native int getAssignedResources(RTLibResourceType resourceType) throws RTLibException;

    public native int[] getAffinityMask(int[] ids) throws RTLibException;

    public native int[] getAssignedResources(RTLibResourceType resourceType, int[] systems) throws RTLibException;

    public native void setCPS(float cps) throws RTLibException;

    public native void setJPSGoal(float jpsMin, float jpsMax, int jpc) throws RTLibException;

    public native void updateJPC(int jpc) throws RTLibException;

    public native void setCPSGoal(float cpsMin, float cpsMax) throws RTLibException;

    public native void setCPSGoal(float cps) throws RTLibException;

    public native void setMinimumCycleTimeUs(int minCycleTimeUs) throws RTLibException;

    public native float getCPS();

    public native float getJPS();

    public native int getCTimeUs();

    public native int getCTimeMs();

    public native int cycles();

    public native RTLibWorkingModeParams workingModeParams();

    public native boolean done();

    public native int currentAWM();

    public native RTLibConf configuration();

    /***********************************************************************************************************
     ************************************ AEM APPLICATION CALLBACKS ********************************************
     ***********************************************************************************************************/

    protected abstract void onSetup() throws RTLibException;

    protected abstract void onConfigure(int awmId) throws RTLibException;

    protected abstract void onSuspend() throws RTLibException;

    protected abstract void onResume() throws RTLibException;

    protected abstract void onRun() throws RTLibException;

    protected abstract void onMonitor() throws RTLibException;

    protected abstract void onRelease() throws RTLibException;

    /***********************************************************************************************************
     ************************************* NATIVE CALLBACKS BRIDGE *********************************************
     ***********************************************************************************************************/

    private RTLibExitCode onSetupCallback() {
        try {
            onSetup();
        } catch (RTLibException e) {
            return e.getExitCode();
        }
        return RTLibExitCode.RTLIB_OK;
    }

    private RTLibExitCode onConfigureCallback(int awmId) {
        try {
            onConfigure(awmId);
        } catch (RTLibException e) {
            return e.getExitCode();
        }
        return RTLibExitCode.RTLIB_OK;
    }

    private RTLibExitCode onSuspendCallback() {
        try {
            onSuspend();
        } catch (RTLibException e) {
            return e.getExitCode();
        }
        return RTLibExitCode.RTLIB_OK;
    }

    private RTLibExitCode onResumeCallback() {
        try {
            onResume();
        } catch (RTLibException e) {
            return e.getExitCode();
        }
        return RTLibExitCode.RTLIB_OK;
    }

    private RTLibExitCode onRunCallback() {
        try {
            onRun();
        } catch (RTLibException e) {
            return e.getExitCode();
        }
        return RTLibExitCode.RTLIB_OK;
    }

    private RTLibExitCode onMonitorCallback() {
        try {
            onMonitor();
        } catch (RTLibException e) {
            return e.getExitCode();
        }
        return RTLibExitCode.RTLIB_OK;
    }

    private RTLibExitCode onReleaseCallback() {
        try {
            onRelease();
        } catch (RTLibException e) {
            return e.getExitCode();
        }
        return RTLibExitCode.RTLIB_OK;
    }
}