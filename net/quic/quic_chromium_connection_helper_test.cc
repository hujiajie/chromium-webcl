// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/quic_chromium_connection_helper.h"

#include "net/quic/test_tools/mock_clock.h"
#include "net/quic/test_tools/mock_random.h"
#include "net/quic/test_tools/test_task_runner.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace test {
namespace {

class TestDelegate : public QuicAlarm::Delegate {
 public:
  TestDelegate() : fired_(false) {}

  void OnAlarm() override { fired_ = true; }

  bool fired() const { return fired_; }
  void Clear() { fired_ = false; }

 private:
  bool fired_;
};

class QuicChromiumConnectionHelperTest : public ::testing::Test {
 protected:
  QuicChromiumConnectionHelperTest()
      : runner_(new TestTaskRunner(&clock_)),
        helper_(runner_.get(), &clock_, &random_generator_) {}

  scoped_refptr<TestTaskRunner> runner_;
  QuicChromiumConnectionHelper helper_;
  MockClock clock_;
  MockRandom random_generator_;
};

TEST_F(QuicChromiumConnectionHelperTest, GetClock) {
  EXPECT_EQ(&clock_, helper_.GetClock());
}

TEST_F(QuicChromiumConnectionHelperTest, GetRandomGenerator) {
  EXPECT_EQ(&random_generator_, helper_.GetRandomGenerator());
}

TEST_F(QuicChromiumConnectionHelperTest, CreateAlarm) {
  TestDelegate* delegate = new TestDelegate();
  scoped_ptr<QuicAlarm> alarm(helper_.CreateAlarm(delegate));

  QuicTime::Delta delta = QuicTime::Delta::FromMicroseconds(1);
  alarm->Set(clock_.Now().Add(delta));

  // Verify that the alarm task has been posted.
  ASSERT_EQ(1u, runner_->GetPostedTasks().size());
  EXPECT_EQ(base::TimeDelta::FromMicroseconds(delta.ToMicroseconds()),
            runner_->GetPostedTasks()[0].delay);

  runner_->RunNextTask();
  EXPECT_EQ(QuicTime::Zero().Add(delta), clock_.Now());
  EXPECT_TRUE(delegate->fired());
}

TEST_F(QuicChromiumConnectionHelperTest, CreateAlarmAndCancel) {
  TestDelegate* delegate = new TestDelegate();
  scoped_ptr<QuicAlarm> alarm(helper_.CreateAlarm(delegate));

  QuicTime::Delta delta = QuicTime::Delta::FromMicroseconds(1);
  alarm->Set(clock_.Now().Add(delta));
  alarm->Cancel();

  // The alarm task should still be posted.
  ASSERT_EQ(1u, runner_->GetPostedTasks().size());
  EXPECT_EQ(base::TimeDelta::FromMicroseconds(delta.ToMicroseconds()),
            runner_->GetPostedTasks()[0].delay);

  runner_->RunNextTask();
  EXPECT_EQ(QuicTime::Zero().Add(delta), clock_.Now());
  EXPECT_FALSE(delegate->fired());
}

TEST_F(QuicChromiumConnectionHelperTest, CreateAlarmAndReset) {
  TestDelegate* delegate = new TestDelegate();
  scoped_ptr<QuicAlarm> alarm(helper_.CreateAlarm(delegate));

  QuicTime::Delta delta = QuicTime::Delta::FromMicroseconds(1);
  alarm->Set(clock_.Now().Add(delta));
  alarm->Cancel();
  QuicTime::Delta new_delta = QuicTime::Delta::FromMicroseconds(3);
  alarm->Set(clock_.Now().Add(new_delta));

  // The alarm task should still be posted.
  ASSERT_EQ(1u, runner_->GetPostedTasks().size());
  EXPECT_EQ(base::TimeDelta::FromMicroseconds(delta.ToMicroseconds()),
            runner_->GetPostedTasks()[0].delay);

  runner_->RunNextTask();
  EXPECT_EQ(QuicTime::Zero().Add(delta), clock_.Now());
  EXPECT_FALSE(delegate->fired());

  // The alarm task should be posted again.
  ASSERT_EQ(1u, runner_->GetPostedTasks().size());

  runner_->RunNextTask();
  EXPECT_EQ(QuicTime::Zero().Add(new_delta), clock_.Now());
  EXPECT_TRUE(delegate->fired());
}

TEST_F(QuicChromiumConnectionHelperTest, CreateAlarmAndResetEarlier) {
  TestDelegate* delegate = new TestDelegate();
  scoped_ptr<QuicAlarm> alarm(helper_.CreateAlarm(delegate));

  QuicTime::Delta delta = QuicTime::Delta::FromMicroseconds(3);
  alarm->Set(clock_.Now().Add(delta));
  alarm->Cancel();
  QuicTime::Delta new_delta = QuicTime::Delta::FromMicroseconds(1);
  alarm->Set(clock_.Now().Add(new_delta));

  // Both alarm tasks will be posted.
  ASSERT_EQ(2u, runner_->GetPostedTasks().size());

  // The earlier task will execute and will fire the alarm->
  runner_->RunNextTask();
  EXPECT_EQ(QuicTime::Zero().Add(new_delta), clock_.Now());
  EXPECT_TRUE(delegate->fired());
  delegate->Clear();

  // The latter task is still posted.
  ASSERT_EQ(1u, runner_->GetPostedTasks().size());

  // When the latter task is executed, the weak ptr will be invalid and
  // the alarm will not fire.
  runner_->RunNextTask();
  EXPECT_EQ(QuicTime::Zero().Add(delta), clock_.Now());
  EXPECT_FALSE(delegate->fired());
}

TEST_F(QuicChromiumConnectionHelperTest, CreateAlarmAndUpdate) {
  TestDelegate* delegate = new TestDelegate();
  scoped_ptr<QuicAlarm> alarm(helper_.CreateAlarm(delegate));

  const QuicClock* clock = helper_.GetClock();
  QuicTime start = clock->Now();
  QuicTime::Delta delta = QuicTime::Delta::FromMicroseconds(1);
  alarm->Set(clock->Now().Add(delta));
  QuicTime::Delta new_delta = QuicTime::Delta::FromMicroseconds(3);
  alarm->Update(clock->Now().Add(new_delta),
                QuicTime::Delta::FromMicroseconds(1));

  // The alarm task should still be posted.
  ASSERT_EQ(1u, runner_->GetPostedTasks().size());
  EXPECT_EQ(base::TimeDelta::FromMicroseconds(delta.ToMicroseconds()),
            runner_->GetPostedTasks()[0].delay);

  runner_->RunNextTask();
  EXPECT_EQ(QuicTime::Zero().Add(delta), clock->Now());
  EXPECT_FALSE(delegate->fired());

  // Move the alarm forward 1us and ensure it doesn't move forward.
  alarm->Update(clock->Now().Add(new_delta),
                QuicTime::Delta::FromMicroseconds(2));

  ASSERT_EQ(1u, runner_->GetPostedTasks().size());
  EXPECT_EQ(base::TimeDelta::FromMicroseconds(
                new_delta.Subtract(delta).ToMicroseconds()),
            runner_->GetPostedTasks()[0].delay);
  runner_->RunNextTask();
  EXPECT_EQ(start.Add(new_delta), clock->Now());
  EXPECT_TRUE(delegate->fired());

  // Set the alarm via an update call.
  new_delta = QuicTime::Delta::FromMicroseconds(5);
  alarm->Update(clock->Now().Add(new_delta),
                QuicTime::Delta::FromMicroseconds(1));
  EXPECT_TRUE(alarm->IsSet());

  // Update it with an uninitialized time and ensure it's cancelled.
  alarm->Update(QuicTime::Zero(), QuicTime::Delta::FromMicroseconds(1));
  EXPECT_FALSE(alarm->IsSet());
}

}  // namespace
}  // namespace test
}  // namespace net
