#pragma once

#include <string>

namespace onexexplorer {

  /** Language codes supported by OnexExplorer. */
  enum class LanguageCode { EN, DE, ES, FR };

  /**
   * @brief A class for producing localized greeting strings.
   */
  class OnexExplorer {
    std::string name;

  public:
    /**
     * @brief Creates a new instance.
     * @param name the name to greet
     */
    OnexExplorer(std::string name);

    /**
     * @brief Creates a localized string containing the greeting.
     * @param lang the language to greet in
     * @return a string containing the greeting
     */
    std::string greet(LanguageCode lang = LanguageCode::EN) const;
  };

}  // namespace onexexplorer
