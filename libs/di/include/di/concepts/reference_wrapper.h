#pragma once

#include <di/meta/core.h>
#include <di/meta/remove_cv.h>

namespace di::util {
template<typename T>
class ReferenceWrapper;
}

namespace di::concepts {
template<typename T>
concept ReferenceWrapper = InstanceOf<meta::RemoveCV<T>, util::ReferenceWrapper>;
}
