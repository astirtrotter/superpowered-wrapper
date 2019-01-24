package org.at.sp

/**
 * @author          - Yonis Larsson
 * @contact         - yonis.larsson.biz@gmail.com
 * @date            - 2/22/18
 */
object SPRecorderWrapper {

    init {
        System.loadLibrary("sp-wrapper-lib")
    }


    // if tempPath = null, the recording will be in mp3 format.
    external fun init(tempPath: String?, samplerate: Int, buffersize: Int, channels: Int): Boolean
    external fun prepare(path: String): Boolean
    external fun record(): Boolean
    external fun pause(): Boolean
    external fun stop(): Boolean
    external fun release(): Boolean

    ////////////////////////////////////////////////////////////////////////////////////////////////

    @JvmStatic
    private fun onPeakChanged(peak: Float) {
        listenr?.onPeakChanged(peak)
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////

    interface Listener {
        fun onPeakChanged(peak: Float)
    }

    var listenr: Listener? = null
}