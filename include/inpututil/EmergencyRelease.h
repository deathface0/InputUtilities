#pragma once

namespace inpututil {

/// Installs process-wide handlers that release every key and button held by
/// any Input when the process is about to die abnormally, where destructors
/// do not run:
///   - crashes (unhandled SEH exceptions such as access violations),
///   - abort() and std::terminate (uncaught C++ exceptions) on any thread,
///   - std::exit() (locals are not destroyed),
///   - closing the console, Ctrl+C, Ctrl+Break, logoff and shutdown.
/// Previously installed handlers are chained, not replaced. Calling it more
/// than once does nothing. It releases regardless of Config::releaseOnDestroy,
/// also when main() returns normally (the std::exit handler runs then too).
///
/// Nothing can run when the process is killed (TerminateProcess, Task
/// Manager, taskkill /F); use Config::abortKey to stop a runaway macro instead.
void installEmergencyRelease();

/// Releases right now everything held by every live Input. Used by the
/// handlers above; safe to call by hand.
void emergencyReleaseAll() noexcept;

} // namespace inpututil
