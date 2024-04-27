#include <emscripten/bind.h>
#include <Tag.h>

using namespace emscripten;

Tag Tag_fromArray(val array);
val Tag_asArray(const Tag & tag);
Tag Tag_clone(const Tag & tag);
bool Tag_eqOther(const Tag & tag, const Tag & other);
void Tag_setFromOther(Tag & tag, const Tag & other);
void Tag_setFromArray(Tag & tag, val array);