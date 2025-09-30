#include <spine/spine.h>

spine::SpineExtension *spine::getDefaultExtension() {
  return new DefaultSpineExtension();
}
