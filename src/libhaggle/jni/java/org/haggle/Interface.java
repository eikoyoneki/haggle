package org.haggle;

public class Interface {
        private long nativeInterface = 0;
        private boolean disposed = false;

        private native void nativeFree();

        // Types - should be synced with the ones in the libhaggle
        // native code (interface.h).
        public final static int IFTYPE_UNDEF = 0;
	public final static int IFTYPE_APPLICATION_PORT = 1;
	public final static int IFTYPE_APPLICATION_LOCAL = 2;
	public final static int IFTYPE_BLUETOOTH = 3;
	public final static int IFTYPE_ETHERNET = 4;
	public final static int IFTYPE_WIFI = 5;
	public final static int IFTYPE_MEDIA = 6;
	public final static int _IFTYPE_MAX = 7;

	public final static int IFSTATUS_UNDEF = 0;
	public final static int IFSTATUS_UP = 1;
	public final static int IFSTATUS_DOWN = 2;
	public final static int _IFSTATUS_MAX = 3;
	
        public native int getType();
	public native int getStatus();
        public native String getName();
        public native String getIdentifierString();

        private Interface()
        {
                
        }
        public synchronized void dispose()
        {
                if (disposed == false) {
                        disposed = true;
                        nativeFree();
                }
        }
        protected void finalize() throws Throwable
        {
                dispose();
                super.finalize();
        }
        public class InterfaceException extends Exception {
                InterfaceException(String msg)
                {
                        super(msg);
                }
        }
        static {		
                System.loadLibrary("haggle_jni");
        }
}