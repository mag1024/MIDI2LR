#pragma once
/*
==============================================================================

Misc.h

This file is part of MIDI2LR. Copyright 2015 by Rory Jaffe.

MIDI2LR is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

MIDI2LR is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
MIDI2LR.  If not, see <http://www.gnu.org/licenses/>.
==============================================================================
*/
#ifndef MIDI2LR_MISC_H_INCLUDED
#define MIDI2LR_MISC_H_INCLUDED
#include <atomic>
#include <chrono>
#include <exception>
#include <string>
#include <typeinfo> //for typeid, used in calls to ExceptionResponse
namespace juce {
   class String;
}

#ifdef NDEBUG // asserts disabled
static constexpr bool kNdebug = true;
#else // asserts enabled
static constexpr bool kNdebug = false;
#endif

#ifdef _WIN32
#include <emmintrin.h>
#define CPU_RELAX _mm_pause()
constexpr auto MSWindows{true};
constexpr auto OSX{false};
#else
#define CPU_RELAX __builtin_ia32_pause()
constexpr auto MSWindows{false};
constexpr auto OSX{true};
#endif

namespace rsj {
   [[nodiscard]] inline auto NowMs() noexcept
   {
      return ::std::chrono::time_point_cast<::std::chrono::milliseconds>(
          ::std::chrono::steady_clock::now())
          .time_since_epoch()
          .count();
   }
   using TimeType = decltype(NowMs());

   class RelaxTTasSpinLock {
    public:
      RelaxTTasSpinLock() noexcept = default;
      ~RelaxTTasSpinLock() = default;
      RelaxTTasSpinLock(const RelaxTTasSpinLock& other) = delete;
      RelaxTTasSpinLock(RelaxTTasSpinLock&& other) = delete;
      RelaxTTasSpinLock& operator=(const RelaxTTasSpinLock& other) = delete;
      RelaxTTasSpinLock& operator=(RelaxTTasSpinLock&& other) = delete;
      void lock() noexcept
      {
         do {
            while (flag_.load(::std::memory_order_relaxed))
               CPU_RELAX; // spin without expensive exchange
         } while (flag_.exchange(true, ::std::memory_order_acquire));
      }

      bool try_lock() noexcept
      {
         if (flag_.load(::std::memory_order_relaxed)) // avoid cache invalidation if lock
                                                      // unavailable
            return false;
         return !flag_.exchange(true, ::std::memory_order_acquire); // try to acquire lock
      }

      void unlock() noexcept
      {
         flag_.store(false, ::std::memory_order_release);
      }

    private:
      ::std::atomic<bool> flag_{false};
   };

   // typical call: rsj::ExceptionResponse(typeid(this).name(), __func__, e);
   void ExceptionResponse(const char* id, const char* fu, const ::std::exception& e) noexcept;
   void LogAndAlertError(const juce::String& error_text);
   void Log(const juce::String& info);
#ifdef _WIN32
   [[nodiscard]] ::std::wstring AppDataFilePath(const ::std::wstring& file_name);
   [[nodiscard]] ::std::wstring AppDataFilePath(const ::std::string& file_name);
   [[nodiscard]] inline ::std::wstring AppLogFilePath(const ::std::wstring& file_name)
   {
      return AppDataFilePath(file_name);
   }
   [[nodiscard]] inline ::std::wstring AppLogFilePath(const ::std::string& file_name)
   {
      return AppDataFilePath(file_name);
   }
#else
   [[nodiscard]] inline ::std::string AppDataFilePath(const ::std::string& file_name)
   {
      return "~/Library/Application Support/MIDI2LR/" + file_name;
   }
   [[nodiscard]] inline ::std::string AppLogFilePath(const ::std::string& file_name)
   {
      return "~/Library/Logs/MIDI2LR/" + file_name;
   }
#endif // def _WIN32
} // namespace rsj

#endif // MISC_H_INCLUDED
