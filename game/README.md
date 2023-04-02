Core game functionality, independent of any platform. Sits next to the [`platform` subdirectory](../platform/) in the layer hierarchy, and can `#include` code from the top level of `platform/`.

Must not contain anything that is implemented in terms of system calls, but *can* contain platform-independent base classes for platform-specific functionality.
