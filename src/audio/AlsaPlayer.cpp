#include <audio/AlsaPlayer.h>

#if defined(__linux__)

#include <algorithm>
#include <cmath>

#include <alsa/asoundlib.h>

namespace
{
	// A rate essentially every device supports natively, so the stream never needs to be
	// reconfigured once opened. The samples themselves run at various lower native rates (7-16
	// kHz); Resample() converts each one up to this before it's handed to the engine thread.
	constexpr unsigned DEVICE_RATE = 44100;
	// Device buffer depth. Generous, since nothing here is latency-sensitive (there's no live
	// input to stay in sync with) and a bigger buffer is more forgiving of scheduling jitter in
	// the engine thread.
	constexpr unsigned LATENCY_US = 200000;
	// Frames per engine thread iteration - small enough that Play()/Stop() are picked up quickly
	// (~6ms at 44.1kHz), large enough to keep the thread's wake-up rate reasonable.
	constexpr snd_pcm_uframes_t CHUNK_FRAMES = 256;
}

AlsaPlayer::AlsaPlayer()
{
	if (snd_pcm_open(&m_handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0)
	{
		m_handle = nullptr;
		return;
	}
	if (snd_pcm_set_params(m_handle, SND_PCM_FORMAT_U8, SND_PCM_ACCESS_RW_INTERLEAVED,
		1 /*mono*/, DEVICE_RATE, 1 /*allow soft resample*/, LATENCY_US) < 0)
	{
		snd_pcm_close(m_handle);
		m_handle = nullptr;
		return;
	}
	m_engine_thread = std::thread(&AlsaPlayer::EngineThreadMain, this);
}

AlsaPlayer::~AlsaPlayer()
{
	m_shutdown = true;
	if (m_engine_thread.joinable())
	{
		m_engine_thread.join();
	}
	if (m_handle)
	{
		snd_pcm_close(m_handle);
	}
}

std::vector<uint8_t> AlsaPlayer::Resample(const std::vector<uint8_t>& pcm, unsigned from_rate, unsigned to_rate)
{
	if (pcm.empty() || from_rate == 0 || from_rate == to_rate)
	{
		return pcm;
	}
	const std::size_t out_len = static_cast<std::size_t>(
		static_cast<double>(pcm.size()) * to_rate / from_rate + 0.5);
	std::vector<uint8_t> out(out_len);
	for (std::size_t i = 0; i < out_len; ++i)
	{
		const double src_pos = static_cast<double>(i) * from_rate / to_rate;
		const std::size_t i0 = static_cast<std::size_t>(src_pos);
		const std::size_t i1 = std::min(i0 + 1, pcm.size() - 1);
		const double frac = src_pos - static_cast<double>(i0);
		const double v = pcm[i0] * (1.0 - frac) + pcm[i1] * frac;
		out[i] = static_cast<uint8_t>(std::lround(std::clamp(v, 0.0, 255.0)));
	}
	return out;
}

bool AlsaPlayer::Play(const std::vector<uint8_t>& pcm, unsigned sample_rate, std::function<void()> on_finished)
{
	if (!m_handle || pcm.empty() || sample_rate == 0)
	{
		return false;
	}
	// No resampling (or any other real work) here - just stash the request. The engine thread
	// resamples it once it picks this up, off the GUI thread entirely, so Play() stays cheap and
	// near-instant regardless of how long the sample is.
	std::lock_guard<std::mutex> lock(m_mutex);
	m_pending_pcm = pcm;
	m_pending_rate = sample_rate;
	m_pending_on_finished = std::move(on_finished);
	m_has_pending = true;
	return true;
}

void AlsaPlayer::Stop()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_has_pending = false;
	m_pending_pcm.clear();
	m_pending_on_finished = nullptr;
	m_playing.clear();
	m_play_pos = 0;
	m_on_finished = nullptr; // dropped, not called - this is an interruption, not completion
	m_draining = false;
	m_drain_silence_frames = 0;
}

void AlsaPlayer::EngineThreadMain()
{
	std::vector<uint8_t> chunk(CHUNK_FRAMES);
	while (!m_shutdown)
	{
		// Pick up a queued request, if any, and resample it here on the engine thread - never
		// under the lock, since it's the one potentially-slow part of all this.
		std::vector<uint8_t> pending_pcm;
		unsigned pending_rate = 0;
		std::function<void()> pending_on_finished;
		bool have_pending = false;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (m_has_pending)
			{
				pending_pcm = std::move(m_pending_pcm);
				pending_rate = m_pending_rate;
				pending_on_finished = std::move(m_pending_on_finished);
				m_pending_pcm.clear();
				m_pending_on_finished = nullptr;
				m_has_pending = false;
				have_pending = true;
			}
		}
		if (have_pending)
		{
			auto resampled = Resample(pending_pcm, pending_rate, DEVICE_RATE);
			std::lock_guard<std::mutex> lock(m_mutex);
			m_playing = std::move(resampled);
			m_play_pos = 0;
			m_on_finished = std::move(pending_on_finished);
			m_draining = false; // any previous sample's pending completion is superseded
			m_drain_silence_frames = 0;
		}

		std::size_t content_frames = 0;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (!m_playing.empty() && m_play_pos < m_playing.size())
			{
				content_frames = std::min<std::size_t>(CHUNK_FRAMES, m_playing.size() - m_play_pos);
				std::copy(m_playing.begin() + m_play_pos, m_playing.begin() + m_play_pos + content_frames, chunk.begin());
				std::fill(chunk.begin() + content_frames, chunk.end(), static_cast<uint8_t>(128));
				m_play_pos += content_frames;
				if (m_play_pos >= m_playing.size())
				{
					// All content is queued to the device now, but up to the device buffer depth
					// of it is still to be played - switch to drain-tracking rather than firing
					// on_finished while the tail is still audible.
					m_playing.clear();
					m_play_pos = 0;
					m_draining = true;
					m_drain_silence_frames = 0;
				}
			}
			else
			{
				std::fill(chunk.begin(), chunk.end(), static_cast<uint8_t>(128));
			}
		}

		snd_pcm_sframes_t written = snd_pcm_writei(m_handle, chunk.data(), chunk.size());
		if (written < 0)
		{
			// Recover (e.g. from an underrun) and retry once; if that also fails just move on to
			// the next chunk rather than getting stuck.
			snd_pcm_recover(m_handle, static_cast<int>(written), 1);
		}

		std::function<void()> fire_on_finished;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (m_draining)
			{
				if (written > 0 && static_cast<std::size_t>(written) > content_frames)
				{
					m_drain_silence_frames += static_cast<std::size_t>(written) - content_frames;
				}
				// The device's delay counts every queued-but-unplayed frame. Once it's no more
				// than the silence queued since the content's final frame, the content itself has
				// fully played. A delay query error just falls back to firing immediately.
				snd_pcm_sframes_t delay = 0;
				if (snd_pcm_delay(m_handle, &delay) < 0
					|| delay <= static_cast<snd_pcm_sframes_t>(m_drain_silence_frames))
				{
					m_draining = false;
					m_drain_silence_frames = 0;
					fire_on_finished = std::move(m_on_finished);
					m_on_finished = nullptr;
				}
			}
		}

		if (fire_on_finished)
		{
			fire_on_finished();
		}
	}
}

#endif // __linux__
