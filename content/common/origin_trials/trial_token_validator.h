// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_ORIGIN_TRIALS_TRIAL_TOKEN_VALIDATOR_H_
#define CONTENT_COMMON_ORIGIN_TRIALS_TRIAL_TOKEN_VALIDATOR_H_

#include <string>
#include "base/strings/string_piece.h"
#include "content/common/content_export.h"
#include "url/origin.h"

namespace content {

namespace TrialTokenValidator {

// This method is thread-safe.
CONTENT_EXPORT bool ValidateToken(const std::string& token,
                                  const url::Origin& origin,
                                  base::StringPiece featureName);

}  // namespace TrialTokenValidator

}  // namespace content

#endif  // CONTENT_COMMON_ORIGIN_TRIALS_TRIAL_TOKEN_VALIDATOR_H_
