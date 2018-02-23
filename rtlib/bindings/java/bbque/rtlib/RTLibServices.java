package bbque.rtlib;

import bbque.rtlib.exception.RTLibInitException;

public class RTLibServices {

    private final long mNativePointer;

    public RTLibServices(String name) throws RTLibInitException {
        mNativePointer = initNative(name);
    }

    private native long initNative(String name) throws RTLibInitException;
}