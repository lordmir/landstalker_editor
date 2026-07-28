#ifndef _ALSA_PLAYER_H_
#define _ALSA_PLAYER_H_

// ALSA is Linux-specific; this whole file compiles to nothing elsewhere (macOS keeps using
// wxSound, whose OSX backend is fine - it's only the Unix/OSS one that's the problem; Windows'
// wxSound backend was always fine too).
#if defined(__linux__)

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

typedef struct _snd_pcm snd_pcm_t;

// Plays mono 8-bit unsigned PCM directly through ALSA. The device is opened once, at a single
// fixed rate, and a background thread feeds it continuously for the player's entire lifetime -
// silence when nothing is queued, the requested sample's bytes (resampled to the device's fixed
// rate) otherwise. It never stops or reconfigures the stream between samples.
//
// That continuous-feed design exists because starting or stopping a real hardware PCM stream is
// itself an audible event on most Linux audio setups (the DAC/link has to go active or idle) -
// independent of what data is in it. An earlier version of this class opened fresh (or
// reconfigured) per Play() and drained + let the stream go idle after each one; that produced a
// click at the start AND end of literally every sample, including replaying the exact same
// sample back to back - conclusively a stream-transition artifact, not anything about the sample
// bytes (confirmed separately: the unmodified raw data, exported to WAV and played through the
// normal OS audio stack, has no click at all). Keeping the stream permanently active removes the
// transition entirely instead of trying to hide it.
class AlsaPlayer
{
public:
	AlsaPlayer();
	~AlsaPlayer();

	AlsaPlayer(const AlsaPlayer&) = delete;
	AlsaPlayer& operator=(const AlsaPlayer&) = delete;

	// Hands pcm (at sample_rate) off to the engine thread to resample and start playing; returns
	// false only if the device isn't open at all. Play() itself does no resampling or other real
	// work - it just stores the request and returns, so it's cheap to call from the GUI thread no
	// matter how long the sample is. on_finished is invoked on the wxWidgets main thread once the
	// buffer has fully played out - not called if Stop() (or a subsequent Play()) supersedes it
	// first, including while it's still queued (not yet resampled/started).
	bool Play(const std::vector<uint8_t>& pcm, unsigned sample_rate, std::function<void()> on_finished);
	// Immediately stops playback (silence resumes on the next chunk) without a fade, so this - as
	// with any audio app's Stop button - can itself click; it is a genuine interruption, not the
	// idle-transition problem this class otherwise avoids. Safe to call when nothing is playing.
	void Stop();

private:
	void EngineThreadMain();
	static std::vector<uint8_t> Resample(const std::vector<uint8_t>& pcm, unsigned from_rate, unsigned to_rate);

	snd_pcm_t* m_handle = nullptr;
	std::thread m_engine_thread;
	std::atomic<bool> m_shutdown{ false };

	std::mutex m_mutex;
	// A queued request the engine thread hasn't picked up (and resampled) yet.
	bool m_has_pending = false;
	std::vector<uint8_t> m_pending_pcm;
	unsigned m_pending_rate = 0;
	std::function<void()> m_pending_on_finished;
	// The buffer actually being streamed, already at the device rate; empty = idle (write silence).
	std::vector<uint8_t> m_playing;
	std::size_t m_play_pos = 0;
	std::function<void()> m_on_finished;
};

#endif // __linux__

#endif // _ALSA_PLAYER_H_
