package org.at.sp.demo

import android.content.Context
import android.media.AudioManager
import android.os.Bundle
import android.os.Handler
import android.support.v7.app.AppCompatActivity
import android.util.Log
import android.view.View
import android.widget.SeekBar
import android.widget.Toast
import kotlinx.android.synthetic.main.activity_main.*
import org.at.sp.SPPlayerWrapper
import org.at.sp.SPRecorderWrapper
import java.util.*

class MainActivity : AppCompatActivity() {

    private var timer: Timer? = null;
    private var isSeeking = false

    companion object {
        private const val playerFilePath1 = "/sdcard/music/music1.mp3"
        private const val playerFilePath2 = "/sdcard/music/music2.mp3"

        private const val recorderFilePath1 = "/sdcard/value" // ".wav" will be appended automatically.

        private fun durationText(time: Int): String {
            val min = time / 60000
            val sec = (time % 60000) / 1000
            val ret = StringBuffer()
            if (min < 10) ret.append('0')
            ret.append(min).append(':')
            if (sec < 10) ret.append('0')
            ret.append(sec)

            return ret.toString()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        playerPath1.setText(playerFilePath1)
        playerPath2.setText(playerFilePath2)
        recorderPath.setText(recorderFilePath1)

        playerSeeker.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {}

            override fun onStartTrackingTouch(seekBar: SeekBar?) {
                isSeeking = true
            }

            override fun onStopTrackingTouch(seekBar: SeekBar?) {
                val pos = SPPlayerWrapper.duration() * playerSeeker.progress / playerSeeker.max
                SPPlayerWrapper.seek(pos.toDouble())
                playerPosition.text = durationText(pos)
                isSeeking = false
            }
        })

        with(volume1Seekbar) {
            progress = (SPPlayerWrapper.getVolume(0) * max).toInt()
            setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
                override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                    SPPlayerWrapper.setVolume(0, progress.toFloat() / 100)
                }

                override fun onStartTrackingTouch(seekBar: SeekBar?) {}

                override fun onStopTrackingTouch(seekBar: SeekBar?) {}

            })
        }

        with(volume2Seekbar) {
            progress = (SPPlayerWrapper.getVolume(1) * max).toInt()
            setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
                override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                    SPPlayerWrapper.setVolume(1, progress.toFloat() / 100)
                }

                override fun onStartTrackingTouch(seekBar: SeekBar?) {}

                override fun onStopTrackingTouch(seekBar: SeekBar?) {}
            })
        }
    }

    private fun toast(msg: String) = runOnUiThread {
        Toast.makeText(this, msg, Toast.LENGTH_LONG).show()
    }

    fun onPlayerInit(view: View) {
        val audioManager = this.getSystemService(Context.AUDIO_SERVICE) as AudioManager
        val samplerateString = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE)
        val buffersizeString = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER)

        if (SPPlayerWrapper.init(cacheDir.absolutePath, Integer.parseInt(samplerateString), Integer.parseInt(buffersizeString))) {
            toast("Successfully initialized.")
        } else {
            toast("Failed to init. You should <Release> first.")
        }
    }

    fun onPlayerOpen(view: View) {
        val path1 = playerPath1.text.toString()
        val path2 = playerPath2.text.toString()

        var path11: String? = null
        var path22: String? = null
        if (path1.isBlank()) {
            if (path2.isBlank()) {
                toast("Invalid input. At least 1 input should not be empty.")
                return
            }
            path11 = path2
        } else {
            path11 = path1
            path22 = path2.takeIf { !path2.isBlank() }
        }
        if (SPPlayerWrapper.prepare(path11, path22)) {
            toast("Successfully opened.")

            playerPlayPause.text = "play"

            // onLoadSuccess
            Handler().postDelayed({
                playerSeeker.progress = 0
                playerPosition.text = durationText(0)
                playerDuration.text = durationText(SPPlayerWrapper.duration())
            }, 1000)
        } else {
            toast("Failed to open. You should <Init> first or the files don't exist.")
        }
    }

    fun onPlayerPlayPause(view: View) {
        if (!"play".equals(playerPlayPause.text as String, true)) {
            if (SPPlayerWrapper.pause()) {
                playerPlayPause.text = "play"
                timer?.cancel()
            } else {
                toast("You should <Open> first.")
            }
        } else {
            if (SPPlayerWrapper.play()) {
                playerPlayPause.text = "pause"
                timer = Timer().apply {
                    schedule(object : TimerTask() {
                        override fun run() {
                            if (isSeeking) return

                            if (!SPPlayerWrapper.playing()) {
                                timer?.cancel()
                                runOnUiThread {
                                    playerPlayPause.text = "play"
                                    playerSeeker.progress = 0
                                    playerPosition.text = durationText(0)
                                }
                                return
                            }

                            val pos = SPPlayerWrapper.position()
                            val duration = SPPlayerWrapper.duration()
                            val progress = pos * playerSeeker.max / duration

                            Log.d("Player", "progress: $progress")

                            runOnUiThread {
                                playerSeeker.progress = progress.toInt()
                                playerPosition.text = durationText(pos.toInt())
                            }
                        }
                    }, 1000, 200)
                }
            } else {
                toast("You should <Open> first.")
            }
        }
    }

    fun onPlayerStop(view: View) {
        if (SPPlayerWrapper.stop()) {
            playerPlayPause.text = "play"
            timer?.cancel()

            playerSeeker.progress = 0
            playerPosition.text = durationText(0)
        } else {
            toast("You should <Open> first.")
        }
    }

    fun onPlayerRelease(view: View) {
        if (SPPlayerWrapper.release()) {
            playerPlayPause.text = "play"
            timer?.cancel()

            playerSeeker.progress = 0
            playerPosition.text = durationText(0)
            playerDuration.text = durationText(0)
        } else {
            toast("You should <Init> first.")
        }
    }

    fun onRecorderRecordInit(view: View) {
        val audioManager = this.getSystemService(Context.AUDIO_SERVICE) as AudioManager
        val samplerateString = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE)
        val buffersizeString = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER)

        var tempPath: String? = null
        if (recWav.isChecked) {
            tempPath = cacheDir.absolutePath + "/recorder.tmp"
        }
        if (SPRecorderWrapper.init(tempPath, Integer.parseInt(samplerateString), Integer.parseInt(buffersizeString), 1.takeIf { recMono.isChecked } ?: 2)) {
            toast("Successfully initialized.")
        } else {
            toast("Failed to init. You should <Release> first.")
        }
    }

    fun onRecorderRecordPrepare(view: View) {
        if (SPRecorderWrapper.prepare(recorderPath.text.toString())) {
            recorderStartPause.text = "rec"

            toast("Successfully prepared.")
        } else {
            toast("You should <Init> first. Or the input is invalid.")
        }
    }

    fun onRecorderRecordPause(view: View) {
        if (!"rec".equals(recorderStartPause.text as String, true)) {
            if (SPRecorderWrapper.pause()) {
                recorderStartPause.text = "rec"
            } else {
                toast("You should <Prepare> first.")
            }
        } else {
            if (SPRecorderWrapper.record()) {
                recorderStartPause.text = "pause"
            } else {
                toast("You should <Prepare> first.")
            }
        }
    }

    fun onRecorderStop(view: View) {
        if (SPRecorderWrapper.stop()) {
            recorderStartPause.text = "rec"
        } else {
            toast("You should <Prepare> first.")
        }
    }

    fun onRecorderRelease(view: View) {
        if (SPRecorderWrapper.release()) {
            recorderStartPause.text = "rec"
        } else {
            toast("You should <Init> first.")
        }
    }
}
