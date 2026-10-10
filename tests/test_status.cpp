#include "TestSupport.h"

#include <inpututil/Status.h>

using inpututil::Error;
using inpututil::Status;

TEST_CASE("default Status is success") {
    Status st;
    CHECK(st.ok());
    CHECK(static_cast<bool>(st));
    CHECK(st == Error::None);
    CHECK(st.win32Error() == 0);
    CHECK(st.message() == "None");
}

TEST_CASE("Status converts implicitly from Error and keeps the Win32 code") {
    Status st = Error::SystemFailure;
    CHECK_FALSE(st);
    CHECK(st.error() == Error::SystemFailure);

    Status withCode{Error::SystemFailure, 5}; // ERROR_ACCESS_DENIED
    CHECK(withCode.win32Error() == 5);
    CHECK(withCode.message().rfind("SystemFailure (Win32 error 5", 0) == 0);
}

TEST_CASE("every error has a name") {
    CHECK(inpututil::errorName(Error::PartialSend) == "PartialSend");
    CHECK(inpututil::errorName(Error::UnmappableCharacter) == "UnmappableCharacter");
    CHECK(inpututil::errorName(Error::TargetNotReached) == "TargetNotReached");
    CHECK(inpututil::errorName(Error::Aborted) == "Aborted");
}

TEST_CASE("Progress is a Status that also tells how far an operation got") {
    using inpututil::Progress;

    const Progress done{Status{}, 4};
    CHECK(done);
    CHECK(done.completed() == 4);
    CHECK_FALSE(done.failedAt());

    const Progress failed{Status{Error::PartialSend, 5}, 2, 2};
    CHECK_FALSE(failed);
    CHECK(failed == Error::PartialSend);
    CHECK(failed.win32Error() == 5);
    CHECK(failed.completed() == 2);
    CHECK(failed.failedAt() == 2u);

    const Progress fromError = Error::InvalidArgument;
    CHECK(fromError == Error::InvalidArgument);
    CHECK(fromError.completed() == 0);
    CHECK_FALSE(fromError.failedAt());

    const Status plain = failed; // usable wherever a Status is expected
    CHECK(plain == Error::PartialSend);
}
