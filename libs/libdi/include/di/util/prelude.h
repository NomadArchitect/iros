#pragma once

#include <di/util/as_const.h>
#include <di/util/clone.h>
#include <di/util/create.h>
#include <di/util/declval.h>
#include <di/util/exchange.h>
#include <di/util/forward.h>
#include <di/util/get.h>
#include <di/util/initializer_list.h>
#include <di/util/move.h>
#include <di/util/reference_wrapper.h>
#include <di/util/source_location.h>
#include <di/util/compile_time_fail.h>
#include <di/util/swap.h>
#include <di/util/unreachable.h>

namespace di {
using util::clone;
using util::create;

using util::cref;
using util::ref;
using util::ReferenceWrapper;

using util::InitializerList;

using util::SourceLocation;

using util::as_const;
using util::compile_time_fail;
using util::declval;
using util::exchange;
using util::forward;
using util::get;
using util::move;
using util::swap;
using util::unreachable;
}
