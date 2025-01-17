// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/text_input_state.h"

namespace content {

TextInputState::TextInputState()
    : type(ui::TEXT_INPUT_TYPE_NONE),
      mode(ui::TEXT_INPUT_MODE_DEFAULT),
      flags(0),
      can_compose_inline(true),
      show_ime_if_needed(false) {}

TextInputState::TextInputState(const TextInputState& other) = default;

}  //  namespace content
