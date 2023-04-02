Core game functionality, independent of any platform. Sits below the [`platform` subdirectory](../platform/) in the layer hierarchy.

Must not contain anything that is implemented in terms of system calls, but *can* contain platform-independent base classes for platform-specific functionality.
