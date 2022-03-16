#ifndef _FCITX5_MCBOPOMOFO_MCBOPOMOFO_H_
#define _FCITX5_MCBOPOMOFO_MCBOPOMOFO_H_

#include "InputState.h"
#include <fcitx-config/configuration.h>
#include <fcitx-config/enum.h>
#include <fcitx-config/iniparser.h>
#include <fcitx-utils/i18n.h>
#include <fcitx/addonfactory.h>
#include <fcitx/inputmethodengine.h>
#include <optional>

enum class BopomofoKeyboardLayout { Standard, Eten };
FCITX_CONFIG_ENUM_NAME_WITH_I18N(BopomofoKeyboardLayout, N_("standard"),
                                 N_("eten"));

FCITX_CONFIGURATION(
    McBopomofoConfig,
    // Keyboard layout: standard, eten, etc.

    fcitx::OptionWithAnnotation<BopomofoKeyboardLayout,
                                BopomofoKeyboardLayoutI18NAnnotation>
        bopomofoKeyboardLayout{this, "BopomofoKeyboardLayout",
                               _("Bopomofo Keyboard Layout"),
                               BopomofoKeyboardLayout::Standard};
    // Whether to map Dvorak characters back to Qwerty layout;
    // this is a workaround of fcitx5/wayland's limitations.
    // See https://bugzilla.gnome.org/show_bug.cgi?id=162726
    // TODO: Remove this once fcitx5 handles Dvorak better.
    fcitx::Option<bool> debugMapDvorakBackToQwerty{
        this, "DebugMapDvorakBackToQwerty",
        _("Debug Only - Map Dvorak back to Qwerty"), false};);

class McBopomofoEngine : public fcitx::InputMethodEngine {

public:
  McBopomofoEngine();

  void reset(const fcitx::InputMethodEntry& entry,
             fcitx::InputContextEvent& keyEvent) override;
  void keyEvent(const fcitx::InputMethodEntry& entry,
                fcitx::KeyEvent& keyEvent) override;

  const fcitx::Configuration* getConfig() const override;
  void setConfig(const fcitx::RawConfig& config) override;
  void reloadConfig() override;

private:
  void handle(McBopomofo::InputState* newState);
  void handleEmpty(McBopomofo::InputStateEmpty* newState,
                   McBopomofo::InputState* state);
  void handleEmptyIgnoringPrevious(
      McBopomofo::InputStateEmptyIgnoringPrevious* newState,
      McBopomofo::InputState* state);
  void handleCommitting(McBopomofo::InputStateCommitting* newState,
                        McBopomofo::InputState* state);
  void handleInputting(McBopomofo::InputStateInputting* newState,
                       McBopomofo::InputState* state);
  void handleCandidates(McBopomofo::InputStateChoosingCandidate* newState,
                        McBopomofo::InputState* state);

  McBopomofo::InputState* m_state;
  McBopomofoConfig config_;
  fcitx::KeyList selection_keys_;
  std::string foo_buffer_;
};

class McBopomofoEngineFactory : public fcitx::AddonFactory {
  fcitx::AddonInstance* create(fcitx::AddonManager* manager) override {
    FCITX_UNUSED(manager);
    return new McBopomofoEngine();
  }
};

#endif // _FCITX5_MCBOPOMOFO_MCBOPOMOFO_H_
