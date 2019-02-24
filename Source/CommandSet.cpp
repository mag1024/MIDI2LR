#include "CommandSet.h"
#ifdef _WIN32
#include <filesystem> //not available in XCode yet
namespace fs = std::filesystem;
#endif
#include <fstream>

CommandSet::CommandSet() : m_impl_(MakeImpl())
{ // manually insert unmapped at first position
   cmd_by_number_.emplace_back("unmapped");
   cmd_idx_["unmapped"] = 0;
   size_t idx = 1;
   for (const auto& bycategory : m_impl_.allcommands_) {
      std::vector<MenuStringT> menu_items_temp{};
      for (const auto& cmd_pair : bycategory.second) {
         cmd_by_number_.push_back(cmd_pair.first);
         cmd_idx_[cmd_pair.first] = idx++;
         menu_items_temp.emplace_back(cmd_pair.second);
      }
      menus_.emplace_back(bycategory.first);
      menu_entries_.emplace_back(std::move(menu_items_temp));
   }
}

CommandSet::Impl::Impl()
{
   try {
#ifdef _WIN32
      fs::path p{rsj::AppDataFilePath("MenuTrans.xml")};
#else
      const auto p = rsj::AppDataFilePath("MenuTrans.xml");
#endif
      std::ifstream infile(p);
      if (infile.is_open()) {
         cereal::XMLInputArchive iarchive(infile);
         iarchive(*this);
#ifdef _WIN32
         rsj::Log("Cereal controls archive loaded from " + juce::String(p.c_str()));
#else
         rsj::Log("Cereal controls archive loaded from " + p);
#endif
      }
      else
         rsj::LogAndAlertError(
             "Unable to load control settings from xml file. Unable to open file");
   }
   catch (const std::exception& e) {
      rsj::ExceptionResponse(typeid(this).name(), __func__, e);
      throw;
   }
}

const CommandSet::Impl& CommandSet::MakeImpl()
{
   static const Impl singleimpl;
   return singleimpl;
}

size_t CommandSet::CommandTextIndex(const std::string& command) const
{
   const auto found = cmd_idx_.find(command);
   if (found == cmd_idx_.end()) {
      rsj::LogAndAlertError("Command not found in CommandTextIndex: " + command);
      return 0;
   }
   return found->second;
}