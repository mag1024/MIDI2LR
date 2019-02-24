/*
  ==============================================================================

    Misc.cpp

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
#include "Misc.h"
#include "UtfUtilities.h"
#include "../JuceLibraryCode/JuceHeader.h"
#ifdef _WIN32
#include <gsl/gsl_util>
#include <ShlObj.h>
#include <Windows.h>
#endif

// from http://www.cplusplus.com/forum/beginner/175177 and
// https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/libsupc%2B%2B/cxxabi.h#L156

#ifdef __GNUG__ // gnu C++ compiler
#include <cxxabi.h>
#include <memory>
#include <type_traits>
template<typename T>
[[nodiscard]] T Demangle(const char* mangled_name) {
   static_assert(std::is_pointer<T>() == false, "Result must be copied as __cxa_demagle returns "
                                                "pointer to temporary. Cannot use pointer type for "
                                                "this template.");
   std::size_t len = 0;
   int status = 0;
   std::unique_ptr<char, decltype(&std::free)> ptr(
       abi::__cxa_demangle(mangled_name, nullptr, &len, &status), &std::free);
   if (status)
      return mangled_name;
   return ptr.get();
}
#else  // ndef _GNUG_
template<typename T>
[[nodiscard]] T Demangle(const char* mangled_name) { return mangled_name; }
#endif // _GNUG_
void rsj::Log(const juce::String& info)
{
   if (juce::Logger::getCurrentLogger())
      juce::Logger::writeToLog(juce::Time::getCurrentTime().toISO8601(false) + ": " + info);
}

void rsj::LogAndAlertError(const juce::String& error_text)
{
   juce::NativeMessageBox::showMessageBox(juce::AlertWindow::WarningIcon, "Error", error_text);
   rsj::Log(error_text);
}

#pragma warning(push)
#pragma warning(disable : 26447)
// use typeid(this).name() for first argument to add class information
// typical call: rsj::ExceptionResponse(typeid(this).name(), __func__, e);
void rsj::ExceptionResponse(const char* id, const char* fu, const std::exception& e) noexcept
{
   try {
      const auto error_text{juce::String("Exception ") + e.what() + ' ' + Demangle<juce::String>(id)
                            + "::" + fu + " Version " + ProjectInfo::versionString};
      rsj::LogAndAlertError(error_text);
   }
   catch (...) { //-V565
   }
}
#pragma warning(pop)

#ifdef _WIN32
std::wstring rsj::AppDataFilePath(const std::wstring& file_name)
{
   wchar_t* pathptr{nullptr};
   auto dp = gsl::finally([&pathptr] {
      if (pathptr)
         CoTaskMemFree(pathptr);
   });
   const auto hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &pathptr);
   if (SUCCEEDED(hr))
      return std::wstring(pathptr) + L"\\MIDI2LR\\" + file_name;
   return std::wstring(file_name);
}

std::wstring rsj::AppDataFilePath(const std::string& file_name)
{
   return rsj::AppDataFilePath(rsj::UtfConvert<wchar_t>(file_name));
}
#endif