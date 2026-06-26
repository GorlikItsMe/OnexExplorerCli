#pragma once

#include <string>

namespace onexexplorer {

  enum class LanguageCode { EN, DE, ES, FR };

  class OnexExplorer {
    std::string name;

  public:
    OnexExplorer(std::string name);

    std::string greet(LanguageCode lang = LanguageCode::EN) const;
  };

}  // namespace onexexplorer
