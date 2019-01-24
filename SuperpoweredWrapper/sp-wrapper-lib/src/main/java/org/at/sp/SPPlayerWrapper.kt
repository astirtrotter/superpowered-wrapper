package org.at.sp

/**
 * @author          - Yonis Larsson
 * @contact         - yonis.larsson.biz@gmail.com
 * @date            - 2/22/18
 */
object SPPlayerWrapper {

    init {
        System.loadLibrary("sp-wrapper-lib")
    }

    external fun init(tempFolderPath: String, samplerate: Int, buffersize: Int): Boolean
    external fun prepare(filePath1: String?, filePath2: String?): Boolean
    external fun play(): Boolean
    external fun pause(): Boolean
    external fun seek(ms: Double): Boolean
    external fun stop(): Boolean
    external fun release(): Boolean

    external fun getVolume(index: Int): Float
    external fun setVolume(index: Int, volume: Float)
    external fun getPitch(): Int
    external fun setPitch(pitch: Int): Boolean
    external fun playing(): Boolean
    external fun duration(): Int
    external fun position(): Double

    ////////////////////////////////////////////////////////////////////////////////////////////////

    @JvmStatic
    private fun onPlayCompleted() {
        listenr?.onPlayCompleted()
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////

    interface Listener {
        fun onPlayCompleted()
    }

    var listenr: Listener? = null
}