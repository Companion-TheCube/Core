/*
MIT License
Copyright (c) 2025 A-McD Technology LLC
*/
#pragma once

namespace audio {

// Core audio parameters
inline constexpr unsigned int SAMPLE_RATE = 16000;          // Hz
inline constexpr unsigned int NUM_CHANNELS = 1;             // mono
inline constexpr unsigned int BITS_PER_SAMPLE = 16;         // bits
inline constexpr unsigned int BYTES_PER_SAMPLE = BITS_PER_SAMPLE / 8;

// Capture callback frame size (was ROUTER_FIFO_SIZE)
inline constexpr unsigned int ROUTER_FIFO_FRAMES = 1280;    // ~32ms at 16kHz

// Wake/utterance related
inline constexpr unsigned int PRE_TRIGGER_SECONDS = 5;      // seconds of history
inline constexpr int WAKEWORD_RETRIGGER_MS = 5000;          // debounce
inline constexpr double SILENCE_TIMEOUT_SEC = 0.7;          // future use

} // namespace audio

