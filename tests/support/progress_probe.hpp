#pragma once

#include <Message_ProgressIndicator.hxx>
#include <Message_ProgressScope.hxx>
#include <algorithm>
#include <atomic>

namespace tests::support
{
    /**
     * @brief An OCCT progress indicator for tests: records the furthest
     * position reached, and reports a user break once cancel is set.
     *
     * @code{.cpp}
     * occ::handle<ProgressProbe> probe = new ProgressProbe();
     * Shape::fromSTL(path, probe->Start());
     * CHECK(probe->maxPosition >= 0.99);
     * @endcode
     */
    class ProgressProbe : public Message_ProgressIndicator
    {
      public:
        std::atomic<bool> cancel{false};
        double            maxPosition = 0.0;

        bool UserBreak() override
        {
            return cancel;
        }

        void Show(const Message_ProgressScope&, const bool) override
        {
            maxPosition = std::max(maxPosition, GetPosition());
        }
    };
} // namespace tests::support
