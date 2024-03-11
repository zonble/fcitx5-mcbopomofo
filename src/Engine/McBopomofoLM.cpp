// Copyright (c) 2022 and onwards The McBopomofo Authors.
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#include "McBopomofoLM.h"

#include <algorithm>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "gramambular2/reading_grid.h"

namespace McBopomofo {

McBopomofoLM::McBopomofoLM() {}

McBopomofoLM::~McBopomofoLM() {
  m_languageModel.close();
  m_userPhrases.close();
  m_excludedPhrases.close();
  m_phraseReplacement.close();
  m_associatedPhrases.close();
}

void McBopomofoLM::loadLanguageModel(const char* languageModelDataPath) {
  if (languageModelDataPath) {
    m_languageModel.close();
    m_languageModel.open(languageModelDataPath);
  }
}

bool McBopomofoLM::isDataModelLoaded() { return m_languageModel.isLoaded(); }

void McBopomofoLM::loadAssociatedPhrases(const char* associatedPhrasesPath) {
  if (associatedPhrasesPath) {
    m_associatedPhrases.close();
    m_associatedPhrases.open(associatedPhrasesPath);
  }
}

bool McBopomofoLM::isAssociatedPhrasesLoaded() {
  return m_associatedPhrases.isLoaded();
}

void McBopomofoLM::loadAssociatedPhrasesV2(const char* associatedPhrasesPath) {
  if (associatedPhrasesPath) {
    m_associatedPhrasesV2.close();
    m_associatedPhrasesV2.open(associatedPhrasesPath);
  }
}

void McBopomofoLM::loadUserPhrases(const char* userPhrasesDataPath,
                                   const char* excludedPhrasesDataPath) {
  if (userPhrasesDataPath) {
    m_userPhrases.close();
    m_userPhrases.open(userPhrasesDataPath);
  }
  if (excludedPhrasesDataPath) {
    m_excludedPhrases.close();
    m_excludedPhrases.open(excludedPhrasesDataPath);
  }
}

void McBopomofoLM::loadPhraseReplacementMap(const char* phraseReplacementPath) {
  if (phraseReplacementPath) {
    m_phraseReplacement.close();
    m_phraseReplacement.open(phraseReplacementPath);
  }
}

std::vector<Formosa::Gramambular2::LanguageModel::Unigram>
McBopomofoLM::getUnigrams(const std::string& key) {
  if (key == " ") {
    std::vector<Formosa::Gramambular2::LanguageModel::Unigram> spaceUnigrams;
    spaceUnigrams.emplace_back(" ", 0);
    return spaceUnigrams;
  }

  std::vector<Formosa::Gramambular2::LanguageModel::Unigram> allUnigrams;
  std::vector<Formosa::Gramambular2::LanguageModel::Unigram> userUnigrams;

  std::unordered_set<std::string> excludedValues;
  std::unordered_set<std::string> insertedValues;

  if (m_excludedPhrases.hasUnigrams(key)) {
    std::vector<Formosa::Gramambular2::LanguageModel::Unigram>
        excludedUnigrams = m_excludedPhrases.getUnigrams(key);
    transform(excludedUnigrams.begin(), excludedUnigrams.end(),
              inserter(excludedValues, excludedValues.end()),
              [](const Formosa::Gramambular2::LanguageModel::Unigram& u) {
                return u.value();
              });
  }

  if (m_userPhrases.hasUnigrams(key)) {
    std::vector<Formosa::Gramambular2::LanguageModel::Unigram> rawUserUnigrams =
        m_userPhrases.getUnigrams(key);
    userUnigrams = filterAndTransformUnigrams(rawUserUnigrams, excludedValues,
                                              insertedValues);
  }

  if (m_languageModel.hasUnigrams(key)) {
    std::vector<Formosa::Gramambular2::LanguageModel::Unigram>
        rawGlobalUnigrams = m_languageModel.getUnigrams(key);
    allUnigrams = filterAndTransformUnigrams(rawGlobalUnigrams, excludedValues,
                                             insertedValues);
  }

  // This relies on the fact that we always use the default separator.
  bool isKeyMultiSyllable =
      key.find(Formosa::Gramambular2::ReadingGrid::kDefaultSeparator) !=
      std::string::npos;

  // If key is multi-syllabic (for example, ㄉㄨㄥˋ-ㄈㄢˋ), we just
  // insert all collected userUnigrams on top of the unigrams fetched from
  // the database. If key is mono-syllabic (for example, ㄉㄨㄥˋ), then
  // we'll have to rewrite the collected userUnigrams.
  //
  // This is because, by default, user unigrams have a score of 0, which
  // guarantees that grid walks will choose them. This is problematic,
  // however, when a single-syllabic user phrase is competing with other
  // multisyllabic phrases that start with the same syllable. For example,
  // if a user has 丼 for ㄉㄨㄥˋ, and because that unigram has a score
  // of 0, no other phrases in the database that start with ㄉㄨㄥˋ would
  // be able to compete with it. Without the rewrite, ㄉㄨㄥˋ-ㄗㄨㄛˋ
  // would always result in "丼" + "作" instead of "動作" because the
  // node for "丼" would dominate the walk.
  if (isKeyMultiSyllable || allUnigrams.empty()) {
    allUnigrams.insert(allUnigrams.begin(), userUnigrams.begin(),
                       userUnigrams.end());
  } else if (!userUnigrams.empty()) {
    // Find the highest score from the existing allUnigrams.
    double topScore = std::numeric_limits<double>::lowest();
    for (const auto& unigram : allUnigrams) {
      if (unigram.score() > topScore) {
        topScore = unigram.score();
      }
    }

    // Boost by a very small number. This is the score for user phrases.
    constexpr double epsilon = 0.000000001;
    double boostedScore = topScore + epsilon;

    std::vector<Formosa::Gramambular2::LanguageModel::Unigram>
        rewrittenUserUnigrams;
    for (const auto& unigram : userUnigrams) {
      rewrittenUserUnigrams.emplace_back(
          Formosa::Gramambular2::LanguageModel::Unigram(unigram.value(),
                                                        boostedScore));
    }
    allUnigrams.insert(allUnigrams.begin(), rewrittenUserUnigrams.begin(),
                       rewrittenUserUnigrams.end());
  }

  return allUnigrams;
}

bool McBopomofoLM::hasUnigrams(const std::string& key) {
  if (key == " ") {
    return true;
  }

  if (!m_excludedPhrases.hasUnigrams(key)) {
    return m_userPhrases.hasUnigrams(key) || m_languageModel.hasUnigrams(key);
  }

  return !getUnigrams(key).empty();
}

std::string McBopomofoLM::getReading(const std::string& value) {
  std::vector<ParselessLM::FoundReading> foundReadings =
      m_languageModel.getReadings(value);
  double topScore = std::numeric_limits<double>::lowest();
  std::string topValue;
  for (const auto& foundReading : foundReadings) {
    if (foundReading.score > topScore) {
      topValue = foundReading.reading;
      topScore = foundReading.score;
    }
  }
  return topValue;
}

void McBopomofoLM::setPhraseReplacementEnabled(bool enabled) {
  m_phraseReplacementEnabled = enabled;
}

bool McBopomofoLM::phraseReplacementEnabled() const {
  return m_phraseReplacementEnabled;
}

void McBopomofoLM::setExternalConverterEnabled(bool enabled) {
  m_externalConverterEnabled = enabled;
}

bool McBopomofoLM::externalConverterEnabled() const {
  return m_externalConverterEnabled;
}

void McBopomofoLM::setExternalConverter(
    std::function<std::string(const std::string&)> externalConverter) {
  m_externalConverter = std::move(externalConverter);
}

void McBopomofoLM::setMacroConverter(
    std::function<std::string(const std::string&)> macroConverter) {
  m_macroConverter = std::move(macroConverter);
}

std::string McBopomofoLM::convertMacro(const std::string& input) {
  if (m_macroConverter != nullptr) {
    return m_macroConverter(input);
  }
  return input;
}

std::vector<Formosa::Gramambular2::LanguageModel::Unigram>
McBopomofoLM::filterAndTransformUnigrams(
    const std::vector<Formosa::Gramambular2::LanguageModel::Unigram> unigrams,
    const std::unordered_set<std::string>& excludedValues,
    std::unordered_set<std::string>& insertedValues) {
  std::vector<Formosa::Gramambular2::LanguageModel::Unigram> results;

  for (auto&& unigram : unigrams) {
    // excludedValues filters out the unigrams with the original value.
    // insertedValues filters out the ones with the converted value
    const std::string& originalValue = unigram.value();
    if (excludedValues.find(originalValue) != excludedValues.end()) {
      continue;
    }

    std::string value = originalValue;
    if (m_phraseReplacementEnabled) {
      std::string replacement = m_phraseReplacement.valueForKey(value);
      if (!replacement.empty()) {
        value = replacement;
      }
    }
    if (m_macroConverter) {
      std::string replacement = m_macroConverter(value);
      value = replacement;
    }
    if (m_externalConverterEnabled && m_externalConverter) {
      std::string replacement = m_externalConverter(value);
      value = replacement;
    }
    if (insertedValues.find(value) == insertedValues.end()) {
      results.emplace_back(value, unigram.score());
      insertedValues.insert(value);
    }
  }
  return results;
}

const std::vector<std::string> McBopomofoLM::associatedPhrasesForKey(
    const std::string& key) {
  return m_associatedPhrases.valuesForKey(key);
}

bool McBopomofoLM::hasAssociatedPhrasesForKey(const std::string& key) {
  return m_associatedPhrases.hasValuesForKey(key);
}

std::vector<AssociatedPhrasesV2::Phrase> McBopomofoLM::findAssociatedPhrasesV2(const std::string& prefixValue,
                                                                       const std::vector<std::string>& prefixReadings) const {
  return m_associatedPhrasesV2.findPhrases(prefixValue, prefixReadings);
}


}  // namespace McBopomofo
