#include <emscripten/bind.h>
#include <VectorTimestamp.h>

using namespace emscripten;

VectorTimestamp VectorTimestamp_fromArray(val array);
val VectorTimestamp_asArray(const VectorTimestamp & clock);
VectorTimestamp VectorTimestamp_clone(const VectorTimestamp & clock);
bool VectorTimestamp_ltOther(const VectorTimestamp & clock, const VectorTimestamp & other);
bool VectorTimestamp_eqOther(const VectorTimestamp & clock, const VectorTimestamp & other);
void VectorTimestamp_setFromOther(VectorTimestamp & clock, const VectorTimestamp & other);