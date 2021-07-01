#pragma once

#include <liim/forward.h>
#include <liim/string.h>
#include <liim/vector.h>

namespace Ext {
class Path {
public:
    static Maybe<Path> resolve(const String& path);

    Path();
    ~Path();

    String basename() const;
    String to_string() const;

    void set_to_parent();

    String join_component(const String& name) const;

private:
    Path(const String& path);

    Vector<String> m_components;
};
}