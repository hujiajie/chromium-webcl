// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/app_list_view.h"

#include <stddef.h>

#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/app_list/app_list_constants.h"
#include "ui/app_list/app_list_switches.h"
#include "ui/app_list/pagination_model.h"
#include "ui/app_list/search_box_model.h"
#include "ui/app_list/test/app_list_test_model.h"
#include "ui/app_list/test/app_list_test_view_delegate.h"
#include "ui/app_list/test/test_search_result.h"
#include "ui/app_list/views/app_list_folder_view.h"
#include "ui/app_list/views/app_list_main_view.h"
#include "ui/app_list/views/apps_container_view.h"
#include "ui/app_list/views/apps_grid_view.h"
#include "ui/app_list/views/contents_view.h"
#include "ui/app_list/views/search_box_view.h"
#include "ui/app_list/views/search_result_page_view.h"
#include "ui/app_list/views/search_result_tile_item_view.h"
#include "ui/app_list/views/start_page_view.h"
#include "ui/app_list/views/test/apps_grid_view_test_api.h"
#include "ui/app_list/views/tile_item_view.h"
#include "ui/events/event_utils.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/test/test_views_delegate.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/views_delegate.h"
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"

namespace app_list {
namespace test {

namespace {

enum TestType {
  TEST_TYPE_START = 0,
  NORMAL = TEST_TYPE_START,
  LANDSCAPE,
  EXPERIMENTAL,
  TEST_TYPE_END,
};

template <class T>
size_t GetVisibleViews(const std::vector<T*>& tiles) {
  size_t count = 0;
  for (const auto& tile : tiles) {
    if (tile->visible())
      count++;
  }
  return count;
}

void SimulateClick(views::View* view) {
  gfx::Point center = view->GetLocalBounds().CenterPoint();
  view->OnMousePressed(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, center, center, ui::EventTimeForNow(),
      ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  view->OnMouseReleased(ui::MouseEvent(
      ui::ET_MOUSE_RELEASED, center, center, ui::EventTimeForNow(),
      ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
}

// Choose a set that is 3 regular app list pages and 2 landscape app list pages.
const int kInitialItems = 34;

class TestStartPageSearchResult : public TestSearchResult {
 public:
  TestStartPageSearchResult() { set_display_type(DISPLAY_RECOMMENDATION); }
  ~TestStartPageSearchResult() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TestStartPageSearchResult);
};

// Allows the same tests to run with different contexts: either an Ash-style
// root window or a desktop window tree host.
class AppListViewTestContext {
 public:
  AppListViewTestContext(int test_type, gfx::NativeView parent);
  ~AppListViewTestContext();

  // Test displaying the app list and performs a standard set of checks on its
  // top level views. Then closes the window.
  void RunDisplayTest();

  // Hides and reshows the app list with a folder open, expecting the main grid
  // view to be shown.
  void RunReshowWithOpenFolderTest();

  // Tests that pressing the search box's back button navigates correctly.
  void RunBackTest();

  // Tests displaying of the experimental app list and shows the start page.
  void RunStartPageTest();

  // Tests switching rapidly between multiple pages of the launcher.
  void RunPageSwitchingAnimationTest();

  // Tests changing the App List profile.
  void RunProfileChangeTest();

  // Tests displaying of the search results.
  void RunSearchResultsTest();

  // Tests displaying the app list overlay.
  void RunAppListOverlayTest();

  // A standard set of checks on a view, e.g., ensuring it is drawn and visible.
  static void CheckView(views::View* subview);

  // Invoked when the Widget is closing, and the view it contains is about to
  // be torn down. This only occurs in a run loop and will be used as a signal
  // to quit.
  void NativeWidgetClosing() {
    view_ = NULL;
    run_loop_->Quit();
  }

  // Whether the experimental "landscape" app launcher UI is being tested.
  bool is_landscape() const {
    return test_type_ == LANDSCAPE || test_type_ == EXPERIMENTAL;
  }

 private:
  // Switches the launcher to |state| and lays out to ensure all launcher pages
  // are in the correct position. Checks that the state is where it should be
  // and returns false on failure.
  bool SetAppListState(AppListModel::State state);

  // Returns true if all of the pages are in their correct position for |state|.
  bool IsStateShown(AppListModel::State state);

  // Shows the app list and waits until a paint occurs.
  void Show();

  // Closes the app list. This sets |view_| to NULL.
  void Close();

  // Checks the search box widget is at |expected| in the contents view's
  // coordinate space.
  bool CheckSearchBoxWidget(const gfx::Rect& expected);

  // Gets the PaginationModel owned by |view_|.
  PaginationModel* GetPaginationModel();

  const TestType test_type_;
  scoped_ptr<base::RunLoop> run_loop_;
  app_list::AppListView* view_;  // Owned by native widget.
  scoped_ptr<app_list::test::AppListTestViewDelegate> delegate_;

  DISALLOW_COPY_AND_ASSIGN(AppListViewTestContext);
};

// Extend the regular AppListTestViewDelegate to communicate back to the test
// context. Note the test context doesn't simply inherit this, because the
// delegate is owned by the view.
class UnitTestViewDelegate : public app_list::test::AppListTestViewDelegate {
 public:
  explicit UnitTestViewDelegate(AppListViewTestContext* parent)
      : parent_(parent) {}

  // Overridden from app_list::AppListViewDelegate:
  bool ShouldCenterWindow() const override {
    return app_list::switches::IsCenteredAppListEnabled();
  }

  // Overridden from app_list::test::AppListTestViewDelegate:
  void ViewClosing() override { parent_->NativeWidgetClosing(); }

 private:
  AppListViewTestContext* parent_;

  DISALLOW_COPY_AND_ASSIGN(UnitTestViewDelegate);
};

AppListViewTestContext::AppListViewTestContext(int test_type,
                                               gfx::NativeView parent)
    : test_type_(static_cast<TestType>(test_type)) {
  switch (test_type_) {
    case NORMAL:
      base::CommandLine::ForCurrentProcess()->AppendSwitch(
          switches::kDisableExperimentalAppList);
      break;
    case LANDSCAPE:
      base::CommandLine::ForCurrentProcess()->AppendSwitch(
          switches::kDisableExperimentalAppList);
      base::CommandLine::ForCurrentProcess()->AppendSwitch(
          switches::kEnableCenteredAppList);
      break;
    case EXPERIMENTAL:
      base::CommandLine::ForCurrentProcess()->AppendSwitch(
          switches::kEnableExperimentalAppList);
      break;
    default:
      NOTREACHED();
      break;
  }

  delegate_.reset(new UnitTestViewDelegate(this));
  view_ = new app_list::AppListView(delegate_.get());

  // Initialize centered around a point that ensures the window is wholly shown.
  view_->InitAsBubbleAtFixedLocation(parent,
                                     0,
                                     gfx::Point(300, 300),
                                     views::BubbleBorder::FLOAT,
                                     false /* border_accepts_events */);
}

AppListViewTestContext::~AppListViewTestContext() {
  // The view observes the PaginationModel which is about to get destroyed, so
  // if the view is not already deleted by the time this destructor is called,
  // there will be problems.
  EXPECT_FALSE(view_);
}

// static
void AppListViewTestContext::CheckView(views::View* subview) {
  ASSERT_TRUE(subview);
  EXPECT_TRUE(subview->parent());
  EXPECT_TRUE(subview->visible());
  EXPECT_TRUE(subview->IsDrawn());
  EXPECT_FALSE(subview->bounds().IsEmpty());
}

bool AppListViewTestContext::SetAppListState(AppListModel::State state) {
  ContentsView* contents_view = view_->app_list_main_view()->contents_view();
  contents_view->SetActiveState(state);
  contents_view->Layout();
  return IsStateShown(state);
}

bool AppListViewTestContext::IsStateShown(AppListModel::State state) {
  ContentsView* contents_view = view_->app_list_main_view()->contents_view();
  bool success = true;
  for (int i = 0; i < contents_view->NumLauncherPages(); ++i) {
    success = success &&
              (contents_view->GetPageView(i)->GetPageBoundsForState(state) ==
               contents_view->GetPageView(i)->bounds());
  }
  return success && state == delegate_->GetTestModel()->state();
}

void AppListViewTestContext::Show() {
  view_->GetWidget()->Show();
  run_loop_.reset(new base::RunLoop);
  view_->SetNextPaintCallback(run_loop_->QuitClosure());
  run_loop_->Run();

  EXPECT_TRUE(view_->GetWidget()->IsVisible());
}

void AppListViewTestContext::Close() {
  view_->GetWidget()->Close();
  run_loop_.reset(new base::RunLoop);
  run_loop_->Run();

  // |view_| should have been deleted and set to NULL via ViewClosing().
  EXPECT_FALSE(view_);
}

bool AppListViewTestContext::CheckSearchBoxWidget(const gfx::Rect& expected) {
  ContentsView* contents_view = view_->app_list_main_view()->contents_view();
  // Adjust for the search box view's shadow.
  gfx::Rect expected_with_shadow =
      view_->app_list_main_view()
          ->search_box_view()
          ->GetViewBoundsForSearchBoxContentsBounds(expected);
  gfx::Point point = expected_with_shadow.origin();
  views::View::ConvertPointToScreen(contents_view, &point);

  return gfx::Rect(point, expected_with_shadow.size()) ==
         view_->search_box_widget()->GetWindowBoundsInScreen();
}

PaginationModel* AppListViewTestContext::GetPaginationModel() {
  return view_->GetAppsPaginationModel();
}

void AppListViewTestContext::RunDisplayTest() {
  EXPECT_FALSE(view_->GetWidget()->IsVisible());
  EXPECT_EQ(-1, GetPaginationModel()->total_pages());
  delegate_->GetTestModel()->PopulateApps(kInitialItems);

  Show();

#if defined(OS_CHROMEOS)
  // Explicitly enforce the exact dimensions of the app list. Feel free to
  // change these if you need to (they are just here to prevent against
  // accidental changes to the window size).
  //
  // Note: Only test this on Chrome OS; the deprecation banner on other
  // platforms makes the height variable so we can't reliably test it (nor do we
  // really need to).
  switch (test_type_) {
    case NORMAL:
      EXPECT_EQ("400x500", view_->bounds().size().ToString());
      break;
    case LANDSCAPE:
      // NOTE: Height should not exceed 402, because otherwise there might not
      // be enough space to accomodate the virtual keyboard. (LANDSCAPE mode is
      // enabled by default when the virtual keyboard is enabled.)
      EXPECT_EQ("576x402", view_->bounds().size().ToString());
      break;
    case EXPERIMENTAL:
      EXPECT_EQ("768x570", view_->bounds().size().ToString());
      break;
    default:
      NOTREACHED();
      break;
  }
#endif  // defined(OS_CHROMEOS)

  if (is_landscape())
    EXPECT_EQ(2, GetPaginationModel()->total_pages());
  else
    EXPECT_EQ(3, GetPaginationModel()->total_pages());
  EXPECT_EQ(0, GetPaginationModel()->selected_page());

  // Checks on the main view.
  AppListMainView* main_view = view_->app_list_main_view();
  EXPECT_NO_FATAL_FAILURE(CheckView(main_view));
  EXPECT_NO_FATAL_FAILURE(CheckView(main_view->contents_view()));

  AppListModel::State expected = test_type_ == EXPERIMENTAL
                                     ? AppListModel::STATE_START
                                     : AppListModel::STATE_APPS;
  EXPECT_TRUE(main_view->contents_view()->IsStateActive(expected));
  EXPECT_EQ(expected, delegate_->GetTestModel()->state());

  Close();
}

void AppListViewTestContext::RunReshowWithOpenFolderTest() {
  EXPECT_FALSE(view_->GetWidget()->IsVisible());
  EXPECT_EQ(-1, GetPaginationModel()->total_pages());

  AppListTestModel* model = delegate_->GetTestModel();
  model->PopulateApps(kInitialItems);
  const std::string folder_id =
      model->MergeItems(model->top_level_item_list()->item_at(0)->id(),
                        model->top_level_item_list()->item_at(1)->id());

  AppListFolderItem* folder_item = model->FindFolderItem(folder_id);
  EXPECT_TRUE(folder_item);

  Show();

  // The main grid view should be showing initially.
  AppListMainView* main_view = view_->app_list_main_view();
  AppsContainerView* container_view =
      main_view->contents_view()->apps_container_view();
  EXPECT_NO_FATAL_FAILURE(CheckView(main_view));
  EXPECT_NO_FATAL_FAILURE(CheckView(container_view->apps_grid_view()));
  EXPECT_FALSE(container_view->app_list_folder_view()->visible());

  AppsGridViewTestApi test_api(container_view->apps_grid_view());
  test_api.PressItemAt(0);

  // After pressing the folder item, the folder view should be showing.
  EXPECT_NO_FATAL_FAILURE(CheckView(main_view));
  EXPECT_NO_FATAL_FAILURE(CheckView(container_view->app_list_folder_view()));
  EXPECT_FALSE(container_view->apps_grid_view()->visible());

  view_->GetWidget()->Hide();
  EXPECT_FALSE(view_->GetWidget()->IsVisible());

  Show();

  // The main grid view should be showing after a reshow.
  EXPECT_NO_FATAL_FAILURE(CheckView(main_view));
  EXPECT_NO_FATAL_FAILURE(CheckView(container_view->apps_grid_view()));
  EXPECT_FALSE(container_view->app_list_folder_view()->visible());

  Close();
}

void AppListViewTestContext::RunBackTest() {
  if (test_type_ != EXPERIMENTAL) {
    Close();
    return;
  }

  EXPECT_FALSE(view_->GetWidget()->IsVisible());
  EXPECT_EQ(-1, GetPaginationModel()->total_pages());

  Show();

  AppListMainView* main_view = view_->app_list_main_view();
  ContentsView* contents_view = main_view->contents_view();
  SearchBoxView* search_box_view = main_view->search_box_view();

  // Show the apps grid.
  SetAppListState(AppListModel::STATE_APPS);
  EXPECT_NO_FATAL_FAILURE(CheckView(search_box_view->back_button()));

  // The back button should return to the start page.
  EXPECT_TRUE(contents_view->Back());
  contents_view->Layout();
  EXPECT_TRUE(IsStateShown(AppListModel::STATE_START));
  EXPECT_FALSE(search_box_view->back_button()->visible());

  // Show the apps grid again.
  SetAppListState(AppListModel::STATE_APPS);
  EXPECT_NO_FATAL_FAILURE(CheckView(search_box_view->back_button()));

  // Pressing ESC should return to the start page.
  view_->AcceleratorPressed(ui::Accelerator(ui::VKEY_ESCAPE, ui::EF_NONE));
  contents_view->Layout();
  EXPECT_TRUE(IsStateShown(AppListModel::STATE_START));
  EXPECT_FALSE(search_box_view->back_button()->visible());

  // Pressing ESC from the start page should close the app list.
  EXPECT_EQ(0, delegate_->dismiss_count());
  view_->AcceleratorPressed(ui::Accelerator(ui::VKEY_ESCAPE, ui::EF_NONE));
  EXPECT_EQ(1, delegate_->dismiss_count());

  // Show the search results.
  base::string16 new_search_text = base::UTF8ToUTF16("apple");
  search_box_view->search_box()->SetText(base::string16());
  search_box_view->search_box()->InsertText(new_search_text);
  contents_view->Layout();
  EXPECT_TRUE(IsStateShown(AppListModel::STATE_SEARCH_RESULTS));
  EXPECT_NO_FATAL_FAILURE(CheckView(search_box_view->back_button()));

  // The back button should return to the start page.
  EXPECT_TRUE(contents_view->Back());
  contents_view->Layout();
  EXPECT_TRUE(IsStateShown(AppListModel::STATE_START));
  EXPECT_FALSE(search_box_view->back_button()->visible());

  Close();
}

void AppListViewTestContext::RunStartPageTest() {
  EXPECT_FALSE(view_->GetWidget()->IsVisible());
  EXPECT_EQ(-1, GetPaginationModel()->total_pages());
  AppListTestModel* model = delegate_->GetTestModel();
  model->PopulateApps(3);

  Show();

  AppListMainView* main_view = view_->app_list_main_view();
  StartPageView* start_page_view =
      main_view->contents_view()->start_page_view();
  // Checks on the main view.
  EXPECT_NO_FATAL_FAILURE(CheckView(main_view));
  EXPECT_NO_FATAL_FAILURE(CheckView(main_view->contents_view()));
  if (test_type_ == EXPERIMENTAL) {
    EXPECT_NO_FATAL_FAILURE(CheckView(start_page_view));

    // Show the start page view.
    EXPECT_TRUE(SetAppListState(AppListModel::STATE_START));
    gfx::Size view_size(view_->GetPreferredSize());

    // The "All apps" button should have its "parent background color" set
    // to the tiles container's background color.
    TileItemView* all_apps_button = start_page_view->all_apps_button();
    EXPECT_TRUE(all_apps_button->visible());
    EXPECT_EQ(kLabelBackgroundColor,
              all_apps_button->parent_background_color());

    // Simulate clicking the "All apps" button. Check that we navigate to the
    // apps grid view.
    SimulateClick(all_apps_button);
    main_view->contents_view()->Layout();
    EXPECT_TRUE(IsStateShown(AppListModel::STATE_APPS));

    // Hiding and showing the search box should not affect the app list's
    // preferred size. This is a regression test for http://crbug.com/386912.
    EXPECT_EQ(view_size.ToString(), view_->GetPreferredSize().ToString());

    // Check tiles hide and show on deletion and addition.
    EXPECT_TRUE(SetAppListState(AppListModel::STATE_START));
    model->results()->Add(new TestStartPageSearchResult());
    start_page_view->UpdateForTesting();
    EXPECT_EQ(1u, GetVisibleViews(start_page_view->tile_views()));
    model->results()->DeleteAll();
    start_page_view->UpdateForTesting();
    EXPECT_EQ(0u, GetVisibleViews(start_page_view->tile_views()));

    // Tiles should not update when the start page is not active but should be
    // correct once the start page is shown.
    EXPECT_TRUE(SetAppListState(AppListModel::STATE_APPS));
    model->results()->Add(new TestStartPageSearchResult());
    start_page_view->UpdateForTesting();
    EXPECT_EQ(0u, GetVisibleViews(start_page_view->tile_views()));
    EXPECT_TRUE(SetAppListState(AppListModel::STATE_START));
    EXPECT_EQ(1u, GetVisibleViews(start_page_view->tile_views()));
  } else {
    EXPECT_EQ(NULL, start_page_view);
  }

  Close();
}

void AppListViewTestContext::RunPageSwitchingAnimationTest() {
  if (test_type_ == EXPERIMENTAL) {
    Show();

    AppListMainView* main_view = view_->app_list_main_view();
    // Checks on the main view.
    EXPECT_NO_FATAL_FAILURE(CheckView(main_view));
    EXPECT_NO_FATAL_FAILURE(CheckView(main_view->contents_view()));

    ContentsView* contents_view = main_view->contents_view();

    contents_view->SetActiveState(AppListModel::STATE_START);
    contents_view->Layout();

    IsStateShown(AppListModel::STATE_START);

    // Change pages. View should not have moved without Layout().
    contents_view->SetActiveState(AppListModel::STATE_SEARCH_RESULTS);
    IsStateShown(AppListModel::STATE_START);

    // Change to a third page. This queues up the second animation behind the
    // first.
    contents_view->SetActiveState(AppListModel::STATE_APPS);
    IsStateShown(AppListModel::STATE_START);

    // Call Layout(). Should jump to the third page.
    contents_view->Layout();
    IsStateShown(AppListModel::STATE_APPS);
  }

  Close();
}

void AppListViewTestContext::RunProfileChangeTest() {
  EXPECT_FALSE(view_->GetWidget()->IsVisible());
  EXPECT_EQ(-1, GetPaginationModel()->total_pages());
  delegate_->GetTestModel()->PopulateApps(kInitialItems);

  Show();

  if (is_landscape())
    EXPECT_EQ(2, GetPaginationModel()->total_pages());
  else
    EXPECT_EQ(3, GetPaginationModel()->total_pages());

  // Change the profile. The original model needs to be kept alive for
  // observers to unregister themselves.
  scoped_ptr<AppListTestModel> original_test_model(
      delegate_->ReleaseTestModel());
  delegate_->set_next_profile_app_count(1);

  // The original ContentsView is destroyed here.
  view_->SetProfileByPath(base::FilePath());
  EXPECT_EQ(1, GetPaginationModel()->total_pages());

  StartPageView* start_page_view =
      view_->app_list_main_view()->contents_view()->start_page_view();
  if (test_type_ == EXPERIMENTAL)
    EXPECT_NO_FATAL_FAILURE(CheckView(start_page_view));
  else
    EXPECT_EQ(NULL, start_page_view);

  // New model updates should be processed by the start page view.
  delegate_->GetTestModel()->results()->Add(new TestStartPageSearchResult());
  if (test_type_ == EXPERIMENTAL) {
    start_page_view->UpdateForTesting();
    EXPECT_EQ(1u, GetVisibleViews(start_page_view->tile_views()));
  }

  // Old model updates should be ignored.
  original_test_model->results()->Add(new TestStartPageSearchResult());
  original_test_model->results()->Add(new TestStartPageSearchResult());
  if (test_type_ == EXPERIMENTAL) {
    start_page_view->UpdateForTesting();
    EXPECT_EQ(1u, GetVisibleViews(start_page_view->tile_views()));
  }

  Close();
}

void AppListViewTestContext::RunSearchResultsTest() {
  EXPECT_FALSE(view_->GetWidget()->IsVisible());
  EXPECT_EQ(-1, GetPaginationModel()->total_pages());
  AppListTestModel* model = delegate_->GetTestModel();
  model->PopulateApps(3);

  Show();

  AppListMainView* main_view = view_->app_list_main_view();
  ContentsView* contents_view = main_view->contents_view();
  EXPECT_TRUE(SetAppListState(AppListModel::STATE_APPS));

  // Show the search results.
  contents_view->ShowSearchResults(true);
  contents_view->Layout();
  EXPECT_TRUE(contents_view->IsStateActive(AppListModel::STATE_SEARCH_RESULTS));

  EXPECT_TRUE(IsStateShown(AppListModel::STATE_SEARCH_RESULTS));

  // Hide the search results.
  contents_view->ShowSearchResults(false);
  contents_view->Layout();

  // Check that we return to the page that we were on before the search.
  EXPECT_TRUE(IsStateShown(AppListModel::STATE_APPS));

  if (test_type_ == EXPERIMENTAL) {
    // Check that typing into the search box triggers the search page.
    EXPECT_TRUE(SetAppListState(AppListModel::STATE_START));
    view_->Layout();
    EXPECT_TRUE(IsStateShown(AppListModel::STATE_START));

    base::string16 search_text = base::UTF8ToUTF16("test");
    main_view->search_box_view()->search_box()->SetText(base::string16());
    main_view->search_box_view()->search_box()->InsertText(search_text);
    // Check that the current search is using |search_text|.
    EXPECT_EQ(search_text, delegate_->GetTestModel()->search_box()->text());
    EXPECT_EQ(search_text, main_view->search_box_view()->search_box()->text());
    contents_view->Layout();
    EXPECT_TRUE(
        contents_view->IsStateActive(AppListModel::STATE_SEARCH_RESULTS));
    EXPECT_TRUE(
        CheckSearchBoxWidget(contents_view->GetDefaultSearchBoxBounds()));

    // Check that typing into the search box triggers the search page.
    EXPECT_TRUE(SetAppListState(AppListModel::STATE_APPS));
    contents_view->Layout();
    EXPECT_TRUE(IsStateShown(AppListModel::STATE_APPS));
    EXPECT_TRUE(
        CheckSearchBoxWidget(contents_view->GetDefaultSearchBoxBounds()));

    base::string16 new_search_text = base::UTF8ToUTF16("apple");
    main_view->search_box_view()->search_box()->SetText(base::string16());
    main_view->search_box_view()->search_box()->InsertText(new_search_text);
    // Check that the current search is using |new_search_text|.
    EXPECT_EQ(new_search_text, delegate_->GetTestModel()->search_box()->text());
    EXPECT_EQ(new_search_text,
              main_view->search_box_view()->search_box()->text());
    contents_view->Layout();
    EXPECT_TRUE(IsStateShown(AppListModel::STATE_SEARCH_RESULTS));
    EXPECT_TRUE(
        CheckSearchBoxWidget(contents_view->GetDefaultSearchBoxBounds()));
  }

  Close();
}

void AppListViewTestContext::RunAppListOverlayTest() {
  Show();

  AppListMainView* main_view = view_->app_list_main_view();
  SearchBoxView* search_box_view = main_view->search_box_view();

  // The search box should not be enabled when the app list overlay is shown.
  view_->SetAppListOverlayVisible(true);
  EXPECT_FALSE(search_box_view->enabled());

  // The search box should be refocused when the app list overlay is hidden.
  view_->SetAppListOverlayVisible(false);
  EXPECT_TRUE(search_box_view->enabled());
  EXPECT_EQ(search_box_view->search_box(),
            view_->GetWidget()->GetFocusManager()->GetFocusedView());

  Close();
}

class AppListViewTestAura : public views::ViewsTestBase,
                            public ::testing::WithParamInterface<int> {
 public:
  AppListViewTestAura() {}
  virtual ~AppListViewTestAura() {}

  // testing::Test overrides:
  void SetUp() override {
    views::ViewsTestBase::SetUp();

    // On Ash (only) the app list is placed into an aura::Window "container",
    // which is also used to determine the context. In tests, use the ash root
    // window as the parent. This only works on aura where the root window is a
    // NativeView as well as a NativeWindow.
    gfx::NativeView container = NULL;
#if defined(USE_AURA)
    container = GetContext();
#endif

    test_context_.reset(new AppListViewTestContext(GetParam(), container));
  }

  void TearDown() override {
    test_context_.reset();
    views::ViewsTestBase::TearDown();
  }

 protected:
  scoped_ptr<AppListViewTestContext> test_context_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AppListViewTestAura);
};

class AppListViewTestDesktop : public views::ViewsTestBase,
                               public ::testing::WithParamInterface<int> {
 public:
  AppListViewTestDesktop() {}
  virtual ~AppListViewTestDesktop() {}

  // testing::Test overrides:
  void SetUp() override {
    set_views_delegate(make_scoped_ptr(new AppListViewTestViewsDelegate(this)));
    views::ViewsTestBase::SetUp();
    test_context_.reset(new AppListViewTestContext(GetParam(), NULL));
  }

  void TearDown() override {
    test_context_.reset();
    views::ViewsTestBase::TearDown();
  }

 protected:
  scoped_ptr<AppListViewTestContext> test_context_;

 private:
  class AppListViewTestViewsDelegate : public views::TestViewsDelegate {
   public:
    explicit AppListViewTestViewsDelegate(AppListViewTestDesktop* parent)
#if defined(OS_CHROMEOS)
        : parent_(parent)
#endif
    {
    }

    // Overridden from views::ViewsDelegate:
    void OnBeforeWidgetInit(
        views::Widget::InitParams* params,
        views::internal::NativeWidgetDelegate* delegate) override;

   private:
#if defined(OS_CHROMEOS)
    AppListViewTestDesktop* parent_;
#endif

    DISALLOW_COPY_AND_ASSIGN(AppListViewTestViewsDelegate);
  };

  DISALLOW_COPY_AND_ASSIGN(AppListViewTestDesktop);
};

void AppListViewTestDesktop::AppListViewTestViewsDelegate::OnBeforeWidgetInit(
    views::Widget::InitParams* params,
    views::internal::NativeWidgetDelegate* delegate) {
// Mimic the logic in ChromeViewsDelegate::OnBeforeWidgetInit(). Except, for
// ChromeOS, use the root window from the AuraTestHelper rather than depending
// on ash::Shell:GetPrimaryRootWindow(). Also assume non-ChromeOS is never the
// Ash desktop, as that is covered by AppListViewTestAura.
#if defined(OS_CHROMEOS)
  if (!params->parent && !params->context)
    params->context = parent_->GetContext();
#elif defined(USE_AURA)
  if (params->parent == NULL && params->context == NULL && !params->child)
    params->native_widget = new views::DesktopNativeWidgetAura(delegate);
#endif
}

}  // namespace

// Tests showing the app list with basic test model in an ash-style root window.
TEST_P(AppListViewTestAura, Display) {
  EXPECT_NO_FATAL_FAILURE(test_context_->RunDisplayTest());
}

// Tests showing the app list on the desktop. Note on ChromeOS, this will still
// use the regular root window.
TEST_P(AppListViewTestDesktop, Display) {
  EXPECT_NO_FATAL_FAILURE(test_context_->RunDisplayTest());
}

// Tests that the main grid view is shown after hiding and reshowing the app
// list with a folder view open. This is a regression test for crbug.com/357058.
TEST_P(AppListViewTestAura, ReshowWithOpenFolder) {
  EXPECT_NO_FATAL_FAILURE(test_context_->RunReshowWithOpenFolderTest());
}

TEST_P(AppListViewTestDesktop, ReshowWithOpenFolder) {
  EXPECT_NO_FATAL_FAILURE(test_context_->RunReshowWithOpenFolderTest());
}

// Tests that the start page view operates correctly.
TEST_P(AppListViewTestAura, StartPageTest) {
  EXPECT_NO_FATAL_FAILURE(test_context_->RunStartPageTest());
}

TEST_P(AppListViewTestDesktop, StartPageTest) {
  EXPECT_NO_FATAL_FAILURE(test_context_->RunStartPageTest());
}

// Tests that the start page view operates correctly.
TEST_P(AppListViewTestAura, PageSwitchingAnimationTest) {
  EXPECT_NO_FATAL_FAILURE(test_context_->RunPageSwitchingAnimationTest());
}

TEST_P(AppListViewTestDesktop, PageSwitchingAnimationTest) {
  EXPECT_NO_FATAL_FAILURE(test_context_->RunPageSwitchingAnimationTest());
}

// Tests that the profile changes operate correctly.
TEST_P(AppListViewTestAura, ProfileChangeTest) {
  EXPECT_NO_FATAL_FAILURE(test_context_->RunProfileChangeTest());
}

TEST_P(AppListViewTestDesktop, ProfileChangeTest) {
  EXPECT_NO_FATAL_FAILURE(test_context_->RunProfileChangeTest());
}

// Tests that the correct views are displayed for showing search results.
TEST_P(AppListViewTestAura, SearchResultsTest) {
  EXPECT_NO_FATAL_FAILURE(test_context_->RunSearchResultsTest());
}

TEST_P(AppListViewTestDesktop, SearchResultsTest) {
  EXPECT_NO_FATAL_FAILURE(test_context_->RunSearchResultsTest());
}

// Tests that the back button navigates through the app list correctly.
TEST_P(AppListViewTestAura, BackTest) {
  EXPECT_NO_FATAL_FAILURE(test_context_->RunBackTest());
}

TEST_P(AppListViewTestDesktop, BackTest) {
  EXPECT_NO_FATAL_FAILURE(test_context_->RunBackTest());
}

// Tests that the correct views are displayed for showing search results.
TEST_P(AppListViewTestAura, AppListOverlayTest) {
  EXPECT_NO_FATAL_FAILURE(test_context_->RunAppListOverlayTest());
}

TEST_P(AppListViewTestDesktop, AppListOverlayTest) {
  EXPECT_NO_FATAL_FAILURE(test_context_->RunAppListOverlayTest());
}

#if defined(USE_AURA)
INSTANTIATE_TEST_CASE_P(AppListViewTestAuraInstance,
                        AppListViewTestAura,
                        ::testing::Range<int>(TEST_TYPE_START, TEST_TYPE_END));
#endif

INSTANTIATE_TEST_CASE_P(AppListViewTestDesktopInstance,
                        AppListViewTestDesktop,
                        ::testing::Range<int>(TEST_TYPE_START, TEST_TYPE_END));

}  // namespace test
}  // namespace app_list
