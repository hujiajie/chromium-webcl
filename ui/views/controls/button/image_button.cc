// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/button/image_button.h"

#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "ui/accessibility/ax_view_state.h"
#include "ui/gfx/animation/throb_animation.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/views/painter.h"
#include "ui/views/widget/widget.h"

namespace views {

// Default button size if no image is set. This is ignored if there is an image,
// and exists for historical reasons (any number of clients could depend on this
// behaviour).
static const int kDefaultWidth = 16;
static const int kDefaultHeight = 14;

const char ImageButton::kViewClassName[] = "ImageButton";

////////////////////////////////////////////////////////////////////////////////
// ImageButton, public:

ImageButton::ImageButton(ButtonListener* listener)
    : CustomButton(listener),
      h_alignment_(ALIGN_LEFT),
      v_alignment_(ALIGN_TOP),
      draw_image_mirrored_(false),
      focus_painter_(Painter::CreateDashedFocusPainter()) {
  // By default, we request that the gfx::Canvas passed to our View::OnPaint()
  // implementation is flipped horizontally so that the button's images are
  // mirrored when the UI directionality is right-to-left.
  EnableCanvasFlippingForRTLUI(true);
}

ImageButton::~ImageButton() {
}

const gfx::ImageSkia& ImageButton::GetImage(ButtonState state) const {
  return images_[state];
}

void ImageButton::SetImage(ButtonState for_state, const gfx::ImageSkia* image) {
  images_[for_state] = image ? *image : gfx::ImageSkia();
  PreferredSizeChanged();
  if (state() == for_state)
    SchedulePaint();
}

void ImageButton::SetBackground(SkColor color,
                                const gfx::ImageSkia* image,
                                const gfx::ImageSkia* mask) {
  if (image == NULL || mask == NULL) {
    background_image_ = gfx::ImageSkia();
    return;
  }

  background_image_ = gfx::ImageSkiaOperations::CreateButtonBackground(color,
     *image, *mask);
}

void ImageButton::SetImageAlignment(HorizontalAlignment h_align,
                                    VerticalAlignment v_align) {
  h_alignment_ = h_align;
  v_alignment_ = v_align;
  SchedulePaint();
}

void ImageButton::SetFocusPainter(scoped_ptr<Painter> focus_painter) {
  focus_painter_ = std::move(focus_painter);
}

void ImageButton::SetMinimumImageSize(const gfx::Size& size) {
  if (minimum_image_size_ == size)
    return;

  minimum_image_size_ = size;
  PreferredSizeChanged();
}

////////////////////////////////////////////////////////////////////////////////
// ImageButton, View overrides:

gfx::Size ImageButton::GetPreferredSize() const {
  gfx::Size size(kDefaultWidth, kDefaultHeight);
  if (!images_[STATE_NORMAL].isNull()) {
    size = gfx::Size(images_[STATE_NORMAL].width(),
                     images_[STATE_NORMAL].height());
  }

  size.SetToMax(minimum_image_size_);

  gfx::Insets insets = GetInsets();
  size.Enlarge(insets.width(), insets.height());
  return size;
}

const char* ImageButton::GetClassName() const {
  return kViewClassName;
}

void ImageButton::OnPaint(gfx::Canvas* canvas) {
  // Call the base class first to paint any background/borders.
  View::OnPaint(canvas);

  // TODO(estade|tdanderson|bruthig): The ink drop layer should be positioned
  // behind the button's image which means the image needs to be painted to its
  // own layer instead of to the Canvas.
  gfx::ImageSkia img = GetImageToPaint();

  if (!img.isNull()) {
    gfx::ScopedCanvas scoped(canvas);
    if (draw_image_mirrored_) {
      canvas->Translate(gfx::Vector2d(width(), 0));
      canvas->Scale(-1, 1);
    }

    gfx::Point position = ComputeImagePaintPosition(img);
    if (!background_image_.isNull())
      canvas->DrawImageInt(background_image_, position.x(), position.y());

    canvas->DrawImageInt(img, position.x(), position.y());
  }

  Painter::PaintFocusPainter(this, canvas, focus_painter());
}

////////////////////////////////////////////////////////////////////////////////
// ImageButton, protected:

void ImageButton::OnFocus() {
  CustomButton::OnFocus();
  if (focus_painter_.get())
    SchedulePaint();
}

void ImageButton::OnBlur() {
  CustomButton::OnBlur();
  if (focus_painter_.get())
    SchedulePaint();
}

gfx::ImageSkia ImageButton::GetImageToPaint() {
  gfx::ImageSkia img;

  if (!images_[STATE_HOVERED].isNull() && hover_animation().is_animating()) {
    img = gfx::ImageSkiaOperations::CreateBlendedImage(
        images_[STATE_NORMAL], images_[STATE_HOVERED],
        hover_animation().GetCurrentValue());
  } else {
    img = images_[state()];
  }

  return !img.isNull() ? img : images_[STATE_NORMAL];
}

////////////////////////////////////////////////////////////////////////////////
// ImageButton, private:

gfx::Point ImageButton::ComputeImagePaintPosition(const gfx::ImageSkia& image) {
  int x = 0, y = 0;
  gfx::Rect rect = GetContentsBounds();

  HorizontalAlignment h_alignment = h_alignment_;
  if (draw_image_mirrored_) {
    if (h_alignment == ALIGN_RIGHT)
      h_alignment = ALIGN_LEFT;
    else if (h_alignment == ALIGN_LEFT)
      h_alignment = ALIGN_RIGHT;
  }

  if (h_alignment == ALIGN_CENTER)
    x = (rect.width() - image.width()) / 2;
  else if (h_alignment == ALIGN_RIGHT)
    x = rect.width() - image.width();

  if (v_alignment_ == ALIGN_MIDDLE)
    y = (rect.height() - image.height()) / 2;
  else if (v_alignment_ == ALIGN_BOTTOM)
    y = rect.height() - image.height();

  x += rect.x();
  y += rect.y();

  return gfx::Point(x, y);
}

////////////////////////////////////////////////////////////////////////////////
// ToggleImageButton, public:

ToggleImageButton::ToggleImageButton(ButtonListener* listener)
    : ImageButton(listener),
      toggled_(false) {
}

ToggleImageButton::~ToggleImageButton() {
}

void ToggleImageButton::SetToggled(bool toggled) {
  if (toggled == toggled_)
    return;

  for (int i = 0; i < STATE_COUNT; ++i) {
    gfx::ImageSkia temp = images_[i];
    images_[i] = alternate_images_[i];
    alternate_images_[i] = temp;
  }
  toggled_ = toggled;
  SchedulePaint();

  NotifyAccessibilityEvent(ui::AX_EVENT_VALUE_CHANGED, true);
}

void ToggleImageButton::SetToggledImage(ButtonState image_state,
                                        const gfx::ImageSkia* image) {
  if (toggled_) {
    images_[image_state] = image ? *image : gfx::ImageSkia();
    if (state() == image_state)
      SchedulePaint();
  } else {
    alternate_images_[image_state] = image ? *image : gfx::ImageSkia();
  }
}

void ToggleImageButton::SetToggledTooltipText(const base::string16& tooltip) {
  toggled_tooltip_text_ = tooltip;
}

////////////////////////////////////////////////////////////////////////////////
// ToggleImageButton, ImageButton overrides:

const gfx::ImageSkia& ToggleImageButton::GetImage(
    ButtonState image_state) const {
  if (toggled_)
    return alternate_images_[image_state];
  return images_[image_state];
}

void ToggleImageButton::SetImage(ButtonState image_state,
                                 const gfx::ImageSkia* image) {
  if (toggled_) {
    alternate_images_[image_state] = image ? *image : gfx::ImageSkia();
  } else {
    images_[image_state] = image ? *image : gfx::ImageSkia();
    if (state() == image_state)
      SchedulePaint();
  }
  PreferredSizeChanged();
}

////////////////////////////////////////////////////////////////////////////////
// ToggleImageButton, View overrides:

bool ToggleImageButton::GetTooltipText(const gfx::Point& p,
                                       base::string16* tooltip) const {
  if (!toggled_ || toggled_tooltip_text_.empty())
    return Button::GetTooltipText(p, tooltip);

  *tooltip = toggled_tooltip_text_;
  return true;
}

void ToggleImageButton::GetAccessibleState(ui::AXViewState* state) {
  ImageButton::GetAccessibleState(state);
  GetTooltipText(gfx::Point(), &state->name);
}

}  // namespace views
