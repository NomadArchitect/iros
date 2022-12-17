#pragma once

#include <di/parser/basic/code_point_parser.h>
#include <di/parser/basic/eof_parser.h>
#include <di/parser/combinator/sequence.h>
#include <di/parser/concepts/parser.h>
#include <di/parser/concepts/parser_context.h>
#include <di/parser/create_parser.h>
#include <di/parser/integral_set.h>
#include <di/parser/into_parser_context.h>
#include <di/parser/meta/parser_context_error.h>
#include <di/parser/meta/parser_value.h>
#include <di/parser/parse.h>
#include <di/parser/parse_partial.h>
#include <di/parser/string_view_parser_context.h>

namespace di {
using parser::parse;
using parser::parse_partial;
}