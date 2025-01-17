// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/media_stream_track.h"

namespace content {

// static
MediaStreamTrack* MediaStreamTrack::GetTrack(
    const blink::WebMediaStreamTrack& track) {
  return track.isNull() ? nullptr
                        : static_cast<MediaStreamTrack*>(track.getExtraData());
}

MediaStreamTrack::MediaStreamTrack(bool is_local_track)
    : is_local_track_(is_local_track) {
}

MediaStreamTrack::~MediaStreamTrack() {
}

}  // namespace content
