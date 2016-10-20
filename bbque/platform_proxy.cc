#include "bbque/platform_proxy.h"

namespace bbque
{

plugins::PlatformLoaderIF * PlatformProxy::pli = nullptr;

bool PlatformProxy::IsHighPerformance(bbque::res::ResourcePathPtr_t const & path) const {
	(void) path;
	return false;
}

} //namespace bbque
