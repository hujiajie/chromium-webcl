// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/mus/ws/display_binding.h"

#include "components/mus/ws/display.h"
#include "components/mus/ws/window_manager_access_policy.h"
#include "components/mus/ws/window_server.h"
#include "components/mus/ws/window_tree.h"
#include "mojo/shell/public/interfaces/connector.mojom.h"

namespace mus {
namespace ws {

DisplayBindingImpl::DisplayBindingImpl(mojom::WindowTreeHostRequest request,
                                       Display* display,
                                       const UserId& user_id,
                                       mojom::WindowTreeClientPtr client,
                                       WindowServer* window_server)
    : window_server_(window_server),
      user_id_(user_id),
      binding_(display, std::move(request)),
      client_(std::move(client)) {}

DisplayBindingImpl::~DisplayBindingImpl() {}

WindowTree* DisplayBindingImpl::CreateWindowTree(ServerWindow* root) {
  WindowTree* tree = window_server_->EmbedAtWindow(
      root, user_id_, std::move(client_),
      make_scoped_ptr(new WindowManagerAccessPolicy));
  tree->ConfigureWindowManager();
  return tree;
}

}  // namespace ws
}  // namespace mus
